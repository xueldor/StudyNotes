## **第一步**

在hardware/interfaces/目录下新建freg/1.0目录，在目录1.0下面创建文件`IFreg.hal`,代码写入：

```hal
package android.hardware.freg@1.0;

interface IFreg {
    getReg1() generates(uint64_t result);
};
```

shell里面：

```shell
source build/envsetup.sh
lunch 35
#试一下有没有hidl-gen命令，如果没有，则执行make hidl-gen
make hidl-gen
```

然后执行

```shell
hidl-gen -o hardware/interfaces/freg/output -Lc++-impl android.hardware.freg@1.0
```

可以看到-o指定的目录下多了`Freg.cpp`和`Freg.h`两个文件。

* 现在我们的命令先不加`-r`选项。代码必须放在hardware/interfaces/目录，package必须是`android.hardware.xx.xx.xx@x.x`的格式，并且``android.hardware`后面的路径和`hardware/interfaces/`里面的目录层级匹配，比如，假设hal文件里的第一行是`package android.hardware.freg.my@1.5;`,那么hal文件就必须放在`hardware/interfaces/freg/my/1.5`目录。注意最后的1.5表示版本，也要建一层目录。

* `-r`选项指定包名和路径，前面我们不加，就是默认的`android.hardware:hardware/interfaces`，表示包名前缀是android.hardware，路径放在hardware/interfaces里面。可以指定多个-r，例如：

  ```shell
  hidl-gen -O "" -Landroidbp   -r vendor.qcom.proprietary.hardware.interfaces.early_autocore:vendor/qcom/proprietary/hardware/interfaces/early_autocore   -r android.hidl:system/libhidl/transport   vendor.qcom.proprietary.hardware.interfaces.early_autocore.serial@1.0
  ```

  

* package包名后面一定要有一个`@x.x`,表示版本。并且x只能是数字，比如`@1.0`、`@2.5`、`@11.3`是合法的，而这些是非法的:`@V1.0`、`@1`、`@1.a`、`@1.0.0`。

通过看源码interfaces目录里的其它模块例子，以及网上的资料，我们发现一般把生成的C++代码放到1.0(或1.5、2.0等其它名称)里面的default目录里面,然后会在default里添加其它文件。所以先把output删除，在1.0目录下新建default目录，重新执行前面的命令。

然后执行

```shell
hidl-gen -o hardware/interfaces/freg/1.0/default -Landroidbp-impl android.hardware.freg@1.0
```

这个命令会在**default**目录下生成Android.bp。记住要放在default目录下面，因为下一步，我们会用`update-makefiles.sh`生成另外一个Android.bp,在1.0目录下面，所以两个文件不要放一起。

## **第二步**

如果这时执行mm，肯定会报错，因为Android.bp里的：

```bp
    shared_libs: [
        "libhidlbase",
        "libhidltransport",
        "libutils",
        "android.hardware.freg@1.0",
    ],
```

依赖"android.hardware.freg@1.0"，这是是什么？在哪里？生成的代码里，struct Freg是继承 public IFreg的，IFreg又是什么？在哪里？这些其实是谷歌帮我们定义好的套路，我们只要写一个hal文件，然后用工具生成一系列代码的骨架。然后我们只需关注逻辑实现。

在interfaces目录执行`update-makefiles.sh`：

```shell
work@S111-CCS2plus:~/xue/plus_qm/hardware/interfaces$ ./update-makefiles.sh
Updating makefiles for android.hardware in /home/work/xue/plus_qm/hardware/interfaces.
...
Updating android.hardware.freg@1.0  #<====这里
...
```

然后，外面一层目录生成了一个Android.bp，指定name为： name: "android.hardware.freg@1.0"，所以上面那个bp依赖"android.hardware.freg@1.0"，然后构建系统找到刚生成的这个bp，执行它生成一系列文件。



Freg.h里面的`#include <android/hardware/freg/1.0/IFreg.h>`这个头文件就是外面这个bp生成的，位于：

