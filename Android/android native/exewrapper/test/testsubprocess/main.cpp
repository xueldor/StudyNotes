#include <android-base/logging.h>
#include <bits/getopt.h>
#include <chrono>
#include <log/log_main.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <string>
#include <vector>
#include <thread>

using std::string;

#define LOG_TAG "testsubprocess"

static string getTime() {
  time_t timep;
  time(&timep);
  char tmp[64];
  strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S", localtime(&timep));
  return tmp;
}
static void initLogger(android::base::LogId /* id */, android::base::LogSeverity severity,
                         const char* tag, const char* /* file */, unsigned int /* line */,
                         const char* message) {
  static constexpr char log_characters[] = "VDIWEF";
  fprintf(stdout, "%s  %d  %d %c mcuupdater_%s: %s\n", getTime().c_str(), getpid(), gettid(),
          log_characters[severity], tag, message);
}

bool IsRecoveryMode() {
  ALOGI("xxxx iiiiii");
  std::this_thread::sleep_for(std::chrono::seconds(3));
  string* str = nullptr;
  printf("%s",str->c_str());
    return access("/system/bin/recovery", F_OK) == 0;
}

int main(int argc, char* argv[]) {
  if (IsRecoveryMode()) {
    android::base::InitLogging(argv, &initLogger);
  }
  int i = 0;
  int ret = 0;
  char path[256] = { 0 };
  while (true) {
    ALOGI("i=%d", i);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    ++i;
  }
  return 0;
}