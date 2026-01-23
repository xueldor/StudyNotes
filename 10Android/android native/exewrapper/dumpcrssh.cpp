
#include <android-base/logging.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>
#include <android-base/unique_fd.h>
#include <backtrace/Backtrace.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include <csignal>
#include <memory>
#include <string>
#include <utility>
#include <vector>

// 用`addr2line  -C -f -p -i -e symbols/recovery/root/system/lib64/localserial-impl.so xxxx`得到位置
static void printStackTrace(void *context) {
  int status;
  std::unique_ptr<Backtrace> backtrace(
      Backtrace::Create(-1, -1));  // -1 means current thread and current thread
  if (backtrace == nullptr) {
    return;
  }
  if (!backtrace->Unwind(0, context)) {
    LOG(ERROR) << __FUNCTION__ << " Failed to unwind callstack.";
  }
  LOG(ERROR) << __FUNCTION__ << " unwind " << backtrace->NumFrames() << std::endl;
  for (size_t i = 0; i < backtrace->NumFrames(); i++) {
    LOG(ERROR) << backtrace->FormatFrameData(i);
  }
}

int signal_array[] = {SIGILL, SIGABRT, SIGBUS, SIGFPE, SIGSEGV, SIGSTKFLT, SIGSYS, SIGTRAP};
struct sigaction old_signal_handlers[sizeof(signal_array) / sizeof(int)];

void signal_handle(int code, siginfo_t *si, void *context) {
  LOG(ERROR) << "Receive signal " << code << ", si_signo=" << si->si_signo
             << " si_code=" << si->si_code << " si_errno=" << si->si_errno;
  printStackTrace(context);
  // 日志打印backtrace后调用_exit退出程序
  // 如果发生异常时，已经收到来自MCU的激活result，就忽略错误，返回激活结果；否则返回异常码

    int SIGNALS_LEN = sizeof(signal_array) / sizeof(int);
    for (int i = 0; i < SIGNALS_LEN; i++) {
        if (code == signal_array[i]) {
        struct sigaction* old = &old_signal_handlers[i];
        if (old == nullptr) {
            break;
        }
        if (old->sa_handler == SIG_DFL) {
            /* 先前的信号处理程序是默认处理程序 */
            LOG(ERROR) <<"SIG_DFL1";
        } else if (old->sa_handler == SIG_IGN) {
            /* 先前的信号处理程序是忽略信号 */
            LOG(ERROR) <<"SIG_IGN";
        } else {
            /* 先前安装了自定义的信号处理程序 */
            LOG(ERROR) <<"3";
            old->sa_sigaction(code, si, context);
        }
        }
    }
    _exit(-code);
}

void initHandler() {
  int SIGNALS_LEN = sizeof(signal_array) / sizeof(int);

  char myaltstack[SIGSTKSZ];
  stack_t ss;
  ss.ss_sp = myaltstack;
  ss.ss_size = sizeof(myaltstack);
  ss.ss_flags = 0;
  if (sigaltstack(&ss, NULL))
    _exit(-1);

  struct sigaction handler;
  handler.sa_sigaction = signal_handle;
  handler.sa_flags = SA_SIGINFO|SA_ONSTACK;

  for (int i = 0; i < SIGNALS_LEN; ++i) {
    sigaction(signal_array[i], &handler, &old_signal_handlers[i]);
  }
}