`out/soong/.intermediates/hardware/interfaces/freg/1.0/android.hardware.freg@1.0_genc++_headers/gen/android/hardware/freg/1.0/IFreg.h`。

其它生成的文件大概有这些：

>  ./out/soong/.intermediates/hardware/interfaces/freg/1.0/android.hardware.freg@1.0_genc++_headers/gen/android/hardware/freg/1.0/IFreg.h
> ./out/soong/.intermediates/hardware/interfaces/freg/1.0/android.hardware.freg@1.0_genc++_headers/gen/android/hardware/freg/1.0/IHwFreg.h
> ./out/soong/.intermediates/hardware/interfaces/freg/1.0/android.hardware.freg@1.0_genc++_headers/gen/android/hardware/freg/1.0/BpHwFreg.h
> ./out/soong/.intermediates/hardware/interfaces/freg/1.0/android.hardware.freg@1.0_genc++_headers/gen/android/hardware/freg/1.0/IFreg.h.d
> ./out/soong/.intermediates/hardware/interfaces/freg/1.0/android.hardware.freg@1.0_genc++_headers/gen/android/hardware/freg/1.0/BsFreg.h
> ./out/soong/.intermediates/hardware/interfaces/freg/1.0/android.hardware.freg@1.0_genc++_headers/gen/android/hardware/freg/1.0/BnHwFreg.h
> ./out/soong/.intermediates/hardware/interfaces/freg/1.0/android.hardware.freg@1.0_genc++/gen/android/hardware/freg/1.0/FregAll.cpp
> ./out/soong/.intermediates/hardware/interfaces/freg/1.0/android.hardware.freg@1.0_genc++/gen/android/hardware/freg/1.0/FregAll.cpp.d

除了代码，还有一些动态库、静态库。我们可以到目录下观察一下生成物，以便更好的理解。从名字上看，跟我们以前用aidl-cpp命令生成的文件很像，大概能猜到，它们是使用hwbinder通信的实现代码。

* `**IFreg.h**` - 描述 C++ 类中的纯 `IFreg` 接口；它包含 `IFreg.hal` 文件中的 `IFreg` 接口中所定义的方法和类型，必要时会转换为 C++ 类型。**不包含**与用于实现此接口的 RPC 机制（例如 `HwBinder`）相关的详细信息。类的命名空间包含软件包名称和版本号，例如 `::android::hardware::samples::IFreg::V1_0`。客户端和服务器都包含此标头：客户端用它来调用方法，服务器用它来实现这些方法。

* `**IHwFreg.h**` - 头文件，其中包含用于对接口中使用的数据类型进行序列化的函数的声明。开发者不得直接包含其标头（它不包含任何类）。

* `**BpHwFreg.h**` - 从 `IFreg` 继承的类，可描述接口的 `HwBinder` 代理（客户端）实现。开发者不得直接引用此类。

* `**BnHwFreg.h**` - 保存对 `IFreg` 实现的引用的类，可描述接口的 `HwBinder` 存根（服务器端）实现。开发者不得直接引用此类。

* `**FregAll.cpp**` - 包含 `HwBinder` 代理和 `HwBinder` 存根的实现的类。当客户端调用接口方法时，代理会自动从客户端封送参数，并将事务发送到绑定内核驱动程序，该内核驱动程序会将事务传送到另一端的存根（该存根随后会调用实际的服务器实现）。



我们这个例子采用Passthrough模式，因此，打开Freg.h,放开注释：

```c++
// FIXME: most likely delete, this is only for passthrough implementations
//extern "C" IFreg* HIDL_FETCH_IFreg(const char* name);
^-------把注释放开
```

Freg.cpp里面同理，放开这段：

```c++
IFreg* HIDL_FETCH_IFreg(const char* /* name */) {
    return new Freg();
}
```

