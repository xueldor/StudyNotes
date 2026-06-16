缺少GLIBC_2.28的问题，说明本地的libc6版本太低。ubuntu18系统里的glibc是2.27, 很多软件已经要求更新的版本了。

``` shell
#通过这个命令，看到本地的最高版本是GLIBC_2.27
strings /lib/x86_64-linux-gnu/libc.so.6 | grep GLIBC_ 

#不出意外会提示最新版本就是2.27
sudo apt update
sudo apt upgrade libc6

```

下面先介绍怎么升级glibc，然后说明为什么不能直接升级，以及正确的方式。

从官网下载http://ftp.gnu.org/gnu/glibc/glibc-2.28.tar.gz

```
cd glibc-2.28/
mkdir build
cd build/
../configure --prefix=/opt/glibc-2.28
make -j8
sudo make install
```

这会安装到/opt/glibc-2.28。
1、如果过程中提示缺gcc gawk bison,安装即可
2、如果报错：glibc-2.28/build/elf/ldconfig: Warning: ignoring configuration file that cannot be opened: /opt/glibc-2.28/etc/ld.so.conf: No such file or directory
sudo cp /etc/ld.so.conf /opt/glibc-2.28/etc/ 即可。

注意：用编译的高版本替换原来的2.27版本，是不可能的，直接替换/lib/x86_64-linux-gnu/下的文件，会导致系统挂掉。添加环境变量也不行。
原因是，glibc相当底层，系统的诸多程序和命令都依赖它。升级了glibc就要一起升级整个系统。直接替换系统原来的glibc，而ls、echo、rm的基本命令没有一起升级，这些命令需要的是旧版本glibc，导致这些命令都不能使用，实际系统起不来了。

也就是说，原系统的大部分程序还是依赖glibc2.27,你不能为了个别软件，升级glibc。

` /etc/ld.so.conf`  和` /etc/ld.so.conf.d/*.conf`文件里定义了系统加载so库的优先级，千万不要让/opt/glibc-2.28这个路径被系统加载，一旦开机用的是/opt/glibc-2.28里的高版本glibc，直接开机起不来。

正确方式是：

* 一定要指定安装路径，即上面的../configure --prefix=/opt/glibc-2.28 ，千万不要覆盖系统原来的glibc。

* glibc-2.28只提供给个别依赖高版本glibc的程序，方法是：

  * 打开一个shell，临时修改环境变量：

    ```
     $ export LD_LIBRARY_PATH=/opt/glibc-2.28/lib:$LD_LIBRARY_PATH
     $ ./myExe # 这样myExe使用 GLIBC_2.28，不影响系统
     或者
     $ LD_PRELOAD=/opt/glibc-2.28/lib/libc.so.6 yourExe #注意中间空格
    ```

    这样修改过的LD_LIBRARY_PATH只对这个shell可见。其实这还是有风险的，因为你的软件可能会调用了系统的指令，而这个指令依赖glibc2.27。那么你的软件一样会挂掉。

    总而言之，难点是，我们没法精细的、按照需要，一些程序用旧版glibc，一些用新版glibc，互相又不兼容。

* 惟一绝对可靠的方法是，直接升级系统，重新部署环境。前提是，电脑是自己的。公司服务器显然是不允许这样做。

* 用docker吧。宿主机ubuntu18,docker里面ubuntu20。完全没问题。

如果已经挂掉，试参考《ubuntu命令行升级系统版本》




最后再强调注意事项（万分重要）：
千万不要覆盖系统默认的 glibc（如 /lib, /lib64 中的 libc.so），这会导致系统无法启动，无法恢复，只能重装系统。风险极高。

最好的方案是，直接升级系统到ubuntu20. 如何升级见《ubuntu命令行升级系统版本》。