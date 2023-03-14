aosp源码编译后，会生成simg2img、lpunpack等工具（可以用which lpunpack找到路径，一般在out/soong/host/linux-x86/bin/或out/host/linux-x86/bin下面）

1. 将 super.img 从 Android sparse image 转换为 raw image

```shell
simg2img super.img super.img_ext4
```

2. 从 raw image 解包出分区镜像文件  
命令：lpunpack [-p partition_name] SUPER_IMAGE [OUTPPUT_DIR]  
下面以 sysetm 分区镜像文件为例
```shell
mkdir system
lpunpack -p system super.img_ext4 system(末尾的这个system是路径)
```
system.img 镜像会解到system目录里面。 

3. ubuntu系统的话，右击system.img文件，选择“用磁盘映像挂载器打开”即可。