getReg1函数的实现部分，我们暂且先让它直接返回一个数字，以后再完善跟驱动的交互逻辑：

```c++
// Methods from ::android::hardware::freg::V1_0::IFreg follow.
Return<uint64_t> Freg::getReg1() {
    // TODO implement
    return uint64_t(97);
}
```

然后，到`freg/1.0/default`目录执行mm，build成功后，留意两个目标产物：

* android.hardware.naruto@1.0-impl.so   freg模块的实现端，即binder server端，push到/vendor/lib64/hw/

* android.hardware.naruto@1.0.so  freg模块的调用端，即binder client端，push到 /system/lib64/，给需要调用freg的模块用。

对应目录在：

> ./target/product/mek_8q/symbols/vendor/lib64/hw/android.hardware.freg@1.0-impl.so
> ./target/product/mek_8q/symbols/vendor/lib/hw/android.hardware.freg@1.0-impl.so
> ./target/product/mek_8q/vendor/lib64/hw/android.hardware.freg@1.0-impl.so   <===64位版本
> ./target/product/mek_8q/vendor/lib/hw/android.hardware.freg@1.0-impl.so        <===32位版本
>
> ./target/product/mek_8q/system/lib64/vndk-28/android.hardware.freg@1.0.so
> ./target/product/mek_8q/system/lib64/android.hardware.freg@1.0.so       <===64位版本
> ./target/product/mek_8q/system/lib/vndk-28/android.hardware.freg@1.0.so
> ./target/product/mek_8q/system/lib/android.hardware.freg@1.0.so           <===32位版本

懂调试的应该知道那个symbols是用来debug的。



## 第三步

需要为binder的服务端写一个可执行程序。在default目录里面创建`service.cpp`，代码：

```c++
#define LOG_TAG "android.hardware.freg@1.0-service"

#include <android/hardware/freg/1.0/IFreg.h>
#include <hidl/LegacySupport.h>

using android::hardware::freg::V1_0::IFreg;
using android::hardware::defaultPassthroughServiceImplementation;

int main() {
    //直通（passthrough）模式
    return defaultPassthroughServiceImplementation<IFreg>();
}
```

这样就可以了，没有多余的代码。前面已经说过，这种方法(passthrough模式)需要暴露HIDL_FETCH_*接口，第二步已经暴露了。

编译脚本：

```bp
//文件default/Android.bp
//在原来的基础上追加
cc_binary {
    name: "android.hardware.freg@1.0-service",
    defaults: ["hidl_defaults"],
    proprietary: true,
    relative_install_path: "hw",
    srcs: ["service.cpp"],
    shared_libs: [
        "libhidlbase",
        "libhidltransport",
        "libutils",
        "liblog",
        "android.hardware.freg@1.0",
        "android.hardware.freg@1.0-impl",
    ],
}
```

我们知道这个bp文件是hidl-gen工具生成的，所以，万一哪次忘了，重新执行命令生成一下，哪么我们后加的这段就没了。所以，可以另建一个Android.mk, 用mk来构建 "android.hardware.freg@1.0-service"。这样互不干扰。

输出二进制：

```shell
work@S111-CCS2plus:~/xue/plus_qm/out$ find . -name android.hardware.freg@1.0-service
./target/product/mek_8q/symbols/vendor/bin/hw/android.hardware.freg@1.0-service
./target/product/mek_8q/vendor/bin/hw/android.hardware.freg@1.0-service
```

我们把“android.hardware.freg@1.0-service” push到“vendor/bin/hw”目录下。前面让留意的两个so，一个push到`/system/lib64/`，一个push到`/vendor/lib64/hw/`：

```shell
> adb push android.hardware.freg@1.0.so /system/lib64/
android.hardware.freg@1.0.so: 1 file pushed, 0 skipped. 76.5 MB/s (135432 bytes in 0.002s)

> adb push android.hardware.freg@1.0-impl.so /vendor/lib64/hw/
android.hardware.freg@1.0-impl.so: 1 file pushed, 0 skipped. 31.6 MB/s (68312 bytes in 0.002s)
```

