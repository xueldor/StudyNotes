## 概念

NFS就是Network File System的缩写，它的功能是把网络上的文件挂载到本地文件系统，就像访问本地文件一样。

## 工作原理

![img](_img/打开nfs/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3FxXzQxOTU5ODk5,size_16,color_FFFFFF,t_70-16859465407812.png)

## 基于Ubuntu18搭建

两台ubuntu系统，第一台的IP是`192.168.40.129`, 作为nfs服务器端；另外一台IP是`192.168.40.128`, 作为客户端。

### server端配置

server端安装nfs-kernel-server：

```shell
sudo apt-get install nfs-kernel-server
```

服务器端，创建`/etc/exports`文件，写入内容：

```shell
/home/xue/nfstest 192.168.40.128(rw,sync,no_root_squash,no_subtree_check)
```

意思是：把`/home/xue/nfstest`这个目录，共享给IP是192.168.40.128的机器，权限是rw。也就是ip：128的机器可以看到nfstest目录，可以在里面读、写文件。

括号里的几个属性解释一下：

* rw：此选项允许客户端计算机读取以及对卷的写入权限。
* sync：它强制NFS在回复之前将更改写入磁盘，从而产生更稳定和一致的环境。这主要是因为回复复制了远程卷的实际状态。
* nosubtreecheck：此选项可以避免子树检查，这是一个强制主机检查文件是否在每个请求的导出树中实际可用的过程。在客户端打开文件时重命名文件时可能会产生问题。出于同样的原因，在大多数情况下，建议禁用子树检查。
* norootsquash：默认情况下，NFS将来自root用户的请求远程转换为服务器上的非特权请求。这是一种安全功能，不允许客户端上的root帐户以root用户身份使用主机的文件系统。这种指令禁用了一定数量的共享。

### client端配置

client端安装nfs-common:

```shell
sudo apt-get install nfs-common

将会同时安装下列软件：
  keyutils libnfsidmap2 libtirpc1 rpcbind
```

(高版本的ubuntu可能安装`nfs-utils`)

然后把远程目录挂载到本地：

```shell
sudo mount -t nfs 192.168.40.129:/home/xue/nfstest /mnt/nfs/nfstest
```

如果没有安装nfs-common包，mount命令将不会支持`-t nfs`选项。另外，如果远程主机的nfs服务没有配置好的话，mount也是不能成功的。



测试：

1. server端nfstest目录新建a.txt、b.txt两个文件：

   ```shell
   xue@xue-virtual-machine:~/nfstest$ ls -l
   总用量 8
   -rw-r--r-- 1 xue xue 2 6月   5 14:43 a.txt
   -rw-r--r-- 1 xue xue 2 6月   5 14:43 b.txt
   ```

   

2. client端是否能看到、读取：

   ```shell
   xue@xue-virtual-machine:/mnt/nfs/nfstest$ ls -l
   总用量 8
   -rw-r--r-- 1 xue xue 2 6月   5 14:43 a.txt
   -rw-r--r-- 1 xue xue 2 6月   5 14:43 b.txt
   
   xue@xue-virtual-machine:/mnt/nfs/nfstest$ cat a.txt 
   1
   ```

3. 测试client端是否有写权限：

   ```shell
   rtual-machine:/mnt/nfs/nfstest$ touch c.txt
   xue@xue-virtual-machine:/mnt/nfs/nfstest$ echo 1 > c.txt
   ```

   server执行：

   ```shell
   xue@xue-virtual-machine:~/nfstest$ ls -l
   总用量 12
   -rw-r--r-- 1 xue xue 2 6月   5 14:43 a.txt
   -rw-r--r-- 1 xue xue 2 6月   5 14:43 b.txt
   -rw-r--r-- 1 xue xue 2 6月   5 15:39 c.txt
   xue@xue-virtual-machine:~/nfstest$ cat c.txt 
   1
   ```



## QNX打开nfs服务

在PC上，通过在线安装nfs的软件包提供服务， QNX显然不能。但是qnx的源码里已经有了相关的几个程序。

依赖两个可执行文件：nfsd、rpcbind。

rpcbind服务是将RPC程序/版本号映射为TCP和UDP端口号。仅当rpcbind在服务器上运行时，客户端才能进行RPC调用。nfsd是nfs守护程序。

我们在CCU的板子上开发，先用pidin确认两个服务是否存在。对于CCU项目，这两个文件是没有打包进系统的，需要手动上传到系统bin目录。文件路径：

```
qnx700/target/qnx7/aarch64le/usr/sbin/nfsd
qnx700/target/qnx7/aarch64le/usr/sbin/rpcbind
```

另外两个也一起添加进来(非必须，有助于调试)：

```shell
qnx700/target/qnx7/aarch64le/usr/bin/showmount
qnx700/target/qnx7/aarch64le/usr/bin/rpcinfo
```

前面两个是`/usr/sbin`,后面两个是`/usr/bin`,保持路径不变。

拖到车机里后执行`chmod +x nfsd`赋予可执行权限。四个文件都要。

### 配置

rpcbind服务的运行需要用到`/etc/netconfig`网络配置文件;

nfsd需要读取/etc/exports文件。

前面在ubuntu上面开通nfs的时候，是安装的工具包自动帮我们把一些配置文件弄好了。在qnx上面，需要我们自己来写。

`/etc/netconfig`:

