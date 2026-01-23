一. 测试system和vendor分区bin目录下程序的互相调用
在红旗C801系统(安卓12)测试用exewrapper运行test。分别把exewrapper和test放在system/bin和vendor/bin。共四种组合。

1. exewrapper里通过system函数运行test

* exewrapper在vendor，test在system
  结果：test的路径用绝对路径可以调用，只写test程序名不能找到程序。

  ```
  msmnile_gvmq:/ # exewrapper -r testsubprocess
  sh: testsubprocess: inaccessible or not found
  finished, return 32512
  msmnile_gvmq:/ # exewrapper -r /system/bin/testsubprocess
  ```
* exewrapper在system,test在vendor。或者二者同在system或同在vendor。
  结果：可以调用，不需要路径写全

  结论是：通过system()函数调用时，vendor里程序找不到system里的。反之可以。

2. 通过自己写的ForkExecvp函数执行。原理是fork+execvp
   结论是: 表明无论是都在system或vendor，一个在system/bin,test另一个在/vendor/bin，都可以运行。不需要写全绝对路径就可以找到。

综合结论： 安卓系统对于一个进程执行另一个进程，没有限制。只是有的时候vendor下进程会找不到system下的文件。这时只要用绝对路径即可。

疑点：当前是在shell里以root身份执行的。是否普通程序会有权限限制？

二. 测试system和vendor分区程序读文件的权限限制

结论：只要有绝对路径，root或system身份运行的程序，无论在system还是vendor，都可以读任何目录的文件。

以前开发过一个hal服务位于/vendor/bin，由init.rc拉起，无法把配置文件写在/data/system，必须用/data/vendor。所以权限控制肯定是有的。只是我用shell执行，测不出来。抑或读可以，写有限制？

三. 测试system的程序使用vendor lib64下的so。或反过来。

* helloworld动态库，Android.bp指定vendor_available: true，编译到vendor下

1. exewrapper编译到system的bin，bp里引用helloworld动态库。helloworld是vendor的。编译失败。暗示system不能引用vendor的库。
2. bp指定recovery:true，引用helloworld动态库失败。这是理所当然的，无需解释。
3. exewrapper和helloworld都是vendor，编译通过。

* helloworld动态库，Android.bp不指定vendor，编译到system.img里

1. exewrapper编译到system，bp里引用helloworld动态库。编译通过。
2. exewrapper编译到vendor, helloworld是system，编译失败。

综上，只有程序和引用的库都在system或者都在vendor时，采用引用到。

接下来，将可执行程序和helloworld.so推送到车机，四种组合验证，证明了上述判断。

根据谷歌文档https://source.android.google.cn/docs/core/architecture/vndk/enabling?hl=zh-cn，

system肯定不能访问vendor的库。vendor访问system分区的库要分情况：

* 不能访问核心库
* 可以访问LL-NDK、VNDK-sp、VNDK。

这也好理解。VNDK就是平台提供给vendor用的NDK，所以放在system，可以由vendor访问。

四. 测试一个进程运行一个子进程时，子进程的selinux标签。

结果表明，如果没有配置selinux，子进程会继承父进程的标签。

如果配置了selinux。即如下语句：

```
# 在file_contexts里
/system/bin/testsubprocess                        u:object_r:testsubprocess_system_exec:s0

#te文件里
type testsubprocess_system, domain;
type testsubprocess_system_exec, file_type, system_file_type;
# 当type为exewrapper_system 的程序执行type为testsubprocess_system_exec的可执行文件时，将被运行起来的进程的type转变为testsubprocess_system
type_transition exewrapper_system testsubprocess_system_exec:process testsubprocess_system;

```

则子进程的标签是 `u:r:testsubprocess_system:s0。`

另外也可以通过代码来指定子进程的label，此时te文件只需要定义 `type testsubprocess_system, domain;`即可：