两个so的32位版本也要push，对应目录为“/system/lib/”以及“/vendor/lib/hw/”。

然后，要修改/vendor/etc/vintf/manifest.xml，把这个hwbinder的实现注册到系统，这样getService()才能找到它。我现在先不加，看看会发生什么。

## 第四步 验证

1. 最后写一个demo验证调用getReg1。

```c++
# include <android/hardware/freg/1.0/IFreg.h>

# include <hidl/Status.h>
# include <hidl/LegacySupport.h>
# include <utils/misc.h>
# include <hidl/HidlSupport.h>

# include <stdio.h>

using android::hardware::freg::V1_0::IFreg;
using android::sp;
using android::hardware::hidl_string;

int main()
{
    android::sp<IFreg> service = IFreg::getService();
    if(service == nullptr) {
        printf("Failed to get service\n");
        return -1;
    }

    int val = service->getReg1();
    printf("val = %d\n", val);

    return 0;
}
```

代码随便放在哪。构建脚本：

```makefile
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_PROPRIETARY_MODULE := true
LOCAL_MODULE := freg_test
LOCAL_SRC_FILES := \
    TestFreg.cpp \

LOCAL_SHARED_LIBRARIES := \
   liblog \
   libhidlbase \
   libutils \
   android.hardware.freg@1.0 \

include $(BUILD_EXECUTABLE)
```

把生成的freg_test可执行文件push到“/data/local/tmp”，然后执行：

```shell
1|mek_8q:/data/local/tmp # ./freg_test                                         
Failed to get service
```

为什么IFreg::getService()是null？因为server端没启呀。前面我们已经把“android.hardware.freg@1.0-service” push到“vendor/bin/hw”目录下了，虽然这名字不太像可执行文件，但他就是可执行文件。我们在shell里执行

```shell
# ps -e | grep @
root          1550     1   13308   4192 binder_thread_read  0 S android.hardware.boot@1.0-service
system        1551     1   15552   5104 binder_thread_read  0 S android.hardware.keymaster@3.0-service
wifi          1635     1   19672   6928 binder_thread_read  0 S android.hardware.wifi@1.0-service
system        1642     1   12840   4032 binder_thread_read  0 S android.hidl.allocator@1.0-service
audioserver   1644     1   22696   9824 binder_thread_read  0 S android.hardware.audio@2.0-service
audioserver   1645     1   12864   4104 binder_thread_read  0 S android.hardware.automotive.audiocontrol@1.0-service
vehicle_network 1646   1   20832   4468 binder_thread_read  0 S android.hardware.automotive.vehicle@2.0-service
bluetooth     1647     1   13168   4200 binder_thread_read  0 S android.hardware.bluetooth@1.0-service
audioserver   1648     1   13540   4556 binder_thread_read  0 S android.hardware.broadcastradio@1.1-service
cameraserver  1649     1   29556   7140 binder_thread_read  0 S android.hardware.camera.provider@2.4-service
media         1653     1   10096   4664 binder_thread_read  0 S android.hardware.cas@1.0-service
system        1654     1   16712   4496 binder_thread_read  0 S android.hardware.configstore@1.1-service
media         1655     1   19600   6556 binder_thread_read  0 S android.hardware.drm@1.0-service
system        1656     1   17472   5308 binder_thread_read  0 S android.hardware.gatekeeper@1.0-service
system        1657     1   19172   4792 binder_thread_read  0 S android.hardware.graphics.allocator@2.0-service
system        1658     1   64660   8596 binder_thread_read  0 S android.hardware.graphics.composer@2.1-service
system        1659     1   13228   4368 SyS_epoll_wait      0 S android.hardware.health@2.0-service.imx
system        1660     1   13376   4192 binder_thread_read  0 S android.hardware.light@2.0-service
system        1661     1   13248   4116 binder_thread_read  0 S android.hardware.memtrack@1.0-service
system        1662     1   13380   4192 binder_thread_read  0 S android.hardware.power@1.0-service
system        1663     1   13312   4288 binder_thread_read  0 S android.hardware.sensors@1.0-service
root          1664     1   19132   4548 binder_thread_read  0 S android.hardware.usb@1.1-service.imx
system        1665     1   14076   4124 binder_thread_read  0 S android.hardware.xili@1.0-service
root          1724     1   20308   5936 binder_thread_read  0 S android.hardware.gnss@1.0-service-unicore
```

