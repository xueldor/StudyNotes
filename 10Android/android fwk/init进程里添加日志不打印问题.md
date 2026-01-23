现象1：

​	安卓11，高通平台。在init进程的源码里添加了日志，编译，push到system/bin/下面，不生效。

原因：

 1. 除了/system/bin/init，还需要刷入boot.img。原因是启动的init first stage阶段，system分区还没挂载，执行的ramdisk里的。

 2. InitKernelLogging(argv);这行代码后面的LOG打印才有效，前面的不会输到logcat。

    

现象2：

在开机启动过程中，logcat日志似乎中间会丢失一段。

* logd启动后

  原因不明。可以我们自己把日志写到/dev/bootprof/bootprof文件里.。/dev是tmpfs，可用。

  ```c++
  void bootProf(const char* msg) {
      int fd = open("/dev/bootprof/bootprof", O_RDWR | O_CREAT | O_APPEND, 0777);
      if (fd == -1) {
          return;
      }
      const size_t BUF_SIZE = 256;
      char buf[BUF_SIZE];
      memset(buf, 0, BUF_SIZE);
  
      if (lseek(fd, 0, SEEK_END) == 0 || lseek(fd, -1, SEEK_END) < 0 || read(fd, buf, 1) != '\n') {
          lseek(fd, 0, SEEK_END); // 移动到文件末尾
          write(fd, "\n", 1);      // 添加换行符
      }
      struct timespec now;
      clock_gettime(CLOCK_MONOTONIC, &now);
      sprintf(buf, "%ld.%ld %s", now.tv_sec, now.tv_nsec/1000000, msg);
      if (fd > 0) {
          write(fd, buf, strlen(buf));
          close(fd);
      }
  }
  ```

* logd启动前

理论上，在init first stage，logd服务还么启动，安卓日志应该不生效，但实际，在`InitKernelLogging(argv);`后面的日志是有的, 因为InitKernelLogging把log定向到了内核。虽然不会写入logd，但是会写入内核。当后面logd启动后，会从内核读取，插入到logd的buffer。所以日志应该是有的。

如果依旧由丢日志现象，或者要在InitKernelLogging之前写日志，可采用以下方案。因为first stage，太早的时候，文件系统还没准备好，写到/dev/bootprof/bootprof文件的方案也不行，就需要自己写入/dev/kmsg。可以使用klog_write函数。

（因为这个阶段，上面的bootprof也无效，只能通过向内核输出。ATC平台他们自己自定义了bootprof驱动在内核层实现，高通没这玩意，移植成本高）

```c++
#include <cutils/klog.h>
void klog_message(const char* msg) {
    const size_t BUF_SIZE = 256;
    char buf[BUF_SIZE];
    memset(buf, 0, BUF_SIZE);

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    sprintf(buf, "%ld.%ld %s", now.tv_sec, now.tv_nsec/1000000, msg);

    klog_write(KLOG_WARNING_LEVEL, "%s", buf);
}
```

