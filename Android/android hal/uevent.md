## uevent

Uevent是Kobject的一部分，用于在Kobject状态发生改变时，例如增加、移除等，通知用户空间程序。

该机制通常是用来支持热拔插设备的，例如U盘插入后，USB相关的驱动软件会动态创建用于表示该U盘的device结构（相应的也包括其中的kobject），并告知用户空间程序，为该U盘动态的创建/dev/目录下的设备节点，更进一步，可以通知其它的应用程序，将该U盘设备mount到系统中，从而动态的支持该设备。

Uevent模块准备好上报事件的格式后，可以通过两个途径把事件上报到用户空间：一种是通过kmod模块，直接调用用户空间的可执行文件；另一种是通过netlink通信机制，将事件从内核空间传递给用户空间。

## netlink

Netlink 是一种IPC机制，用于内核与用户空间通信。（也以用于进程间通信，但Netlink 更多用于内核通信，因为进程之间通信更多使用Unix域套接字）。

**异步全双工**     在一般情况下，用户态和内核态通信会使用传统的Ioctl、sysfs属性文件或者procfs属性文件，这3种通信方式都是同步通信方式，由用户态主动发起向内核态的通信，内核无法主动发起通信。而Netlink是一种异步全双工的通信方式，它支持由内核态主动发起通信。

用户态则基于socket  API，内核态则使用内核为Netlink通信提供的一组特殊的API接口。

## udev

 *udev*是linux kernel的设备管理器,用来代替以前的devfs、hotplug等功能。

```shell
xue@xue-virtual-machine:~$ ps -e | grep udev
   470 ?        00:00:00 systemd-udevd

#在这个Ubuntu上，systemd-udevd监听内核发出的设备事件,并根据udev规则处理每个事件
```

devfs和udev都是在/dev目录下创建设备文件，不同的是devfs的策略是将设备文件的创建过程放在了内核空间，而udev的策略是由内核提供机制而用户空间提供策略从而完成设备的创建。

内核通过netlink机制将设备添加过程的uevent发送到用户空间的udev程序。udev程序根据rules文件定义的策略处理事件执行后续动作。

我用来代码来说明devfs的策略将设备文件的创建过程放在了内核空间：

```c++
//udev出现前，某驱动的代码
//在模块加载函数中创建设备文件，在模块卸载函数中撤销设备文件
static int __init xxx_init(void){
    ...
    /*创建设备文件*/
    devfs_handle = devfs_register(NULL, DEVICE_NAME, DEVFS_FL_DEFAULT, XXX_MAJOR, 0, 						S_IFCHR | S_IRUSR | S_IWUSR, &xxx_fops, NULL); 
    ...
}
static int __exit xxx_exit(void){
    ...
    /*撤销设备文件*/
    devfs_unregister(devfs_handle);
    ...
}
```

devfs废弃后，不再出现devfs_register这样的函数。但是作为兼容，函数仍然是支持的。

许多嵌入式平台上没有udev，而是使用mdev，相当于一个简化版的udev。

Android也没有实现udev，而是用vold等其它机制代替它，见后文。不过busybox中自带mdev。

## ueventd

ueventd是安卓里面的一个服务，d是daemon，守护进程的意思。

```shell
mek_8q:/sys/class/gpio # ps -e | grep uevent
root          1179     1    6688   2300 poll_schedule_timeout 0 S ueventd
```

ueventd通过使用uevent，监控驱动发送的消息，做进一步处理。

ueventd进程和init进程是同一个可执行文件，只是执行时走了不同的分支

```c++
//根据命令行参数，成为ueventd、watchdogd、或init
if (!strcmp(basename(argv[0]), "ueventd"))
        return ueventd_main(argc, argv);
 
    if (!strcmp(basename(argv[0]), "watchdogd"))
        return watchdogd_main(argc, argv);

```

ueventd的整体代码比较简单，主要是三部分：

-  解析ueventd.rc
-  初始化设备信息
-  循环polling uevent消息

见“/system/core/init/ueventd.cpp”的代码。

**解析ueventd.rc**    ueventd.rc相比init.rc更简单，主要就是通过rc文件，来控制目录节点的权限。如：

```txt
/dev/ttyUSB2       0666   radio   radio  
/dev/ts0710mux*           0640   radio      radio  
/dev/ppp                  0666   radio      vpn  
  
# sysfs properties  
/sys/devices/virtual/input/input*   enable      0666  system   system  
/sys/devices/virtual/input/input*   poll_delay  0666  system   system  
```

**初始化设备信息**    kernel在加载设备时，会通过socket发送uevent事件给userspace， 在ueventd里，通过接受这些uevent事件，来创建设备的节点。主要函数是device_init()。

**uevent消息处理**    接受内核发的event消息，解析此消息，创建/删除设备节点，同时根据ueventd.rc中描述的权限设置更新一些节点权限。

## netd和vold

除了ueventd外，netd和vold也是使用uevent的。*Netd*是Android network属于偏底层的部分。*vold*的全称是volume daemon，是负责完成系统的CDROM, USB大容量存储，MMC卡等扩展存储的挂载任务自动完成的守护进程。

```shell
mek_8q:/sys/class/gpio # ps -e | grep -E 'netd|vold'                           
root          1564     1   28124   7088 binder_thread_read  0 S vold
root          1630     1   42164   7892 binder_thread_read  0 S netd
```

vold模块接收uevent的代码在NetlinkManager.cpp（system/vold/NetlinkManager.cpp）。

## sysfs

无论是udev还是vold,都是依赖sysfs的，因为是sysfs发出uevent的socket。所以有必要知道sysfs是什么。

sys文件系统、proc文件系统这些属于linux的基础知识，短篇幅也无法说清楚。简单的说，sysfs展示设备驱动模型中各组件的层级关系，它把连接在系统上的设备和总线组织成一个个分级的文件，产生一个包括所有系统硬件的分层式视图，可以由用户空间存取，向用户空间导出内核数据结构以及它们的属性。和proc文件系统十分相似，只是proc是提供进程和状态信息。顶层目录包括block、devices、bus、dev、class、power等。

- **block**: 　　　包含所有的块设备，如ram，sda等
- **bus**: 　　　　包含系统中所有的总线类型，如pci，usb，i2c等
- **class**: 　　　包含系统中的设备类型，如input，pci_bus，mmc_host等
- **dev**: 　　　　包含两个子目录：char和block，分别存放字符设备和块设备的主次设备号(major:minor)，指向 /sys/devices 目录下的设备
- **devices**:　　包含系统所有的设备

sysfs中显示的每一个对象都对应一个 kobject结构（完整定义位于 linux/kobject.h ，结构内部包含一个 parent 指针），而另一个相联系的结构为 kset 。 kset 是嵌入相同类型结构的 kobject 对象的集合。 内核用 kobject 、 kset 和 parent 之间的关系将各个对象连接起来组成一个分层的结构体系，从而与模型化的子系统相匹配。

## 总结

我们看到，虽然有许多机制，如udev、mdev、vold、ueventd等，但本质上都是利用内核uevent的，也就是他们的原理都是共通的。

## 参考

uevent的概念参考http://www.wowotech.net/linux_kenrel/uevent.html，原文中的其它关联概念亦参考该作者的文章系列。

Linux设备驱动模型和sysfs文件系统https://www.cnblogs.com/hueyxu/p/13659262.html