是不是看到一堆类似的进程。

现在执行freg的服务端进程：

```shell
mek_8q:/ # /vendor/bin/hw/android.hardware.freg@1.0-service                    
CANNOT LINK EXECUTABLE "/vendor/bin/hw/android.hardware.freg@1.0-service": library "android.hardware.freg@1.0-impl.so" not found
```

报错的意思是，找不到"android.hardware.freg@1.0-impl.so"这个文件。可明明已经把32位和64位的so分别push到“/vendor/lib/hw/”和“/vendor/lib64/hw/”了。那么就只有一种可能，即系统不是在“/vendor/lib(64)/hw/”这个目录下找的。

找到dlopen函数的源码：

```c++
//bionic/linker/linker.cpp
static int open_library(android_namespace_t* ns,
                        ZipArchiveCache* zip_archive_cache,
                        const char* name, soinfo *needed_by,
                        off64_t* file_offset, std::string* realpath) {
  TRACE("[ opening %s at namespace %s]", name, ns->get_name());

//加入PRINT语句，来打印日志，分析加载失败的原因
  if(strcmp("android.hardware.freg@1.0-impl.so",name) == 0){
    PRINT("xxx %s\n", name);
  }
....
}
```

在代码里插入许多PRINT。PRINT是`linker_debug.h`里面定义的宏。

因为系统里每个进程的加载so都会走到这里，为了避免无用日志，我只有name是"android.hardware.freg@1.0-impl.so"的时候，才打印。

最终我发现open_library函数是在用“/vendor/lib64/”和“/system/lib64”这两个路径，拼接name，然后加载，也就是加载的是“/vendor/lib/android.hardware.freg@1.0-impl.so”和“/vendor/lib64/android.hardware.freg@1.0-impl.so”。

所以，我姑且把so文件从hw目录拷贝到了外面。然后“/vendor/bin/hw/android.hardware.freg@1.0-service”就能跑了。

> 按规范，android.hardware.freg@1.0-impl.so应该是放到/vendor/lib64/hw。估计是我前面少做了什么。可以试一下整编，烧录system.img镜像，而不是push单个so。

2. 然后执行freg_test，依然不能得到binder service。因为我前面故意少了重要的一步：

   ```shell
   #打开/vendor/etc/vintf/manifest.xml
   busybox vi /vendor/etc/vintf/manifest.xml
   #在文件后面加
   <hal format="hidl">
       <name>android.hardware.freg</name>
       <transport>hwbinder</transport>
       <version>1.0</version>
       <interface>
           <name>IFreg</name>
           <instance>default</instance>
       </interface>
   </hal>
   ```

   这是直接在机器上面改的。如果要在源码里改，manifest.xml的位置取决于你的SOC。比如我的车机，位于/device/fsl/imx8q/mek_8q/manifest.xml。fsl是我们的供应商名称（飞思卡尔），“imx8q”是平台，mek_8q是产品TARGET_PRODUCT。

3. 重启，然后启动android.hardware.freg@1.0-service, 然后执行freg_test

```shell
130|mek_8q:/data/local/tmp # freg_test                                         
val = 97
```

OK了。

如果没有启动binder服务端（android.hardware.freg@1.0-service），freg_test里面getService会阻塞，直到service起来。

