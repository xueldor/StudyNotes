/******************************************************************************
 * Copyright 2023 Shenzhen Hangsheng Electronics Co., Ltd. All Rights Reserved.
 *
 * This software and documentation contain confidential and proprietary
 * information which is the property of HSAE Co., Ltd. Unauthorized use,
 * copying, or distribution of this software, its source code, or its
 * documentation, via any medium, is strictly prohibited without the prior
 * written consent of HSAE Co., Ltd.
 *
 ******************************************************************************/
#include "Utils.h"

#include <android-base/logging.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>
#include <android-base/unique_fd.h>
#include <csignal>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <memory>
#include <string>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>
#include <backtrace/Backtrace.h>

/**
 * string array convert to char* array
 */
static std::vector<const char*> ConvertToArgv(const std::vector<std::string>& args) {
  std::vector<const char*> argv;
  argv.reserve(args.size() + 1);
  for (const auto& arg : args) {
    if (argv.empty()) {
      LOG(DEBUG) << arg;
    } else {
      LOG(DEBUG) << "    " << arg;
    }
    argv.emplace_back(arg.data());
  }
  argv.emplace_back(nullptr);
  return argv;
}

static status_t ReadLinesFromFdAndLog(std::vector<std::string>* output,
                                      android::base::unique_fd ufd) {
  std::unique_ptr<FILE, int (*)(FILE*)> fp(android::base::Fdopen(std::move(ufd), "r"), fclose);
  if (!fp) {
    PLOG(ERROR) << "fdopen in ReadLinesFromFdAndLog";
    return -errno;
  }
  if (output) output->clear();
  char line[1024];
  while (fgets(line, sizeof(line), fp.get()) != nullptr) {
    LOG(DEBUG) << line;
    if (output) output->emplace_back(line);
  }
  return OK;
}

status_t ForkExecvp(const std::vector<std::string>& args, int& forkPid, std::vector<std::string>* output,
                    security_context_t context) {
      auto argv = ConvertToArgv(args);

    android::base::unique_fd pipe_read, pipe_write;
    if (!android::base::Pipe(&pipe_read, &pipe_write)) {
        PLOG(ERROR) << "Pipe in ForkExecvp";
        return -errno;
    }

    pid_t pid = fork();
    if (pid == 0) {
        if (context) {
            if (setexeccon(context)) {
                LOG(ERROR) << "Failed to setexeccon in ForkExecvp";
                abort();
            }
        }
        pipe_read.reset();
        if (dup2(pipe_write.get(), STDOUT_FILENO) == -1) {
            PLOG(ERROR) << "dup2 in ForkExecvp";
            _exit(EXIT_FAILURE);
        }
        pipe_write.reset();
        execvp(argv[0], const_cast<char**>(argv.data()));
        PLOG(ERROR) << "exec in ForkExecvp";
        _exit(EXIT_FAILURE);
    }
    forkPid = pid;
    if (pid == -1) {
        PLOG(ERROR) << "fork in ForkExecvp";
        return -errno;
    }
    pipe_write.reset();
    auto st = ReadLinesFromFdAndLog(output, std::move(pipe_read));
    if (st != 0) return st;

    int status = 0;
    if (waitpid(pid, &status, 0) == -1) {
        PLOG(ERROR) << "waitpid in ForkExecvp";
        return -errno;
    }
    if (!WIFEXITED(status)) {
        LOG(ERROR) << "Process did not exit normally, status: " << status;
        return -ECHILD;
    }
    if (WEXITSTATUS(status)) {
        LOG(ERROR) << "Process exited with code: " << WEXITSTATUS(status);
        return WEXITSTATUS(status);
    }
    return OK;
}

bool IsRecoveryMode() {
    return access("/system/bin/recovery", F_OK) == 0;
}
static std::string getTime() {
  time_t timep;
  time(&timep);
  char tmp[64];
  strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S", localtime(&timep));
  return tmp;
}
void initLogger(android::base::LogId /* id */, android::base::LogSeverity severity,
                         const char* tag, const char* /* file */, unsigned int /* line */,
                         const char* message) {
  static constexpr char log_characters[] = "VDIWEF";
  fprintf(stdout, "%s  %d  %d %c mcuupdater_%s: %s\n", getTime().c_str(), getpid(), gettid(),
          log_characters[severity], tag, message);
}

//只能获得当前位置的堆栈
static void printStackTrace(void *context) {
    int status;
    std::unique_ptr<Backtrace> backtrace(
            Backtrace::Create(-1, -1)); // -1 means current thread and current thread
    if(backtrace == nullptr) {
      return;
    }
    if (!backtrace->Unwind(0, context)) {
        LOG(ERROR) << __FUNCTION__ << " Failed to unwind callstack.";
    }
    LOG(ERROR) << __FUNCTION__ << " unwind "<<backtrace->NumFrames()<<std::endl;
    for (size_t i = 0; i < backtrace->NumFrames(); i++) {
        LOG(ERROR)<<"xxxxxxxxx " << backtrace->FormatFrameData(i);
    }

}
// static void tryAcquireTargetBacktrace() {
//       if (t->frames_size == 0) {
//         size_t i;
//         t->frames_size = unwind_signal(si, sc, t->uframes, 0,BACKTRACE_FRAMES_MAX);
//         for(i = 0 ; i < t->frames_size ; i++) {
//             t->frames[i].absolute_pc = (uintptr_t) t->uframes[i];
//             t->frames[i].stack_top = 0;
//             t->frames[i].stack_size = 0;
//             __android_log_print(ANDROID_LOG_DEBUG, TAG, "absolute_pc:%x", t->frames[i].absolute_pc);
//         }
//     }
// }
void signal_handle(int code, siginfo_t *si, void *context) {
  LOG(ERROR) <<"Receive signal "<<code << " si_code="<<si->si_code<<" si_signo="<<si->si_signo<<" si_errno"<<si->si_errno;
  printStackTrace(context);
  //这里一定要调用_exit退出程序，否则不停收到signal，循环打印。
  _exit(-code);
}

void initHandler() {
  // 试验发现，只要主线程执行initHandler一次，哪个线程上的异常，signal_handle就在哪个线程执行
    int signal_array[] = {SIGILL, SIGABRT, SIGBUS, SIGFPE, SIGSEGV, SIGSTKFLT, SIGSYS, SIGTRAP};
    int SIGNALS_LEN = sizeof(signal_array)/sizeof(int);
    struct sigaction old_signal_handlers[SIGNALS_LEN];

    struct sigaction handler;
    handler.sa_sigaction = signal_handle;
    handler.sa_flags = SA_SIGINFO;

    for (int i = 0; i < SIGNALS_LEN; ++i) {
        sigaction(signal_array[i], &handler, & old_signal_handlers[i]);
        // signal(signal_array[i], signal_handle);
    }
    
}