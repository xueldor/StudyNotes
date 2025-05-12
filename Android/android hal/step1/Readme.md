首先完成再android的内核中新增一个驱动，确保能编译进内核，且功能正常运行。

基于ccs2.0_plus_qm项目。

1. 将freg文件夹放到`<kernel_source>/drivers/`目录。

2. freg目录的上一级，即drivers目录，找到`drivers/Kconfig`，新增一行:

   ```Kconfig
   source "drivers/freg/Kconfig"
   ```

3. drivers/Makefile新增一行：

   ```makefile
   obj-$(CONFIG_FREG)		+= freg/
   ```

   这样编内核才能找到新增模块。

4. CONFIG_FREG的定义放到android_car_defconfig中

   ```txt
   cd到kernel根目录。
   vi ./arch/arm64/configs/android_car_defconfig
   添加CONFIG_FREG=y
   ```

   obj-y表示freg编译到内核中，obj-m表示freg以模块的方式来编译。

5. 参考供应商(NXP)提供的编译手册，执行以下命令

```shell
export MY_ANDROID=~/xue/plus_qm
export ARCH=arm64
export CROSS_COMPILE=$MY_ANDROID/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin/aarch64-linux-android-（就是“aarch64-linux-android-”，后面不缺）

cd ${MY_ANDROID}/vendor/nxp-opensource/kernel_imx
make android_car_defconfig
make KCFLAGS=-mno-android
```

这是构建kernel Image的命令。如果代码没问题，界面输出应该能看到生成freg.o。如果编译有问题，根据打印消息修改代码。

6. 我们目的是构建boot.img刷到板子上，上一步不是必要的，直接在android源码目录执行`make bootimage`即可。当然之前要确保环境配置好，并执行source build/envsetup&lunch 35。

7. 用供应商给的刷机工具，把boot.img烧录到板子上。不过烧录比较麻烦，可以用下面命令：

   ```shell
   dd if=/sdcard/boot.img of=/dev/block/by-name/boot_a
   dd if=/sdcard/boot.img of=/dev/block/by-name/boot_b
   ```

   因为烧录镜像的实质，也不过是把img文件覆盖到boot分区而已。

8. 重启，开机时迅速执行：

   ```shell
   mek_8q:/ $ dmesg | grep freg
   [    0.892089] Initializing freg device.
   ```

   "Initializing freg device"是我代码里printk打印的日志。说明模块已经编译进了内核。

   也许是dmesg的缓冲有限，也许是开机后清空了dmesg，总之要在开机启动，串口刚一识别，就不停的执行dmesg，否则可能看不到日志。

9. 代码中，我为了让代码简单易懂，没有在/dev目录创建freg设备文件。所以现在要用命令手动创建：

   ```shell
   mek_8q:/ # cat /proc/devices | grep freg
   242 freg
   
   mek_8q:/ # mknod /dev/freg c 242 0
   ```

10.  用cat和echo命令验证

    ```shell
    mek_8q:/ # echo qwewrewt > /dev/freg
    mek_8q:/ # 
    mek_8q:/ # cat /dev/freg
    qwew
    
    ```

    

11. 后面补充在/dev目录、/sys/class目录、/proc目录创建设备文件和相关属性文件的代码，使示例更加完整，也不再用mknod命令。



问题记录：

xshell执行make menuconfig，配置项菜单界面是不能正常显示的。要在linux桌面终端执行才行。不过我这里不需要执行make menuconfig。