## 第五步

前面以及验证OK了，但是android.hardware.freg@1.0-service是手动启的。真实场景肯定不可能手动启。所以，我们添加一个rc文件：

创建`hardware/interfaces/freg/1.0/default/android.hardware.freg@1.0-service.rc`：

```rc
service freg_hal_service /vendor/bin/hw/android.hardware.freg@1.0-service
    class hal
    user system
    group system
# 注意缺了selinux的配置，这会带来问题。后面说。
```

打开构建“"android.hardware.freg@1.0-service"”的bp文件，添加init_rc配置：

```bp
cc_binary {
    name: "android.hardware.freg@1.0-service",
    defaults: ["hidl_defaults"],
    proprietary: true,
    relative_install_path: "hw",
    srcs: ["service.cpp"],
    init_rc: ["android.hardware.freg@1.0-service.rc"],//新增这一行
    shared_libs: [
        "libhidlbase",
        "libhidltransport",
        "libutils",
        "liblog",
        "android.hardware.freg@1.0",
        "android.hardware.freg@1.0-impl",
    ],
}
```

编译后，android.hardware.freg@1.0-service.rc在out/target/product/mek_8q/vendor/etc/init目录。打包system.img镜像，然后烧录。

因为只是一个小的改动，我不想烧录系统。所以直接把文件push到/vendor/etc/init，注意权限：

```shell
mek_8q:/vendor/etc/init # touch android.hardware.freg@1.0-service.rc
mek_8q:/vendor/etc/init # chmod 644 android.hardware.freg@1.0-service.rc
mek_8q:/vendor/etc/init # busybox vi android.hardware.freg@1.0-service.rc
# 添加代码
```

关闭SELinux:

```shell
setenforce 0
```

重启后，service没有自动起来。我曾怀疑是SELinux的问题，但是已经设置成宽容模式了。后来经过许多实验，发现还是要加上seclabel：

```rc
service freg_hal_service /vendor/bin/hw/android.hardware.freg@1.0-service
    class hal
    user system
    group system
	seclabel u:r:shell:s0
```

原来，并不是说，把selinux设成宽容模式就完全不需要配置了，有一些还是要的。

重启后进程有了：

```shell
mek_8q:/data/local/tmp $ ps -e | grep freg                                     
system        1656     1   13116   4100 0                   0 S android.hardware.freg@1.0-service

mek_8q:/data/local/tmp $ ./freg_test                                           
val = 97

```

* selinux需要配许多东西，这里我直接在service里面加了个`“seclabel u:r:shell:s0”`，同时把系统selinux设置为Permissive。这个seclabel是随便写的，实际项目肯定不能这样随意。因为这篇笔记我想突出重点，尽量简洁，不要涉及其它内容。
* android.hardware.freg@1.0-service.rc不需要被其它rc文件include。因为像“/vendor/etc/init”、“/system/etc/init”这些目录，开机时是自动扫描加载里面的所有rc文件的。

## 结尾

最终我们的代码目录结构是这样的

```shell
work@S111-CCS2plus:~/xue/plus_qm/hardware/interfaces/freg/1.0$ tree
.
├── Android.bp
├── default
│   ├── Android.bp
│   ├── android.hardware.freg@1.0-service.rc
│   ├── Freg.cpp
│   ├── Freg.h
│   └── service.cpp
└── IFreg.hal
```

其中，Android.bp、Freg.cpp、Freg.h先由命令生成，再在基础上修改。



## **绑定式 HAL**

第三步，我们的service是这样写的：

```c++
int main() {
    //直通（passthrough）模式
    return defaultPassthroughServiceImplementation<IFreg>();
}
```

如果是**绑定式 HAL**，有几点不同。

**一是**， 则应该直接调用RegisterAsService来注册服务：

