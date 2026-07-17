# 概念

LLDB 是 LLVM 项目中的调试器，支持多种编程语言和平台。lldb-server 是 LLDB 的一个组件，充当远程调试服务器，允许 LLDB 客户端连接并控制远程进程。LLDB 和 lldb-server 的关系类似于 GDB 和 gdbserver。但 LLDB 提供了更强大的平台特定 (Platform-specific) 功能，相比于 GDB 将平台交互能力主要集中在 gdbserver 上，LLDB 的 SBPlatform 抽象对象使其具备了更强大的远程文件系统、进程和环境管理能力。

# 对比GDB

1. 远程文件和路径映射：LLDB 的 SBPlatform 知道如何通过 lldb-server 远程访问目标机的文件系统。这对于处理 Sysroot、共享库、查找符号文件以及源代码路径映射至关重要。GDB 则更多依赖于本地文件或手动配置。例如在 LLDB 中，Module 的路径只是一个标识符，它的可达性与访问方式，完全由 Platform 赋予语义。
2. 更细致的远程进程控制：SBPlatform 不仅仅负责启动进程，它管理了整个远程环境的设置，包括环境变量、工作目录等。
3. LLDB 以远端为事实来源，GDB 以本地为事实来源：LLDB 能够通过 SBPlatform 和 lldb-server 协作，更智能地在远程目标上查找依赖的系统库 (Sysroot)，并将其对应的符号加载到宿主机。
4. 可扩展性：平台层具有高度可扩展性，允许开发者为新的 OS 或嵌入式环境编写自定义的平台适配器。

> SBPlatform 是 LLDB 架构中的一个关键抽象层，它充当了核心调试引擎与目标操作系统或执行环境之间的适配器和接口，负责处理所有平台相关的任务，例如进程的启动与管理、文件系统的访问以及在远程调试中与 lldb-server 的网络通信。通过隐藏这些底层细节，SBPlatform 确保了 LLDB 的核心调试逻辑可以保持通用且独立于特定的宿主机或目标机环境，从而实现强大的跨平台调试能力。

## 架构设计对比

| 特性             | GDB (`gdbserver`)                                            | LLDB (`lldb-server` + `SBPlatform`)                          |
| :--------------- | :----------------------------------------------------------- | :----------------------------------------------------------- |
| **架构理念**     | 传统的 C/S 架构，`gdbserver` 功能较单一，主要负责寄存器/内存读写 | 现代化的组件化架构，Platform 层接管了远程环境的完整语义      |
| **文件系统访问** | 弱耦合。GDB 往往假设符号文件位于本地，远程文件读取能力有限   | 强耦合。Platform 知道如何通过 `lldb-server` 高效访问远程文件系统 |
| **路径处理**     | 路径通常被视为本地文件路径，需手动配置 `solib-search-path`   | 路径被视为抽象标识符，由 Platform 决定如何解析（本地或远程） |
| **依赖查找**     | 以**本地**为事实来源。如果本地没有对应的库，调试可能出错     | 以**远端**为事实来源。能自动发现远程依赖并下载/加载符号      |
| **扩展性**       | 主要通过 Python 脚本扩展命令                                 | 核心组件（如 Platform）本身就是插件化的，易于适配新 OS       |

## 远程调试能力对比

| 功能特性            | `gdbserver` | `lldb-server` (gdb-remote) | `lldb-server` (platform) |
| :------------------ | :---------: | :------------------------: | :----------------------: |
| **跨架构调试**      |      ✔      |             ✔              |            ✔             |
| **远程断点 / 单步** |      ✔      |             ✔              |            ✔             |
| **远程进程创建**    |      ✖      |             ✖              |    ✔ (可直接 launch)     |
| **远程文件传输**    |      ✖      |             ✖              |     ✔ (支持 put/get)     |
| **平台自动识别**    |      ✖      |             ✖              |     ✔ (OS/SDK 版本)      |

虽然 LLDB 比 GDB 更现代且强大，但在调试裸机和部分老旧平台时，GDB 仍然具有优势。选择合适的工具才能事半功倍。

本文只探讨LLDB Platform 模式，其它如gdb模式不关注。本文所有的内容只在platform模式下成立。

# 本地

## 准备

新建test.c文件：

```c
#include <stdio.h>

size_t strlen(const char *s) {
    const char *sc;

    for (sc = s; *sc != '\0'; ++sc)
        /* nothing */;
    return sc - s;
}

int main() {
    // 创建 str 字符串
    char str[] = "Hello World";
    // 调用 strlen 函数，并把值赋予 length
    int length = strlen(str);
    // 在终端打印内容
    printf("The length of str is %d\n", length);
    return 0;
}
```

编译：`clang -g test.c -o test`
-g：	生成调试信息（符号表、行号、变量名等）	方便调试（GDB/LLDB 断点、单步执行）
-O0：	不优化，编译最快，调试体验最佳

-O1： 基本优化，平衡速度和编译时间

-O2： 中度优化，推荐用于大多数发布版本

-O3： 激进优化，最大限度提升性能

-Os： 优化代码大小

调试必须加上-g,否则生成的可执行文件不包含供调试器使用的符号表、变量名和行号等信息。这种情况调试时只能看到汇编代码。

-O0参数是默认的，所以不需要加。不要同时-g -O3, 因为-O3导致变量被优化掉，代码执行顺序改变，造成debug混乱。

即：

```
clang -g -O0 xxx   --------OK
clang -g xxx     ------OK
clang -g -O2    ------NG
```

linux上可以用`objdump -g 二进制文件`来查看调试信息。如果输出的内容非常多，甚至溢出终端的屏幕，那说明一般没问题，如果比较少，内容看起来信息熵很小，那多半有问题，需要去排查是否添加了-g -O0等等flag。可以对编译的工具链添加-v参数，让每条编译指令都打印出来。对于大型项目，太难去找哪条编译指令的话，有个技巧是添加一些必定会报错的flag。比如`-jjjjjjjjjjjjjjjjjjjjjj`,让项目编译到这里停下来。

