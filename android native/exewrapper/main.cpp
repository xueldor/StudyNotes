#include <android-base/logging.h>
#include <bits/getopt.h>
#include <cstdlib>
#include <log/log_main.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <sys/wait.h>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include "Utils.h"
#include <backtrace/Backtrace.h>


using std::string;

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "exewrapper"

int runexe(char path[256], char selinuxlabel[256] = nullptr) {
  ALOGI("path=%s", path);
  std::vector<std::string> cmd;
  char delims[] = " \t";// 半角字符的空格，或制表符
  
  char *result = NULL;
  result = strtok( path, delims );
  while( result != NULL ) {
      ALOGI( "result is \"%s\"\n", result );
      cmd.push_back(result);
      result = strtok( NULL, delims );
  }

  std::vector<std::string> output;
  status_t st;
  int forkedPid = -1;
  // selinux 标签
  if (selinuxlabel != nullptr) {
    // 格式例如： "u:r:testsubprocess_system:s0"
    security_context_t sContext  = selinuxlabel;
    st = ForkExecvp(cmd, forkedPid, &output, sContext);
  } else {
    st = ForkExecvp(cmd, forkedPid, &output, nullptr);
  }
  printf("pid of %s is %d, output:\n", path, forkedPid);
  for (std::string& line : output) {
      printf("%s\n", line.c_str());
  }
  return st;
}

void testCrash2() {
    std::this_thread::sleep_for(std::chrono::seconds(2));
    string* str = nullptr;
    printf("%s",str->c_str());
}
void testCrash() {
    testCrash2();
}


int main(int argc, char* argv[]) {
  if (IsRecoveryMode()) {
    android::base::InitLogging(argv, &initLogger);
  }
  initHandler();
  ALOGD("start.");
  int c;
  int ret = 0;
  char path[256] = { 0 };
  char selinuxlabel[256] = { 0 };
  bool runSubProcess = false;
  bool withSelinux = false;
  
  while ((c = getopt(argc, argv, "r:s:h")) != -1) {
    switch (c) {
      case 'r':
        ALOGI("run: %s", optarg);
        strncpy(path, optarg, sizeof(path) - 1);
        runSubProcess = true;
        break;
      case 's':
        strncpy(selinuxlabel, optarg, sizeof(selinuxlabel) - 1);
        printf("selinux: %s\n", selinuxlabel);
        withSelinux = true;
        break;
      case 'h':
        printf("help, unimplement yet()\n");
        break;
      default:
        fprintf(stderr, "Unknown option: -%c\n", optopt);
        // help();
        return 1;
    }
  }
  optind = 1;

  status_t st;
  if (runSubProcess) {
    if (withSelinux) {
      st = runexe(path, selinuxlabel);
    } else {
      st = runexe(path);
    }
    if (st != 0) {
      printf("子进程异常退出，code= %d\n", st);
    }else{
      printf("finished\n");
    }
  }
  //  for test signal
  std::thread testThread([](){
      testCrash();
  });
  testThread.detach();
  
  std::this_thread::sleep_for(std::chrono::seconds(30));
  return 0;
}