```c++
#define LOG_TAG "android.hardware.freg@1.0-service"

#include <android/hardware/freg/1.0/IFreg.h>
#include <hidl/HidlTransportSupport.h>
#include "Freg.h"

using android::sp;
using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;

using android::hardware::freg::V1_0::IFreg;
using android::hardware::freg::V1_0::implementation::Freg;

int main() {
    // 配置 binder驱动线程池数量, 真正实现在 libhwbinder/ProcessState.cpp#setThreadPoolConfiguration中
    //true表示callerJoinsPool,即当前线程也会加到pool中（当前线程就是你现在看到的这个main方法线程）。不过configureRpcThreadpool本身
    //不会把当前线程加入pool，而是后面joinRpcThreadpool加入的。
    configureRpcThreadpool(1, true);//最大的线程数为1;
    sp<IFreg> freg = new Freg();
    if(android::OK !=  freg->registerAsService()){
        return 1; // or handle error
    }

    joinRpcThreadpool();//当前线程加入threadlooper。

    //理论上joinRpcThreadpool就进入了looper循环，joinRpcThreadpool不会返回，也就不会走到这里了
    return 1;
}
```

**二是**，原来用'-impl'生成的bp文件，内容是cc_library_shared，应改用'-service'生成cc_binary：

```bp
cc_binary {
    name: "android.hardware.freg@1.0-service",
    defaults: ["hidl_defaults"],
    proprietary: true,
    relative_install_path: "hw",
    srcs: ["service.cpp", "Freg.cpp"],//相比之前的直通式，这里Freg.cpp加进来
    init_rc: ["android.hardware.freg@1.0-service.rc"],
    shared_libs: [
        "libhidlbase",
        "libhidltransport",
        "libutils",
        "liblog",
        "android.hardware.freg@1.0"
    ],
```

所以那个impl.so就不需要了，代码都在service可执行文件里。

## 区别

如果扒一扒defaultPassthroughServiceImplementation的源码，会发现它的代码跟我们写的这段绑定式很像：

```c++
//是不是很像？
status_t defaultPassthroughServiceImplementation(std::string name,
                                            size_t maxThreads = 1) {
    configureRpcThreadpool(maxThreads, true);
    status_t result = registerPassthroughServiceImplementation<Interface>(name);//里面也是调用了registerAsService

    if (result != OK) {
        return result;
    }
    joinRpcThreadpool();
    return UNKNOWN_ERROR;
}

//registerPassthroughServiceImplementation
status_t registerPassthroughServiceImplementation(
        std::string name = "default") {
    sp<Interface> service = Interface::getService(name, true /* getStub */);//注意这里getStub是true
。。。
    status_t status = service->registerAsService(name);//是不是一模一样？
。。。
    return status;
}
```

那么是不是说直通式和绑定式 HAL就是一回事，是故意造了两个名词骗人的呢？还是说，区别只在一个分离出了so，另一个编译在一起？我们扒一下源码，`freg->registerAsService()`的逻辑是：

```c++
sm = ::android::hardware::defaultServiceManager();
sm->add(serviceName.c_str(), this);
```

然后获取service时就通过`IServiceManager->get`的方法去获取,就和AIDL一个套路。而对于直通式，`registerPassthroughServiceImplementation`里面`Interface::getService(name, true /* getStub */);`参数getStub传的是true，我们一直跟下去，到`transport/ServiceManagement.cpp#getRawServiceInternal`里面:

```c++
....
//绑定模式
    const bool vintfHwbinder = (transport == Transport::HWBINDER);
//直通模式
    const bool vintfPassthru = (transport == Transport::PASSTHROUGH);
......
//绑定模式, getStub为false
    for (int tries = 0; !getStub && (vintfHwbinder || vintfLegacy); tries++) {
        Return<sp<IBase>> ret = sm->get(descriptor, instance);
    }
//直通模式 getStub可能为true
    if (getStub || vintfPassthru || vintfLegacy) {
        const sp<IServiceManager> pm = getPassthroughServiceManager();
            sp<IBase> base = pm->get(descriptor, instance).withDefault(nullptr);
    }

....
```