ubuntu安装lldb：

```
xue@S111-CCS2plus:~$ sudo apt install lldb
装好后发现这个lldb版本比较低。卸载。
xue@S111-CCS2plus:~$ sudo apt install lldb-
lldb-10   lldb-11   lldb-12   lldb-18   lldb-6.0  lldb-7    lldb-8    lldb-9  
意识到ubuntu20官方能够安装的最新版本是18，于是sudo apt install lldb-18
```

（安卓NDK里的lldb是给安卓用的，用它调试会出现找不到lldb-server）

## 进入lldb

```
yu@S111-CCS2plus:~/workspace/test$ lldb-18
(lldb) platform status
  Platform: host
    Triple: x86_64-pc-linux-gnu
OS Version: 5.15.0 (5.15.0-139-generic)
  Hostname: 127.0.0.1
WorkingDir: /workspace/xuexiangyu/workspace/test
    Kernel: #149~20.04.1-Ubuntu SMP Wed Apr 16 08:29:56 UTC 2025
    Kernel: Linux
   Release: 5.15.0-139-generic
   Version: #149~20.04.1-Ubuntu SMP Wed Apr 16 08:29:56 UTC 2025
(lldb) target create test
Current executable set to '/workspace/xuexiangyu/workspace/test/test' (x86_64).
(lldb) run
Process 692966 launched: '/workspace/xuexiangyu/workspace/test/test' (x86_64)
The length of str is 11
Process 692966 exited with status = 0 (0x00000000) 
(lldb) 
```

platform status会显示当前平台的状态信息，其中包括当前工作目录（Working Directory）。

`platform settings -w /workspace/xuexiangyu/workspace/test`用于设置当前工作目录。

`target create  xxx`用于创建调试目标（即加载可执行文件）

run或process launch用于启动或重新启动被调试的程序，run更简洁易记， process launch提供更精确控制和更丰富的选项。

这里由于没有设置断点，所以执行run立即就结束里，输出The length of str is 11。

也可以直接lldb-18 xxx文件进入，会自动帮你target create xxx：

```
xue@S111-CCS2plus:~/workspace/test$ lldb-18 test
```

## 添加断点

| 命令                                            | 说明                            |
| ----------------------------------------------- | ------------------------------- |
| b  main 等价 breakpoint set -n main             | 在 main 函数打断点              |
| b  test.c:10                                    | 在 test.c 文件的第 10 行打断点  |
| b  my_function 或 breakpoint set -n my_function | 在自定义函数 my_function 打断点 |
| breakpoint  list 或 br l                        | 查看所有断点                    |
| breakpoint  delete 1 或 br del 1                | 删除编号为 1 的断点             |
| breakpoint  disable 1 或 br dis 1               | 禁用编号为 1 的断点             |
| breakpoint  enable 1 或 br en 1                 | 启用编号为 1 的断点             |

下面我在test的main函数添加断点：

```shell
(lldb) target create "test"
Current executable set to '/workspace/xuexiangyu/workspace/test/test' (x86_64).
(lldb) b  main                        <--------------------的main函数处添加断点
Breakpoint 1: where = test`main + 15 at test.c:13:10, address = 0x00000000000017ef

(lldb) run                            <--------------------运行程序。你会发现在main的起始处停住。注意观察下面的箭头(13行)
Process 694704 launched: '/workspace/xuexiangyu/workspace/test/test' (x86_64)
Process 694704 stopped
* thread #1, name = 'test', stop reason = breakpoint 1.1
    frame #0: 0x00005555555557ef test`main at test.c:13:10
   10  
   11   int main() {
   12       // 创建 str 字符串
-> 13       char str[] = "Hello World";
   14       // 调用 strlen 函数，并把值赋予 length
   15       int length = strlen(str);
   16       // 在终端打印内容

(lldb) next                        <-------------------------next指令单步执行到下一行(不会进入函数内部)
Process 694704 stopped
* thread #1, name = 'test', stop reason = step over
    frame #0: 0x0000555555555803 test`main at test.c:15:25
   12       // 创建 str 字符串
   13       char str[] = "Hello World";
   14       // 调用 strlen 函数，并把值赋予 length
-> 15       int length = strlen(str);
   16       // 在终端打印内容
   17       printf("The length of str is %d\n", length);
   18       return 0;
(lldb) next
Process 694704 stopped
* thread #1, name = 'test', stop reason = step over
    frame #0: 0x000055555555580f test`main at test.c:17:41
   14       // 调用 strlen 函数，并把值赋予 length
   15       int length = strlen(str);
   16       // 在终端打印内容
-> 17       printf("The length of str is %d\n", length);
   18       return 0;
   19   }
(lldb) 
```

| 命令/简写         | 说明                           |
| ----------------- | ------------------------------ |
| next / n          | 单步跳过（不进入函数）         |
| nexti / ni        | 指令级单步跳过（汇编指令级别） |
| step / s          | 单步进入（进入函数）           |
| stepi / si        | 指令级单步进入                 |
| `next  <count>`   | 连续执行 N 步                  |
| thread  step-over | 完整命令形式                   |

## 关联源码

上面的示例并没有指定源代码路径，为什么能看到代码呢？因为当程序使用 -g 编译时，编译器会将源码的绝对路径或相对路径嵌入到调试信息中，lldb 会自动从调试信息中读取源码路径，并尝试在本地文件系统中查找。

如果把源码文件删除，效果是:

```
(lldb) run
Process 695605 launched: '/workspace/xuexiangyu/workspace/test/test' (x86_64)
Process 695605 stopped
* thread #1, name = 'test', stop reason = breakpoint 1.1
    frame #0: 0x00005555555557ef test`main at test.c:13:10
(lldb)
```

和前面的相比，发现不显示源码了。

如果移动了源码，或者编译和调试不是在一台机器上，比如服务器上编译，代码下载本地调试，那么就会出现，嵌入可执行文件的调试信息中记录的路径，和本地不一致。这种情况就需要设置路径映射。

```
故意移动test.c的位置到backup目录
xue@S111-CCS2plus:~/workspace/test$ tree
.
├── backup
│   └── test.c
└── test

