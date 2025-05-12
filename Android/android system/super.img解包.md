aosp源码编译后，会生成simg2img、lpunpack等工具。

1. 将 super.img 从 Android sparse image 转换为 raw image

```shell
simg2img super.img super.img_ext4
```

2. 用lpdump查看super.img_ext4里的信息：

  ```shell
  # 默认解析 Slot 0 的分区信息
  lpdump `pwd`/super.img_ext4
  
  # 解析 Slot 1 的分区信息
  lpdump -slot 1 `pwd`/super.img_ext4
  
  # 解析全部分区信息
  lpdump -a `pwd`/super.img_ext4
  ```

  这个命令很奇怪，要用绝对路径，这就是为什么我要加一个pwd。可是当我解析super_empty.img时，又支持相对路径了：

  ```shell
  lpdump super_empty.img
  ```

  输出的示例如下：

  ```shell
  work@work-WIND:/media/work/d/ccs5.0/apps/LINUX/android/out/target/product$ lpdump `pwd`/super.img_raw  -slot 1
  Slot 1:
  Metadata version: 10.0
  Metadata size: 592 bytes
  Metadata max size: 65536 bytes
  Metadata slot count: 3
  Header flags: none
  Partition table:
  ------------------------
    Name: system_a
    Group: qti_dynamic_partitions_a
    Attributes: readonly
    Extents:
      0 .. 2745319 linear super 2048
  ------------------------
    Name: system_b
    Group: qti_dynamic_partitions_b
    Attributes: readonly
    Extents:
  ------------------------
    Name: vendor_a
    Group: qti_dynamic_partitions_a
    Attributes: readonly
    Extents:
      0 .. 4290175 linear super 2748416
  ------------------------
    Name: vendor_b
    Group: qti_dynamic_partitions_b
    Attributes: readonly
    Extents:
  ------------------------
  Super partition layout:
  ------------------------
  super: 2048 .. 2747368: system_a (2745320 sectors)
  super: 2748416 .. 7038592: vendor_a (4290176 sectors)
  ------------------------
  Block device table:
  ------------------------
    Partition name: super
    First sector: 2048
    Size: 12884901888 bytes
    Flags: none
  ------------------------
  Group table:
  ------------------------
    Name: default
    Maximum size: 0 bytes
    Flags: none
  ------------------------
    Name: qti_dynamic_partitions_a
    Maximum size: 5314772992 bytes
    Flags: none
  ------------------------
    Name: qti_dynamic_partitions_b
    Maximum size: 5314772992 bytes
    Flags: none
  ------------------------
  ```

  

2. 从 raw image 解包出分区镜像文件  
命令：lpunpack [-p partition_name] SUPER_IMAGE [OUTPPUT_DIR]  
下面以 sysetm 分区镜像文件为例
```shell
mkdir system
lpunpack -p system_a super.img_ext4 system(末尾的这个system是路径)
```
-p后面是分区名，从上面lpdump可以看到，super里面包含system_a、system_b、vendor_a、vendor_b，我们提取system_a，在system目录得到system_a.img文件。

> lpunpack可能没有，需要自己编译出来：
>
> source build/envsetup.sh
>
> lunch xxx
>
> make lpunpack
>
> 编出来后再`out/host/linux-x86/bin下面`下面。其它命令也在这个路径。

3. ubuntu系统的话，右击system.img文件，选择“用磁盘映像挂载器打开”即可。（注意不是磁盘映像写入器）。