然后到getPassthroughServiceManager里面，沿着这个调用链条：

```
getPassthroughServiceManager ——》getPassthroughServiceManager1_1() --》new PassthroughServiceManager() --》 get(const hidl_string& fqName, const hidl_string& name)
--》 openLibs
```

我们看openLibs：

```c++
static void openLibs(省略参数部分) {
.....
        const std::string prefix = packageAndVersion + "-impl";　　　　　//HIDL_FETCH_XXX 出现了，就是passthrough模式下需要被调用的方法。
        const std::string sym = "HIDL_FETCH_" + ifaceName;
.....
        std::vector<std::string> paths = {HAL_LIBRARY_PATH_ODM, HAL_LIBRARY_PATH_VENDOR,
                                          halLibPathVndkSp, HAL_LIBRARY_PATH_SYSTEM};
....
        for (const std::string& path : paths) {
            std::vector<std::string> libs = search(path, prefix, ".so");

            for (const std::string &lib : libs) {　　　　　　　　　　//路径最后组装成/system 或者/vendor 下面的供调用的xxxxx-impl.so,对于这个例子，就是android.hardware.freg@1.0-impl.so
                const std::string fullPath = path + lib;

                if (path == HAL_LIBRARY_PATH_SYSTEM) {                    //这里就供dlopen了。
                    handle = dlopen(fullPath.c_str(), dlMode);
                } else {
                    handle = android_load_sphal_library(fullPath.c_str(), dlMode);
                }
.....
        }
    }
```

好像从源码的角度看，区别还真的只是是否分离一个impl.so出来。defaultPassthroughServiceImplementation帮你dlopen impl.so文件，另一种方式没有这个so。

现在让我们真正来什么是直通式HAL，什么是绑定式HAL，先看官网给的图：

![img](_img/treble_cpp_legacy_hal_progression.png)

用我们写的freg例子来解读这张图。

* （1）是8.0之前的方法，对于freg例子来说，就是我们在step 3里面的，在“com_android_server_FregService.cpp”里通过hw_get_module来访问传统HAL。传统HAL打包成一个so。

* （2）中间绿色部分就是我们的"android.hardware.freg@1.0-impl.so",它包装了传统HAL。也是生成Freg.cpp后，我们应该编辑它，用hw_get_module这种代码访问传统HAL，大概就是一个透传。然后framework里jni的那块删掉hw_get_module这种代码，用hidl (passthrough)访问。至于怎么访问，不清楚，没看到这方面的资料和例子。

  (step4的例子里还没有访问传统HAL, 后面如果写step5,再补全代码)

* （3）就是这个step 4的方式，相比（2），加了个hw service ,就是我们代码里的service.cpp，里面dlopen impl.so。而framework通过hidl binder与hw service交互,调用方式与aidl类似。**注**：我在step4里没有适配framework部分，而是直接写了个freg_test可执行程序来测试调用，不过原理应该演示清楚了。

* （4）则是完全抛弃了传统HAL，代码里没有hw_get_module这种代码，hw service直接和驱动打交道。因为framework用hidl和hw service进程交互，hw service进程由vendor维护，故传统hal的一套不需要了。

所以到这里应能理解，（2）和（3）只是把传统HAL接口包装成了hidl的形式。只是一个过渡，是为了从android 8之前的版本升级过来时能够复用以前的传统HAL代码，并且随着版本升级，可能会逐步的替代成新的架构，而且这种升级对framework是透明的。

因为hal的代码在vendor.img里，framework的代码在system.img里面，HAL 由供应商或 SOC 制造商提供，framework的部分是设备制造商维护，这样就实现了设备专属底层软件与Android 操作系统框架分离开来。框架在自己的分区中通过 OTA 替换框架，而无需重新编译 HAL。