// 设定source-map映射，
(lldb) settings set target.source-map /workspace/xuexiangyu/workspace/test /workspace/xuexiangyu/workspace/test/backup
(lldb) b  main
Breakpoint 2: where = test`main + 15 at test.c:13:10, address = 0x00005555555557ef
(lldb) run
There is a running process, kill it and restart?: [Y/n] Y
Process 696811 exited with status = 9 (0x00000009) killed
Process 697125 launched: '/workspace/xuexiangyu/workspace/test/test' (x86_64)
Process 697125 stopped
* thread #1, name = 'test', stop reason = breakpoint 1.1 2.1
    frame #0: 0x00005555555557ef test`main at test.c:13:10
   10  
   11   int main() {
   12       // 创建 str 字符串
-> 13       char str[] = "Hello World";
   14       // 调用 strlen 函数，并把值赋予 length
   15       int length = strlen(str);
   16       // 在终端打印内容
(lldb) settings clear target.source-map   《------------------清除source-map
```

这里有个需要注意的点。target.source-map 命令的工作原理是进行“前缀替换”。一般都是绝对路径，所以用相对路径，写成：`settings set target.source-map . backup`是不行的。

找不到源码的效果就是，显示汇编。

判断调试信息里保存的是否绝对路径：

```shell
xue@S111-CCS2plus:~/workspace/test$ lldb-18 test
(lldb) target create "test"
Current executable set to '/workspace/xuexiangyu/workspace/test/test' (x86_64).
(lldb) image lookup -vn main
1 match found in /workspace/xuexiangyu/workspace/test/test:
        Address: test[0x00000000000017e0] (test.PT_LOAD[1]..text + 304)
        Summary: test`main at test.c:11
         Module: file = "/workspace/xuexiangyu/workspace/test/test", arch = "x86_64"
    CompileUnit: id = {0x00000000}, file = "/workspace/xuexiangyu/workspace/test/test.c", language = "c11"
       Function: id = {0x00000067}, name = "main", range = [0x00000000000017e0-0x0000000000001828)
       FuncType: id = {0x00000067}, byte-size = 0, decl = test.c:11, compiler_type = "int (void)"
         Blocks: id = {0x00000067}, range = [0x000017e0-0x00001828)
      LineEntry: [0x00000000000017e0-0x00000000000017ef): /workspace/xuexiangyu/workspace/test/test.c:11
         Symbol: id = {0x00000018}, range = [0x00000000000017e0-0x0000000000001828), name="main"
       Variable: id = {0x00000076}, name = "str", type = "char[12]", valid ranges = <block>, location = DW_OP_fbreg -16, decl = test.c:13
       Variable: id = {0x00000081}, name = "length", type = "int", valid ranges = <block>, location = DW_OP_fbreg -20, decl = test.c:15

-----------------根据 CompileUnit: id = {0x00000000}, file = "/workspace/xuexiangyu/workspace/test/test.c"这一行判断。你可以把test移到任意路径，然后执行命令，唯独这行没有跟着变。
如果target create的是不带调试信息的文件，image lookup -vn main 没有CompileUnit这一行。
```

可以通过编译指令参数，改成相对路径“-fdebug-prefix-map=/home/build=/src”。这个不深究。

# 远程

https://lldb.llvm.org/use/remote.html  这是官方文档。

可执行程序在远程机器上执行，代码环境在另一台PC上。

这种情况，我们会在远程机器运行lldb-server,监听端口。代码在的机器上运行lldb连接到lldb-server，发送指令。

为了模拟这种情形，我们把test.c下载到windows上的F盘，以此假设代码是在windows上开发，编出来linux格式的二进制，必须放到linux环境执行。

linux主机的IP是: 192.168.20.111

linux上执行：

```
xue@S111-CCS2plus:~/workspace/test$ lldb-server-18 platform --listen *:1234 --server
```

windows上执行：

```
设置一个可用的python环境，避免被默认设置污染。
C:\Program Files\LLVM\bin>set PYTHONHOME=D:\soft\anaconda3
C:\Program Files\LLVM\bin>set PYTHONPATH=D:\soft\anaconda3\Lib
C:\Program Files\LLVM\bin>set PATH=D:\soft\anaconda3;%PATH%
C:\Program Files\LLVM\bin>lldb
(lldb) platform list          <--------------------- 查询支持的平台，找到remote-linux
Available platforms:
host: Local Windows user platform plug-in.
remote-AIX: Remote AIX user platform plug-in.
remote-linux: Remote Linux user platform plug-in.
remote-android: Remote Android user platform plug-in.
remote-freebsd: Remote FreeBSD user platform plug-in.
remote-gdb-server: A platform that uses the GDB remote protocol as the communication transport.
darwin: Darwin platform plug-in.
remote-ios: Remote iOS platform plug-in.
remote-macosx: Remote Mac OS X user platform plug-in.
host: Local Mac OS X user platform plug-in.
remote-netbsd: Remote NetBSD user platform plug-in.
remote-openbsd: Remote OpenBSD user platform plug-in.
qemu-user: Platform for debugging binaries under user mode qemu
wasm: Platform for debugging Wasm
remote-windows: Remote Windows user platform plug-in.
(lldb) platform connect connect://192.168.20.111:1234  《------------------------连接到lldb-server
 Platform: remote-linux
    Triple: x86_64-pc-linux-gnu
OS Version: 5.15.0 (5.15.0-139-generic)
  Hostname: S111-CCS2plus
 Connected: yes
WorkingDir: /workspace/xuexiangyu/workspace/test
    Kernel: #149~20.04.1-Ubuntu SMP Wed Apr 16 08:29:56 UTC 2025
(lldb) platform shell ls                     <-----------------------------------执行shell指令确认一下当前目录，确认远程路径下有test文件
backup
test
test.c_bak
(lldb) target create test                    <------------------------------------------和本地调试一样，指定调试程序为test
Current executable set to 'test' (x86_64).
(lldb) b main                                <-------------------------------------------添加断点
Breakpoint 1: where = test`main + 15 at test.c:13:10, address = 0x00000000000017ef
(lldb) settings set target.source-map /workspace/xuexiangyu/workspace/test F:\        <---------源码路径映射
(lldb) run
There is a running process, kill it and restart?: [Y/n] Y
Process 776941 exited with status = 9 (0x00000009) killed
(lldb) Process 777545 launched: 'C:\Users\xuexiangyu\.lldb\module_cache\remote-linux\.cache\54722778\test' (x86_64)
Process 777545 stopped
* thread #1, name = 'test', stop reason = breakpoint 1.1
    frame #0: 0x00005555555557ef test`main at test.c:13:10
   10
   11   int main() {
   12       // 鍒涘缓 str 瀛楃涓?[0m
-> 13       char str[] = "Hello World";
   14       // 璋冪敤 strlen 鍑芥暟锛屽苟鎶婂€艰祴浜?length
   15       int length = strlen(str);
   16       // 鍦ㄧ粓绔墦鍗板唴瀹?[0m(lldb) next
Process 777545 stopped
* thread #1, name = 'test', stop reason = step over
    frame #0: 0x0000555555555803 test`main at test.c:15:25
   12       // 鍒涘缓 str 瀛楃涓?[0m
   13       char str[] = "Hello World";
   14       // 璋冪敤 strlen 鍑芥暟锛屽苟鎶婂€艰祴浜?length
-> 15       int length = strlen(str);
   16       // 鍦ㄧ粓绔墦鍗板唴瀹?[0m
   17       printf("The length of str is %d\n", length);
   18       return 0;
(lldb) settings clear target.source-map
```

中文乱码是因为windows侧终端和远程linux侧终端编码格式不一致。在window cmd里先执行一下chcp 65001就可以了。

### 关于target create file的澄清

因为上面，我指定target create test，看起来像是指定里远程路径的文件。直觉上调试需要在远程机器启动程序，所以容易误认为target create 跟远程机器上的路径。其实不然。

语义上，target create指定的是本地路径。然后，在你run的时候，lldb会自动把文件上传到远端平台的工作目录。

因为，lldb需要本地和远程都存在这个文件。本地用于读取调试符号、DWARF 等的文件；远程用来执行。当然，也有一些更高级用法，如单独指定独立的调试符号文件，这里不讨论。（我们这个test示例，调试符号包含在可执行文件里，但是分离出.dSYM包或.pdb是可以做到的）

但是实际，指定本地路径和远程路径都可以。因为Platform 插件比较智能。会根据情况自动同步。如果发现指定的路径，本地文件不存在，远程路径文件存在，会自动帮我们下载到本地.cache目录。比如我这里，你会发现`C:\Users\xue\.lldb\module_cache\remote-linux\.cache\54722778`有个test文件，是在执行target create test时，自动下载过来的。如果本地文件存在，则是run的时候触发上传到远程平台。

所以看起来，就好象既支持本地路径又支持远程路径。

官网提供了方法，精确控制远程的安装位置。文档给出的示例是：

```
(lldb) file a.out
(lldb) script lldb.target.module['a.out'].SetPlatformFileSpec("/bin/a.out")
(lldb) run
```

总之，target create指定的必定是一个带symbols的文件。remote机器上实际运行的则不必带symbols。

# 安卓

如果安卓和PC在一个局域网里，那么安卓相当于上面的remote-linux, 上面的方法适用。但是通常安卓是通过usb以adb方式连接PC的。

另一个是需要解释的是，理论上，安卓设备上确实也可以编译并运行 lldb 客户端，也就是在adb shell里执行lldb。但在实际开发中几乎不会这样做。因为安卓设备的计算资源、存储和交互界面都有限，远不如在 PC 上操作方便。一个明显的问题是，lldb要寻找代码，代码文件肯定是在PC上开发和编译的，嵌入到可执行文件的路径肯定也是PC上的路径，如果纯android里调试，那你还要把代码先push到安卓，且每次要先配置source-map。其它嵌入式设备同理。所以你理解为什么不会把 lldb 客户端放到安卓里了。

目前主流方式是，lldb 客户端在PC上，lldb-server 服务端在安卓设备端。通过usb adb连接。

adb提供了指令`adb forward tcp:1234 tcp:1234`将本地1234端口数据转发到Android 设备（Device）上的 TCP 端口 1234。lldb连接本地1234端口即可。

```
// 先在安卓里启动lldb-server
./lldb-server platform --listen *:1234 --server

// adb forward
C:\Users\xuexiangyu>adb forward tcp:1234 tcp:1234
1234

// 后面就一样了。
C:\Program Files\LLVM\bin>lldb
(lldb) platform select remote-linux
  Platform: remote-linux
 Connected: no
(lldb) platform connect connect://localhost:1234
  Platform: remote-linux
    Triple: aarch64-unknown-linux-android
OS Version: 5.4.86 (5.4.86-qgki-debug-g40acb22f741e)
  Hostname: localhost
 Connected: yes
WorkingDir: /data/local/tmp
    Kernel: #1 SMP PREEMPT Mon May 4 16:37:05 CST 2026
(lldb) platform connect connect://127.0.0.1:1234
error: the platform is already connected to 'localhost', execute 'platform disconnect' to close the current connection
(lldb) platform connect connect://127.0.0.1:1234
  Platform: remote-linux
    Triple: aarch64-unknown-linux-android
OS Version: 5.4.86 (5.4.86-qgki-debug-g40acb22f741e)
  Hostname: localhost
 Connected: yes
WorkingDir: /data/local/tmp
    Kernel: #1 SMP PREEMPT Mon May 4 16:37:05 CST 2026

(lldb) target create D:\workspace\24mmt2\hardware\securityta100\CA\doc\build\libhsckteec\securitychip_pkcs11_ut
warning: (aarch64) C:\Users\xuexiangyu\.lldb\module_cache\remote-linux\.cache\3E00453D-93C2-63F9-F1DC-D178257A131A\libion.so No LZMA support found for reading .gnu_debugdata section
warning: (aarch64) C:\Users\xuexiangyu\.lldb\module_cache\remote-linux\.cache\316B3120-5B8A-84EF-BC6D-1492C06AEEB8\libdl.so No LZMA support found for reading .gnu_debugdata section
warning: (aarch64) C:\Users\xuexiangyu\.lldb\module_cache\remote-linux\.cache\1B99BAD0-6575-7949-B4B5-3A0C1FD55A0D\libm.so No LZMA support found for reading .gnu_debugdata section
warning: (aarch64) C:\Users\xuexiangyu\.lldb\module_cache\remote-linux\.cache\0258740B-928B-138C-E564-C516FD6B9141\libc++.so No LZMA support found for reading .gnu_debugdata section
warning: (aarch64) C:\Users\xuexiangyu\.lldb\module_cache\remote-linux\.cache\661D4366-5D5E-C814-EC19-1E5D951FE16A\liblog.so No LZMA support found for reading .gnu_debugdata section
warning: (aarch64) C:\Users\xuexiangyu\.lldb\module_cache\remote-linux\.cache\C91278A0-FE27-7D0C-DEA8-43D7EEF3233A\libQSEEComAPI.so No LZMA support found for reading .gnu_debugdata section
warning: (aarch64) C:\Users\xuexiangyu\.lldb\module_cache\remote-linux\.cache\5D6AF741-2421-1886-D954-D61C96514A46\libutils.so No LZMA support found for reading .gnu_debugdata section
warning: (aarch64) C:\Users\xuexiangyu\.lldb\module_cache\remote-linux\.cache\DA99C98F-D7D1-B467-A19B-DB466FF48DC1\libcutils.so No LZMA support found for reading .gnu_debugdata section
warning: (aarch64) C:\Users\xuexiangyu\.lldb\module_cache\remote-linux\.cache\01A12DD5-2243-73ED-CC3A-74506F64A9C9\libbase.so No LZMA support found for reading .gnu_debugdata section
warning: (aarch64) C:\Users\xuexiangyu\.lldb\module_cache\remote-linux\.cache\1B885928-4786-9BCE-4CD1-E425028D4A1F\libvndksupport.so No LZMA support found for reading .gnu_debugdata section
(lldb) Current executable set to 'D:\workspace\24mmt2\hardware\securityta100\CA\doc\build\libhsckteec\securitychip_pkcs11_ut' (aarch64).
(lldb) run
error: Failed to connect to 127.0.0.1:55297
```

你会发现，1）.cache多了一堆so。warning是因为无法获取完整调试信息。忽略。

2）error: Failed to connect to 127.0.0.1:55297。 

在 LLDB 的 Platform 模式下，控制端口和数据端口是分开的，因此不仅要转发控制命令交互的端口，还需要转发负责传输调试数据的端口。

执行`adb forward tcp:55297 tcp:55297`，然后重新run，发现文件已自动上传到/data/local/tmp，并执行。

可以手动明确指定两个端口：

```
./lldb-server platform --listen *:1234 --server --gdbserver-port 10000

adb forward tcp:10000 tcp:10000

C:\Users\xuexiangyu>adb forward --list
68346473 tcp:1234 tcp:1234
68346473 tcp:10000 tcp:10000
```

或设定端口范围：`--min-gdbserver-port=17879 --max-gdbserver-port=17880`

## remote-android

debug 安卓设备，上面的方法需要adb forward几个端口。我们可能不知道到底哪些端口需要adb forward、端口是不是每次固定的。所以要先试验、等报错，通过报错信息找到端口。虽然后面又提供里“--gdbserver-port 10000” 的方法，但其实lldb自身提供里更好的方法，就是使用remote-android。

通过platform list指令能看到有remote-android这一项。一但用了它，lldb会自动帮你执行adb forward，无需手动执行。

```
// android shell
./lldb-server platform --listen *:1234 --server

// 本地，无需执行adb forward
(lldb) platform select remote-android
  Platform: remote-android
 Connected: no
(lldb) platform connect connect://localhost:1234   <---------------当使用remote-android，不支持IP,这里必须写localhost。也不支持在hosts文件里自定义
  Platform: remote-android
    Triple: aarch64-unknown-linux-android
OS Version: 30 (5.4.86-qgki-debug-g40acb22f741e)
  Hostname: localhost
 Connected: yes
WorkingDir: /data/local/tmp
    Kernel: #1 SMP PREEMPT Mon May 4 16:37:05 CST 2026
(lldb) target create D:\workspace\24mmt2\hardware\securityta100\CA\doc\build\libhsckteec\securitychip_pkcs11_ut
(lldb) run
```

这个方法自动帮你把所有需要forward的端口都弄好了。端口号都是随机的：

```
C:\Users\xue>adb forward --list
68346473 tcp:22003 tcp:1234
68346473 tcp:22061 tcp:1234
68346473 tcp:22067 tcp:49577
```

根据来自https://lldb.llvm.org/use/remote.html的官网文档:

> When using the “remote-android” platform, the client LLDB forwards two ports, one for connecting to the platform, and another for connecting to the gdbserver. The client ports are configurable through the environment variables ANDROID_PLATFORM_LOCAL_PORT and ANDROID_PLATFORM_LOCAL_GDB_PORT, respectively.

选择 “remote-android” ，可以通过这两个环境变量指定端口号。

根据文档和自测看下来，remote-android好像就是多了自动adb forward端口。是否还有其它功能不清楚。试下来remote-linux、remote-android两个都可行。某些场景下，remote-android反而造成了麻烦。下文"android设备-windows-ubuntu-代码"描述这一方面。

## “设备-windows-ubuntu-代码”模式下的debug

现实项目中，我们的开发方式是：

* 每人一台机器，这台机器通过usb线连接到电脑，电脑可以adb访问
* 安卓native需要在AOSP源码环境开发和编译。公司提供公共服务器，大家在服务器上下载源码。
* 大家都习惯了ssh远程到服务器，直接在服务器上开发。

换句话说，链路是： 代码—(在)—远程ubuntu —(ssh)—本地windows—(adb)—安卓设备

当然，我们可以想到一些曲线救国的方法：
方法1、 想办法直接把usb设备挂载到服务器上。有一些开源免费的USB/IP工具。
方法2、 安卓连接wifi，和服务器在一个局域网，用wifi adb。
方法3、 将服务器映射到本地驱动器路径。或直接把代码下载下来。

抛开这些不谈。让我们正面解决问题。

先分析。

首先，为了远程ubuntu能够识别到adb,需要：

```
windows上执行：
adb server nodaemon -a

ubuntu上执行：
export ADB_SERVER_SOCKET=tcp:192.168.22.36:5037
```

这一点很熟悉了。这时服务器上执行的adb forward实际上是向本地的 adb server 发出创建转发的请求, 创建出来的监听端口仍然是在 adb server 所在的本地电脑 上，而不是远程服务器。

```
// 验证这个观点
// 远程ubuntu上
adb forward tcp:12349 tcp:12349
ss -ltp | grep 12349 //返回空
//到本地windows上
C:\>netstat -ano | findstr "12349"
  TCP    0.0.0.0:12349          0.0.0.0:0              LISTENING       33140

adb forward --list # ubuntu上和windows上显示相同结果，证明观点。
adb forward --remove-all
```

这没问题，我就是要从windows上转发到安卓。

但是，从ubuntu上，adb能识别到机器，不意味着lldb也能。因为他们是通过不同的路径。adb通过ADB_SERVER_SOCKET，lldb根本不会读这个变量。

最容易想到的是,`platform connect connect://localhost:1234`,是否把localhost改成window ip就行？

```shell
xuexiangyu@S111-CCS2plus:~/workspace/test$ lldb-18
(lldb) platform select remote-android
  Platform: remote-android
 Connected: no
(lldb) platform connect connect://localhost:1234    <--------------------意料之中
error: Failed to connect port
(lldb) platform connect connect://192.168.22.36:1234  <-----------------怪事
error: Failed to connect port
(lldb) 
```

于是，在windows本地试一下：

```
(lldb) platform select remote-android
  Platform: remote-android
 Connected: no
(lldb) platform connect connect://127.0.0.1:1234
error: Invalid URL:
(lldb) platform connect connect://192.168.22.36:1234
error: Invalid URL:
(lldb) platform connect connect://localhost:1234
  Platform: remote-android
    Triple: aarch64-unknown-linux-android
OS Version: 30 (5.4.86-qgki-debug-g40acb22f741e)
  Hostname: localhost
 Connected: yes
WorkingDir: /data/local/tmp
    Kernel: #1 SMP PREEMPT Mon May 4 16:37:05 CST 2026
(lldb)
```

发现remote-android时，必须要写成localhost，不能用IP。那么是不是只要加入/etc/hosts就好了呢？

```
cat /etc/hosts
127.0.0.1       localhost
192.168.22.36 remotehost    <----插入

然后ping remotehost，确认已生效。然后：
(lldb) platform connect connect://remotehost:1234
error: Failed to connect port
```

可见不行。也就是，如果选择remote-android，只有 connect://localhost:1234一种写法。

localhost是ubuntu本机，我要把数据发到windows上，但是又必须写localhost。简单，ubuntu上监听1234端口，把数据转发到windows上的1234端口不就行了？方案很多，AI提供了一个socat命令TCP 转发据说广泛使用、以及Windows上ncat.exe命令等。对我来说，容易想到的是ssh隧道。

```
ss -ltp | grep 1234 // 确认ubuntu上1234端口没有被占用
//在windows上执行。如果windows上安装了ssh server,那么也可以从ubuntu侧登陆windows执行ssh -L。
ssh -vv -R 1234:localhost:1234 xuexiangyu@192.168.20.111    //-vv输出详细日志，通过日志确认forwarding成功
ss -ltp | grep 1234 // 确认ubuntu上此时已经监听了1234端口
```

再试：

```
(lldb) platform select remote-android
  Platform: remote-android
 Connected: no
(lldb) platform connect connect://127.0.0.1:1234
error: Failed to connect port
(lldb) platform connect connect://localhost:1234
error: Failed to connect port
```

这就不应该了。想到或许我应该用更通用和简单的remote-linux试一下：

```
xuexiangyu@S111-CCS2plus:~/workspace/test$ lldb-18
(lldb) platform select remote-linux
  Platform: remote-linux
 Connected: no
(lldb) platform connect connect://localhost:1234
error: Connection shut down by remote side while waiting for reply to initial handshake packet
(lldb) 
```

结合ssh的日志，可以认定隧道是没问题的。刚才用remote-android时，都没日志，说明remote-android就是不行。remote-linux可行，只要解决这个报错。

ssh日志有这一行：

```
debug1: connect_next: host localhost ([::1]:1234) in progress, fd=7
```

::1是IP v6的环回地址，相当于IP v4中的127.0.0.1。localhost解析成了::1, 猜测和这个有关。于是ssh命令改成：

```
ssh -vv -R 1234:127.0.0.1:1234 xuexiangyu@192.168.20.111
```

果然成功了：

```
(lldb) platform select remote-android
  Platform: remote-android
 Connected: no
(lldb) platform connect connect://localhost:1234
error: Failed to connect port

(lldb) platform select remote-linux
  Platform: remote-linux
 Connected: no
(lldb) platform connect connect://localhost:1234
  Platform: remote-linux
    Triple: aarch64-unknown-linux-android
OS Version: 5.4.86 (5.4.86-qgki-debug-g40acb22f741e)
  Hostname: localhost
 Connected: yes
WorkingDir: /data/local/tmp
    Kernel: #1 SMP PREEMPT Mon May 4 16:37:05 CST 2026
```

不要忘了还有一个--gdbserver-port端口：

```shell
# 安卓设备shell执行。明确端口号
./lldb-server platform --listen *:1234 --server --gdbserver-port 10000

# windows 端口转发给安卓端口
adb forward tcp:1234 tcp:1234
adb forward tcp:10000 tcp:10000  
# 远程ubuntu端口转发给windows端口
ssh -vv -R 1234:127.0.0.1:1234 xuexiangyu@192.168.20.111
ssh -vv -R 1234:127.0.0.1:10000 xuexiangyu@192.168.20.10000

# 远程执行lldb
(lldb) platform select remote-linux
(lldb) platform connect connect://localhost:1234
(lldb) target create /xxx/out/target/product/msmnile_au/symbols/data/nativetest64/vendor/securitychip_hal_ut/securitychip_hal_ut
(lldb) run
---------通过------------
```

虽然结论是用不了remote-android,但remote-linux一样完美完成任务。组件remote-android内部不知道干了什么，初衷肯定是为了方便，但一些场景下反而实则丧失了灵活，不得不回到remote-linux。到这里，最终解决了问题。 

最后，问一下，我们都用remote-linux了，remote-linux是支持直接`platform connect connect://192.168.22.36:1234`的啊。

所以那个ssh隧道搞了个寂寞。

## 优化：解决warning

上面贴的日志，存在很多warning：

```
warning: (aarch64) C:\Users\xuexiangyu\.lldb\module_cache\remote-linux\.cache\3E00453D-93C2-63F9-F1DC-D178257A131A\libion.so No LZMA support found for reading .gnu_debugdata section
warning: (aarch64) C:\Users\xuexiangyu\.lldb\module_cache\remote-linux\.cache\316B3120-5B8A-84EF-BC6D-1492C06AEEB8\libdl.so No LZMA support found for reading .gnu_debugdata section
```

这个警告是因为正在调试的target, 依赖的其它系统库，没有符号表。image list查看

```
(lldb) image list
[  0] E8093660-6142-3E5F-B693-FA4DBAFE6004-C0ECFA4B 0x0000000000000000 /workspace/xuexiangyu/.lldb/module_cache/remote-linux/.cache/E8093660-6142-3E5F-B693-FA4DBAFE6004-C0ECFA4B/securitychip_pkcs11_ut 
[  1] 661D4366-5D5E-C814-EC19-1E5D951FE16A 0x0000000000000000 /workspace/xuexiangyu/.lldb/module_cache/remote-linux/.cache/661D4366-5D5E-C814-EC19-1E5D951FE16A/liblog.so 
[  2] 3E00453D-93C2-63F9-F1DC-D178257A131A 0x0000000000000000 /workspace/xuexiangyu/.lldb/module_cache/remote-linux/.cache/3E00453D-93C2-63F9-F1DC-D178257A131A/libion.so 
[  3] 0258740B-928B-138C-E564-C516FD6B9141 0x0000000000000000 /workspace/xuexiangyu/.lldb/module_cache/remote-linux/.cache/0258740B-928B-138C-E564-C516FD6B9141/libc++.so 
[  4] C91278A0-FE27-7D0C-DEA8-43D7EEF3233A 0x0000000000000000 /workspace/xuexiangyu/.lldb/module_cache/remote-
....
```

* target list：列出当前 LLDB 中存在的 Target（调试目标）。
* image list：列出某个 Target 已加载的 Image（模块，如可执行文件、.so、.dylib、.dll）。

我们只调试自己的代码，完全可以无视这个警告。但是这个模块，涉及的依赖太多，lldb会自动把他们从机器同步到电脑缓存目录，带来一些性能问题。时间久了还会在.cache目录下产生大量冗余。

解决这个问题，需要告诉调试工具，机器上的某个so,对应在本地PC上的什么位置。

| 设置                             | 查找对象                   | 典型文件                               | 作用                                   | 常见远程调试用途                                   |
| -------------------------------- | -------------------------- | -------------------------------------- | -------------------------------------- | -------------------------------------------------- |
| `target.sysroot`                 | **目标系统根目录映射**     | `/lib/*.so`、`/usr/lib/*.so`、头文件等 | 将远程设备的 `/` 映射到本地 SDK/rootfs | 指向 Android rootfs、嵌入式 Linux sysroot          |
| `target.exec-search-paths`       | **可执行文件和动态库本体** | ELF executable、`.so`、`.dylib`        | 找实际加载的二进制模块                 | 避免从远程下载 `.so`，优先使用本地 .so、可执行文件 |
| `target.debug-file-search-paths` | **独立调试符号文件**       | `.debug`、`.dwo`、DWARF 文件           | 找源码级调试信息                       | 使用独立符号包获得源码、变量、类型信息             |
| `symbols.enable-external-lookup` | **外部符号文件查找开关**   | `.debug`、`.dSYM`、Build-ID 符号       | 通过 Symbol Vendor 或平台机制查找符号  | 启用额外的符号查找机制                             |

- **最高优先级：`target.sysroot`**
  - **作用**：为整个远程文件系统指定一个本地根目录。
  - **查找逻辑**：LLDB **会优先使用** `sysroot` 下的文件，而不是本地文件系统上同名路径的文件。这是由设计决定的，确保了调试环境与远程目标一致。
- **次级搜索路径：`target.exec-search-paths` 等**
  - **作用**：指定一些额外的、具体的目录来搜索可执行文件和符号。
  - **查找逻辑**：当 `sysroot` 中找不到文件时，LLDB 会转而到 `target.exec-search-paths` 和 `target.debug-file-search-paths` 指定的路径中去寻找。

指令：

```
(lldb) platform select remote-linux --sysroot /home/sdk/rootfs   # 标准
(lldb) （错误）settings set target.sysroot /path/to/your/local/sysroot   # target.sysroot是平台（如 remote-linux）实现的一部分，而不是一个全局的 target 设置。
(lldb) （错误）settings show target.sysroot
(lldb) platform status       #应该通过platform status查看Sysroot

(lldb) settings set target.exec-search-paths /home/sdk/rootfs/lib1 /home/sdk/rootfs/lib2
(lldb) settings append target.exec-search-paths /home/sdk/rootfs/lib3
(lldb) settings show target.exec-search-paths
(lldb) settings set target.debug-file-search-paths /home/sdk/debug
(lldb) settings set target.symbols.enable-external-lookup true
```

以我目前手上的安卓模块为例：

```
platform select remote-linux --sysroot  /workspace/xuexiangyu/workspace/24mm_t2/apps/LINUX/android/out/target/product/msmnile_au/symbols
platform connect connect://192.168.22.36:1234 
platform settings -w /data/local/tmp
settings set target.exec-search-paths /xxx/out/target/product/msmnile_au/symbols/system/lib64 /xxx/out/target/product/msmnile_au/symbols/vendor/lib64
target create /xxx/out/target/product/msmnile_au/symbols/data/nativetest64/vendor/securitychip_hal_ut/securitychip_hal_ut
run
```

安卓so的路径比较分散，你还可以考虑加上这些：./system/lib64/bootstrap/、./apex/com.android.vndk.v32/lib64/。这是我根据实际项目检索到的。但不要忧虑遗漏，因为即使不设定target.exec-search-paths都没问题。

# attach模式

安卓开发hal服务，调试更多的是attach到已运行的进程上，而不是通过run/process launch启动。

```
(lldb) process attach --pid <PID>
(lldb) attach -p <PID>  //简写形式

(lldb) process attach --name <进程名>
(lldb) attach -n <进程名>  //简写形式
```

在 LLDB 的 attach 模式下，不需要run指令，但target create 命令通常还是建议使用的。因为target create 的核心作用是加载调试符号（symbols）。target create 可以为你提供有符号调试（Symbolic Debugging），不执行它则只能进行无符号调试（Non-Symbolic Debugging）。如果正在运行的程序本身就是带符号的，那么我想不指定target create应该关系不大（以实际体验为准）。

# IDE 中调试

本地调试调试比较简单。主要是安卓远程调试如何进行。

vscode中使用lldb调试需要通过插件。参见《基于cmake编译安卓native程序》中”IDE导入和调试“一节。这里简要说一下核心部分。

* 配置环境变量，让插件能够找到lldb、lldb-server等必要工具。

* 安装插件CodeLLDB，第一次使用插件会自动下载lldb工具，速度很慢。配了环境变量应该就不会了。这点我没有确认。

* 创建launch.json文件，最简内容：

  ```
  {
      "name": "随便起名",
      "type": "lldb",
      "request": "attach",   // 或 launch
      "program": "xxx",    // created target xxx
      "preLaunchTask": "yourTask", // 可选，debug前，先执行的vscode task, 实现调试前的准备工作
      "initCommands": [
          // 这里面写lldb指令。参考前面部分。
          "platform select remote-linux --sysroot  /workspace/xuexiangyu/workspace/24mm_t2/apps/LINUX/android/out/target/product/msmnile_au/symbols",
          ....
          // process launch 或 attach pid不需要，插件会根据"request": "attach"自动帮我们完成
      ],
      "stopOnEntry": false  // true在程序入口处暂停
  }
  ```

  虽插件支持initCommands、targetCreateCommands、preRunCommands、processCreateCommands、postRunCommands等，但大部分时候，我们全写在initCommands里就可以了。顶多加一个exitCommands调试结束处理释放资源。我们不是在打磨工艺品，所以能用就可以了。

* 其它控制参数，请阅读codelldb用户手册： https://github.com/vadimcn/codelldb/blob/v1.9.1/MANUAL.md 。

* 调试不依赖C++插件、cmake插件等，它们不相干。只要装一个CodeLLDB。如果你安装的是其它调试插件，launch.json语法会有差异，但底层原理都是gdb或lldb，举一反三不难。

# 日志

```
(lldb)log enable gdb-remote packets
(lldb)log enable lldb platform
```

打开日志有利于指令失败时分析问题。

# 常用指令

> 查询状态：
> platform status
> platform list
> platform process list
> platform shell  shell命令        # platform shell pwd是在server端工作目录执行pwd
> shell 命令                                  # shell pwd是在client端当前目录执行pwd
> settings list target
> settings show symbols
> settings show target        #只列出 target 命名空间下的通用设置项，比如target.sysroot就不在这个列表中
> 
>
> 连接
> adb forward tcp:1234 tcp:1234
> lldb-server platform --listen *:1234 --server
> platform select remote-linux
> platform select remote-android
> platform connect connect://192.168.22.36:1234
> platform connect connect://localhost:1234
> platform disconnect
> platform settings -w /data/local/tmp
> target create /path/xxx
> file /path/xxx
> target list
> image list                        #当前target加载的其它 ELF 模块。
> platform shell adb forward tcp:1234 tcp:1234
> settings set target.source-map /path1 /path2
> settings show target.source-map
> settings clear target.source-map
>
> run
> process launch
> attach -p <PID>
> attach -n <进程名>
>
>  image lookup -vn main
>
> 调试：
> b = breakpoint 设置断点
> c = continue 继续运行
> n = next 下一行
> s = step 单步进入
> f = finish 跳出
> p [var] 打印变量值
> var 显示所有局部变量
> bt 打印调用栈
> up 在调用栈中向上移一帧 older
> down 在调用栈中下移一帧 newer
> register 查看寄存器 memory 查看内存
>
> 

# 环境

windows从官网下载安装llvm

ubuntu可以通过apt install安装。直接sudo apt install lldb可能会装一个版本较低的lldb。注意判断一下。你可能需要：`sudo apt install lldb-18`。数字换成ubuntu支持的最大版本。`sudo apt update; apt-cache search lldb | grep ^lldb`查询,或直接按tab键。

调试安卓的话，通过android studio下载NDK。从NDK中找到lldb-server,push到安卓机器。

还要注意系统安装的python对lldb的污染。

# 参考

https://www.hidka.com/zh/blog/lldb-server/ LLDB 远程调试

https://juejin.cn/post/7139773823116640263 使用lldb调试Android native源码

https://lldb.llvm.org/use/remote.html  官网文档

https://github.com/vadimcn/codelldb/blob/v1.9.1/MANUAL.md codelldb 文档