```c++
std::string cc = "u:r:testsubprocess_system:s0";
security_context_t sContext  = cc.data();
status_t st = ForkExecvp(cmd, &output, sContext);

```

ForkExecvp函数会在**fork** 的后面， **execvp**的前面调用setexeccon函数。bp依赖libselinux库。

五. 通过exewrapper执行子进程时，如果子进程崩溃，exewrapper如何捕获子进程的堆栈信息

1. 尝试父进程里用libbacktrace库获取堆栈。注册SIGCHLD即可在子进程崩溃时，父进程收到。libbacktrace用法也很简单。

   ```cpp
   #include <backtrace/Backtrace.h>

   std::unique_ptr<Backtrace> backtrace(
               Backtrace::Create(-1, -1)); // -1 means current thread and current thread

       if (!backtrace->Unwind(0)) {
           LOG(ERROR) << __FUNCTION__ << " Failed to unwind callstack.";
       }
       LOG(ERROR) << __FUNCTION__ << " unwind "<<backtrace->NumFrames()<<std::endl;
       for (size_t i = 0; i < backtrace->NumFrames(); i++) {
           LOG(ERROR)<<"xxxxxxxxx " << backtrace->FormatFrameData(i);// 加xxxx是为了证明确实是这里打印的。因为安卓自身也会捕获崩溃的进程
       }
   ```
   然而libbacktrace只能捕获本进程的，不能抓取其它进程。这一点头文件Backtrace.h里面的注释写的很清楚。一开始没看，浪费了很多时间。
2. 那就尝试在自身程序里监听崩溃信号，然后捕获堆栈。

   ```c++
   // 接收这些信号
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
   ```
   然后发现，获取的堆栈是signal_handler是本身的，而不是异常时的堆栈：

   ```shell
   02-20 16:41:36.116  3611  3611 E exewrapper: xxxxxxxxx #00 pc 0000000000002608  /system/bin/exewrapper (printStackTrace()+96)
   02-20 16:41:36.116  3611  3611 E exewrapper: xxxxxxxxx #01 pc 000000000000259c  /system/bin/exewrapper (signal_handle(int, siginfo*, void*)+124)
   02-20 16:41:36.116  3611  3611 E exewrapper: xxxxxxxxx #02 pc 00000000000005c0  [vdso:0000007e54795000] (__kernel_rt_sigreturn)
   02-20 16:41:36.116  3611  3611 E exewrapper: xxxxxxxxx #03 pc 000000000000212c  /system/bin/exewrapper (main+228)
   ```
   进一步研究发现，signal_handle的第三个参数context，是跳转到signal处理函数之前的上下文。应该把context传过去：`backtrace->Unwind(0, context)`。然后就能捕获backtrace了。具体见代码。

   然后结合addr2line清楚的定位到代码在哪里行。

   ```
   addr2line -C -f -i -p -e xxx/symbols/xxx/lib64/localserial-impl-symbol.so 000000000001e5e8
   ```
3. 还有一个问题，就是对于栈溢出异常，是捕获不到信号的。原因是signal_handle函数自身也是在栈上面执行的，如果栈本身破坏了或者没空间了，那么当然不会执行。所以栈溢出时 SIGSEGV 信号的处理需要额外处理，方法是用sigaltstack函数注册一个新的信号处理栈。当然其它信号也一起在这个空间处理。

   ```c++
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
   ```
   目录下放了一个dumpcrssh.cpp文件，可以直接用。
4. 安卓系统在有程序崩溃时，会发起一个crash_dump64 -p pid进程。crash_dump64的父进程是1（init）。因此可以研究一下init是如何捕获崩溃的进程，以及crash_dump64的代码。不出意外应该和libbacktrace一样也是通过libunwindstack库做的。
5. 可以参考腾讯bugly的博客 https://mp.weixin.qq.com/s/g-WzYF3wWAljok1XjPoo7w，《Android 平台 Native 代码的崩溃捕获机制及实现》
