现象：

​	安卓11，高通平台。在init进程的源码里添加了日志，编译，push到system/bin/下面，不生效。

原因：

 1. 除了/system/bin/init，还需要刷入boot.img。原因可能是启动阶段，init刚执行的时候，system分区还没挂载，执行的ramdisk里的。

 2. InitKernelLogging(argv);这行代码后面的LOG打印才有效，前面的不会输到logcat。

    