```shell
#
# The network configuration file. This file is currently only used in
# conjunction with the TI-RPC code in the libtirpc library.
#
# Entries consist of:
#
#       <network_id> <semantics> <flags> <protofamily> <protoname> \
#               <device> <nametoaddr_libs>
#
# The <device> and <nametoaddr_libs> fields are always empty in this
# implementation.
#
udp        tpi_clts      v     inet     udp     -       -
tcp        tpi_cots_ord  v     inet     tcp     -       -
udp6       tpi_clts      v     inet6    udp     -       -
tcp6       tpi_cots_ord  v     inet6    tcp     -       -
rawip      tpi_raw       -     inet      -      -       -
local      tpi_cots_ord  -     loopback  -      -       -
unix       tpi_cots_ord  -     loopback  -      -       -
```

该网络配置文件大同小异，我直接从ubuntu上面粘贴过去。



在`/etc/services`中添加两行：

```shell
sunrpc		111/tcp		portmapper	# RPC 4.0 portmapper
sunrpc		111/udp		portmapper
```

CCU已经有这两行，所以不需要加。

官网（http://www.qnx.com/developers/docs/7.0.0/#com.qnx.doc.neutrino.utilities/topic/r/rpcbind.html）要求添加:

```shell
sunrpc          111/tcp         rpcbind portmap
sunrpc          111/udp         rpcbind portmap
```

实测好像差别不大，什么意思也不清楚。



`/etc/exports`添加：

```shell
/usr/testnfs 192.168.1.3
```

192.168.1.3是这台车机android的ip地址。testnfs是自己新建的。

重启，先后运行rpcbind和nfsd：

```shell
# rpcbind -L 
rpcbind: Cannot bind `local': Permission denied
rpcbind: Failed to open /var/run/rpcbind.sock: Permission denied
# 
# nfsd -t
```

检查：

```shell
# rpcinfo -p 192.168.1.1
   program vers proto   port  service
    100000    4   tcp    111
    100000    3   tcp    111
    100000    2   tcp    111
    100000    4   udp    111
    100000    3   udp    111
    100000    2   udp    111
    100005    1   udp   2049
    100003    2   udp   2049
    100005    3   udp   2049
    100003    3   udp   2049
    100005    1   tcp   2049
    100003    2   tcp   2049
    100005    3   tcp   2049
    100003    3   tcp   2049

# showmount -e 192.168.1.1
Exports list on 192.168.1.1:
/usr/testnfs                       192.168.1.3 
```

从命令看，qnx端的nfs服务已经开通。下面是android端挂载：

```shell
msmnile_gvmq:/mnt # mount -t nfs 192.168.1.1:/usr/testnfs /mnt/nfs
mount: '192.168.1.2:/usr/testnfs'->'/mnt/nfs': No such device

msmnile_gvmq:/ $ cat /proc/filesystems | grep nfs
nodev   functionfs
```

所以CCU的android不支持nfs文件系统。到内核源码看一下：

```shell
CCU@S052-Code:~/ccu_aosp_dev/lagvm/LINUX/android/kernel/msm-5.4/fs$ ls -l | grep nfs
drwxrwxr-x 2 CCU CCU   4096 10月 19  2022 kernfs
drwxrwxr-x 5 CCU CCU   4096 10月 19  2022 nfs
drwxrwxr-x 2 CCU CCU   4096 10月 19  2022 nfs_common
drwxrwxr-x 2 CCU CCU   4096 10月 19  2022 nfsd
```

nfs是客户端，nfsd是服务端。所以代码里是有的，推测可能是内核的开关没有打开。打开nfs的Makefile:

```makefile
# SPDX-License-Identifier: GPL-2.0
# 
# Makefile for the Linux nfs filesystem routines.
#

obj-$(CONFIG_NFS_FS) += nfs.o

CFLAGS_nfstrace.o += -I$(src)
nfs-y                   := client.o dir.o file.o getroot.o inode.o super.o \
                           io.o direct.o pagelist.o read.o symlink.o unlink.o \
                           write.o namespace.o mount_clnt.o nfstrace.o \
                           export.o sysfs.o
nfs-$(CONFIG_ROOT_NFS)  += nfsroot.o
nfs-$(CONFIG_SYSCTL)    += sysctl.o
nfs-$(CONFIG_NFS_FSCACHE) += fscache.o fscache-index.o
...
```

进入`~/ccu_aosp_dev/lagvm/LINUX/android/kernel/msm-5.4/arch/arm64/configs/vendor`, 找到gen3auto_debug.config、gen3auto_GKI.config、gen3auto_QGKI.config三个文件，在任意一个里面把CONFIG_NFS_FS加进去：

```
CONFIG_EDAC_KRYO_ARM64_PANIC_ON_CE=y
CONFIG_AUDITSYSCALL=y
CONFIG_NL80211_TESTMODE=y
CONFIG_NFS_FS=y       <--------我新增的
CONFIG_NFSD=y         <--------我新增的
```

CONFIG_NFSD是后面把安卓作为server端才要加的。这里顺便一起完事。

重新编译后mount还是不行。有待分析...

## 后续思路

1. 在PC上挂载QNX的NFS目录，验证QNX端服务开发正确。

   --------因为没法把QNX和Ubuntu搞到一个局域网里，所以不好做。

2. 推测还是config文件没有配置好，但是应该怎么弄没头绪，也不知道从哪找相关资料；网上找到一个`fuse-nfs`的开源项目，借助fuse绕过在内核里做修改。后面可以考虑试一下把它移植到安卓里面来，用它来挂载nfs。

