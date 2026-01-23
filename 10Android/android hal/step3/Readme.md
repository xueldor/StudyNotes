介绍一下代码的环境部署。

为了简单和突出重点，kernel里的freg驱动，基于step 2， 但是删掉了sysfs、procfs相关的一部分代码,仅通过sysfs创建设备文件。

## 硬件抽象层

"hardware/libhardware/modules"目录新建freg目录，把freg.cpp和Android.mk放进去。

头文件freg.h放到"hardware/libhardware/include/hardware"下面。

进入"hardware/libhardware/modules/freg"，用mm编译。“out/target/product/mek_8q/system/lib64/hw”目录生成freg.default.so。

## framework层

### 接口

把IFregService.aidl文件放到`frameworks/base/core/java/android/freg/IFregService.aidl`.

然后回到`frameworks/base/`这一级，打开Android.bp。新增一行"core/java/android/freg/IFregService.aidl"。

老版本安卓是Android.mk，一样的加。

然后`frameworks/base/`这一级执行mm，成功后把framework.jar文件导出来。

用反编译工具打开framework.jar，可以看到`IFregService`已经加进去了：

```java
package android.freg;
...
public interface IFregService extends IInterface {
    int getVal() throws RemoteException;

    void setVal(int i) throws RemoteException;

    public static abstract class Stub extends Binder implements IFregService {
        private static final String DESCRIPTOR = "android.freg.IFregService";
        static final int TRANSACTION_getVal = 2;
        static final int TRANSACTION_setVal = 1;

        public Stub() {
            attachInterface(this, DESCRIPTOR);
        }

....
```

### 实现服务

1. java部份

前面只是用aidl定义了接口。那么调用接口后，执行什么逻辑还没写。下面实现。

把FregService.java文件放到“frameworks/base/services/java/com/android/server/FregService.java”。（或者frameworks/base/services/core/java/com/android/server/）因为“frameworks/base/services”的Android.bp里面：

```txt
    srcs: [
        "java/**/*.java",
    ],
```

即自动识别所有java文件。所以无需更改bp文件。

在“frameworks/base/services”这一级执行mm。将生成的services.jar导出，用反编译工具打开。可以看到“com/android/server”这个层级下有了一个FregService类。

2. jni部分

   com_android_server_FregService.cpp拷贝到“frameworks/base/services/core/jni/”。

   代码中实现了“register_android_server_FregService”方法来注册JNI方法表，所以要在"JNI_Onload"里面调用这个方法： 打开相同目录下的onload.cpp文件，把调用register_android_server_FregService加进去。

   ```c++
   。。。
   
   namespace android {
   。。。
   //声明register_android_server_FregService函数
   int register_android_server_FregService(JNIEnv* env);
   ...
   };
   
   using namespace android;
   
   extern "C" jint JNI_OnLoad(JavaVM* vm, void* /* reserved */)
   {
       JNIEnv* env = NULL;
       jint result = -1;
   ...
       //调用
       register_android_server_FregService(env);
   ...
       return JNI_VERSION_1_4;
   }
   
   ```

   接着Android.bp中，把源文件加进去，这样才能编译到这个代码。

   ```txt
   //frameworks/base/services/core/jni/Android.bp
   ...
       srcs: [
           。。。
           "com_android_server_FregService.cpp",
           "onload.cpp",
       ],
   ...
   ```

   

### 启动FregService服务

安卓framework层的服务都运行在system_server进程中。打开“SystemServer.java”（也是在frameworks/base/services/java/com/android/server/）。这个类，安卓不同版本修改较大，并且各家ROM定制修改也比较多，所以我没法明确告诉你加在哪。搜索“addService”，看看其它系统服务在哪加的，依葫芦画瓢。

```java
//SystemServer.java
try{
    Slog.i(TAG, "Add Freg Service");
    ServiceManager.addService("freg", new FregService());
}catch(Throwable e){
    Slog.e(TAG, "Failure starting Freg Service", e);
}
```

重新编译services模块。



make systemimage,得到system.img，烧录。

重启。起来后记得修改ueventd.rc，把/dev/freg的权限改成666:

```txt
/dev/freg                 0666   root       root
```

改成666是肯定没问题的，但是因为我们的服务是放在system_server进程里，所以用户改成system应该是更好的选择：

```txt
/dev/freg                 0660   system      system
```



## 测试app

`out/target/common/obj/JAVA_LIBRARIES/framework_intermediates/`把classes.jar拷贝出来。

AS创建项目TestFreg，gradle中添加对classes.jar的依赖，但是不要编进代码：

```groovy
dependencies {
    compileOnly files('libs\\classes.jar')
}
```

获取freg服务：

```java
mFregService = IFregService.Stub.asInterface(ServiceManager.getService("freg"));
```



