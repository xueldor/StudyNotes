* 普通app（adb install方式）调用so
* 系统app调用so
* c++ 可执行程序

示例：

so代码：

```
// Android.bp
cc_library_shared {
    name: "libhelloworld",
    srcs: ["hello_world.c"],
    cflags: [
        "-Wall",
        "-Werror",
        "-Wno-unused-parameter",
    ],
    shared_libs: [
        "libutils",
        "libbase",
        "libnativehelper",
        "liblog",
    ],
}

//hello_world.c 
#include <jni.h>
#include <stdio.h>
#include <android/log.h>

#define LOG_TAG "HelloWorldJNI"

JNIEXPORT void JNICALL
Java_com_xue_testcallingso_MainActivity_printHelloWorld(JNIEnv *env, jobject obj) {
    printf("Hello, World!\n"); #  Android 系统的标准输出流被重定向，printf不打印
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "Hello, World!");
}

```



app代码：

```java
package com.xue.testcallingso;
//省略部分不相干代码。。。
import android.os.Bundle;

public class MainActivity extends AppCompatActivity {
    static {
        System.loadLibrary("helloworld"); // 加载共享库,自动在前面加上lib
    }

    // 对应so函数命名规则：Java_包名_类名_函数名
    public native void printHelloWorld();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        printHelloWorld();
    }
}
```

结论：

当app和so在同一个分区下，即：

* app push到/system/app, so push到/system/lib64
* app push到/vendor/app, so push到/vendor/lib64
* app push到/product/app, so push到/product/lib64

可以调用。所以会搜索或加载同分区的lib64目录。

不在一个分区不能调用。当so在system分区时，按理其它分区的app应该能够访问，因为原本system分区的so应该对vendor是开放的（反之则不然，vendor独有的不应该对system开放），但安卓后来做了限制，默认不让用，故报错信息是：

```
AndroidRuntime: java.lang.UnsatisfiedLinkError: dlopen failed: library "/system/lib64/libhelloworld.so" needed or dlopened by "/apex/com.android.art/lib64/libnativeloader.so" is not accessible for the namespace "product-clns-6"
```

注意名字product-clns-6。于是可以判断，安卓限制访问的机理，就是类似名称空间(namespace)的东西。也就是，app在product下，那么app访问so是，会加一个procuct的namespace，而system下的so没有这个namespace，访问不到。

不清楚这个namespace的信息是不是打在so内部。如果是，则不能把namespace是system的放到vendor，即前面的手法有问题。但这符合常理，相信不会这样做。因为一来，so是有标准的，谷歌不可能自己动它。而来，如果这样，那么网络上下载的各种第三方库怎么办？ 且前面的验证手法，app和so同时依次push到system、vendor、product，可以访问，这本身已经表明so自身是不应该有这种namespace信息的。

其它情况报错信息则是：

```
AndroidRuntime: java.lang.UnsatisfiedLinkError: dlopen failed: library "libhelloworld.so" not found
```

同理，java代码使用绝对路径加载so：

```
System.load("/product/lib64/libhelloworld.so");
```

把app放在system/app, 报错也是：

```
AndroidRuntime: java.lang.UnsatisfiedLinkError: dlopen failed: library "/product/lib64/libhelloworld.so" needed or dlopened by "/apex/com.android.art/lib64/libnativeloader.so" is not accessible for the namespace "clns-shared-6"
```

除了namespace名字不一样。



adb install方式均不能调用。和上面一样，当libhelloworld.so push到/system/lib64, loadLibrary时报：

```
11-01 15:27:02.021  2153  2153 E AndroidRuntime: java.lang.UnsatisfiedLinkError: dlopen failed: library "/system/lib64/libhelloworld.so" needed or dlopened by "/apex/com.android.art/lib64/libnativeloader.so" is not accessible for the namespace "clns-6"
11-01 15:27:02.021  2153  2153 E AndroidRuntime: 	at java.lang.Runtime.loadLibrary0(Runtime.java:1082)
```

push到其它分区时：

```
java.lang.UnsatisfiedLinkError: dlopen failed: library "libhelloworld.so" not found
```



代办：

测试一下添加公共库的清单：/system/etc/public.libraries.txt

