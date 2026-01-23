## 网络连接受限
类原生系统在国内使用，连接网络后，实际可以上网，但是设置和状态栏的wifi图标会显示网络连接受限，显示一个感叹号。

原因是系统会去请求访问一个google的网址来确定网络是否可用。Google在墙外无法正常访问，系统请求不成功，就会出现叉号或叹号。

根据系统的安卓版本在电脑上cmd中运行以下命令：

（1）Android 7.0之前版本的系统电脑cmd中执行

adb shell "settings put global captive_portal_server connect.rom.miui.com";

（2）Android 7.0之后的版本需要执行下面的两条命令：

```shell
adb shell "settings put global captive_portal_http_url http://connect.rom.miui.com/generate_204"; 

adb shell "settings put global captive_portal_https_url https://connect.rom.miui.com/generate_204";
```

相关关键词见frameworks/base/core/java/android/provider/Settings.java

然后先打开再关闭飞行模式。


同理，如果无法自动同步时间，可修改ntp服务器，见：
https://zhuanlan.zhihu.com/p/32518769?ivk_sa=1024320u



## GPS定位问题
纯的卫星定位是不依赖网络的，只要在户外信号好的地方都可以定位。有一些辅助技术（AGPS）需要联网辅助定位，但总体来说，调用原生定位接口，provider设置为GPS时，如果国内的系统能定位，那么 原生、类原生、国外系统应该也能定位。

问题是，为了提高定位速度，并且为了支持室内，应用往往是采用网络定位（NETWORK_PROVIDER）。网络定位是需要第三方自己实现的，aosp里面只提供接口，需要自己实现这些接口。国内手机做了专门定制，比如小米的网络定位服务应用是 `MetokNLP`，可以把这个应用从`/system/app`路径提取出来，但是不知道这个apk有没有依赖其它文件（比如一些so）; 国外的手机，可能是用的谷歌服务，因为墙的原因，国内无法使用。

国内一般是使用百度的网络定位服务。有些公司和百度合作，百度会提供一个`networklocation_baidu.apk`，网上只找到一个10年前的apk，反编译一看，无非也是集成和调用百度公开的定位sdk（网址：https://lbsyun.baidu.com/index.php?title=android-locsdk）。因此，可以自己写一个。


1.  先编译源码，然后找到`com.android.location.provider.jar`，项目里面以compileOnly 的方式引用
2. 继承`LocationProviderBase`类，实现`onSetRequest`方法，当外面请求一次定位，这里就收到一次`onSetRequest`回调，在这个方法里，调用百度的sdk来请求位置。
3. 当百度sdk定位成功后，调用`reportLocation(mLocation);`将位置上报给系统。
4. 需要申请权限：
```xml
    <uses-permission android:name="android.permission.ACCESS_FINE_LOCATION" />
    <uses-permission android:name="android.permission.ACCESS_COARSE_LOCATION" />
    <uses-permission android:name="android.permission.ACCESS_BACKGROUND_LOCATION" />
    <uses-permission android:name="android.permission.ACCESS_NETWORK_STATE" />
    <uses-permission android:name="android.permission.ACCESS_WIFI_STATE" />
    <uses-permission android:name="android.permission.CHANGE_WIFI_STATE" />
    <uses-permission android:name="android.permission.READ_EXTERNAL_STORAGE"/>
    <uses-permission android:name="android.permission.WRITE_EXTERNAL_STORAGE"/>
    <uses-permission android:name="android.permission.INTERNET" />
    <uses-permission android:name="android.permission.NETWORK_MANAGED_PROVISIONING"/>
```
特别说明一下，最后一个权限（NETWORK_MANAGED_PROVISIONING），普通应用不需要也能定位，百度sdk官网也没有让加这个权限，但是当我把它作为系统app集成到`system/app`下面时，要加上才能定位成功。

5. 编译应用得到apk（我的叫XueNLP.apk），放到源码的`packages/apps/XueNLP`目录，并编写Android.bp:
```bp
android_app_import {

    name: "XueNLP",

    certificate: "platform",
    //presigned: true,
    uses_libs: ["com.android.location.provider"],
    apk: "XueNLP.apk",

}
```
6. 把这个应用集成到系统镜像，这一步不同的手机不一样，我的redmi 9刷Pixel  Experience系统，在`device/xiaomi/lava/device.mk`文件里，添加`PRODUCT_PACKAGES += XueNLP`

7. 接下来，要把我的apk包名告诉系统，告诉系统，网络定位要用我的应用。  
打开文件frameworks/base/core/res/res/values/config.xml，修改以下属性：  
`config_enableNetworkLocationOverlay改为false、config_networkLocationProviderPackageName改为包名`

然后，针对我的手机，改了并没有用，enable仍然是true，因为在vendor里面，用安卓的overlay机制把属性值更新成了true。文件`vendor/aosp/rro_overlays/PixelFwResOverlay/res/values/bools.xml`： 
```xml
<resources>
    <bool name="config_allowDisablingAssistDisclosure">true</bool>
    <bool name="config_enableFusedLocationOverlay">true</bool>
    <bool name="config_enableGeocoderOverlay">true</bool>
    <bool name="config_enableGeofenceOverlay">true</bool>
    <bool name="config_enableNetworkLocationOverlay">true</bool>
    <bool name="config_useRoundIcon">true</bool>
</resources>
```
解决方法：
1） 把相关属性值改成false，重新编译烧录即可。
2） 或者用冰箱或小黑屋或命令行把gms冻结，也能解决问题。

8. 刷机后，到设置里面，找到XueNLP应用，确认应用的权限。

9. 联网，调用原声接口，确认网络定位可用。

## 附修改点(repo diff > diff.txt)
(除了diff.txt内容，还需包括`packages/apps/XueNLP`，因为XueNLP没有在git里面，diff显示不出来）

```
project device/xiaomi/lava/
diff --git a/device.mk b/device.mk
index bff1a41..af5a4cd 100644
--- a/device.mk
+++ b/device.mk
@@ -42,7 +42,8 @@ PRODUCT_PACKAGES += \
     android.hardware.nfc@1.2-service \
     com.android.nfc_extras \
     NfcNci \
-    Tag
+    Tag \
+    XueNLP
 
 # SKU-specific properties
 PRODUCT_COPY_FILES += \

project frameworks/base/
diff --git a/core/res/res/values/config.xml b/core/res/res/values/config.xml
index 90841ab58a65..ebeb34b152c0 100644
--- a/core/res/res/values/config.xml
+++ b/core/res/res/values/config.xml
@@ -1709,17 +1709,17 @@
          config_networkLocationProviderPackageName package will be searched for network location
          provider, otherwise any system package is eligible. Anyone who wants to disable the overlay
          mechanism can set it to false. -->
-    <bool name="config_enableNetworkLocationOverlay" translatable="false">true</bool>
+    <bool name="config_enableNetworkLocationOverlay" translatable="false">false</bool>
     <!-- Package name providing network location support. Used only when
          config_enableNetworkLocationOverlay is false. -->
-    <string name="config_networkLocationProviderPackageName" translatable="false">@null</string>
+    <string name="config_networkLocationProviderPackageName" translatable="false">com.xue.position</string>
 
     <!-- Whether to enable fused location provider overlay which allows fused location provider to
          be replaced by an app at run-time. When disabled, only the
          config_fusedLocationProviderPackageName package will be searched for fused location
          provider, otherwise any system package is eligible. Anyone who wants to disable the overlay
          mechanism can set it to false. -->
-    <bool name="config_enableFusedLocationOverlay" translatable="false">true</bool>
+    <bool name="config_enableFusedLocationOverlay" translatable="false">false</bool>
     <!-- Package name providing fused location support. Used only when
          config_enableFusedLocationOverlay is false. -->
     <string name="config_fusedLocationProviderPackageName" translatable="false">com.android.location.fused</string>
@@ -1778,6 +1778,7 @@
          several permissions by the system, and should be system packages. -->
     <string-array name="config_locationProviderPackageNames" translatable="false">
         <!-- The standard AOSP fused location provider -->
+        <item>com.xue.position</item>
         <item>com.android.location.fused</item>
     </string-array>
 

project vendor/aosp/
diff --git a/rro_overlays/PixelFwResOverlay/res/values/bools.xml b/rro_overlays/PixelFwResOverlay/res/values/bools.xml
index f6598a1c..6a6674f7 100644
--- a/rro_overlays/PixelFwResOverlay/res/values/bools.xml
+++ b/rro_overlays/PixelFwResOverlay/res/values/bools.xml
@@ -1,9 +1,9 @@
 <?xml version="1.0" encoding="utf-8"?>
 <resources>
     <bool name="config_allowDisablingAssistDisclosure">true</bool>
-    <bool name="config_enableFusedLocationOverlay">true</bool>
+    <bool name="config_enableFusedLocationOverlay">false</bool>
     <bool name="config_enableGeocoderOverlay">true</bool>
     <bool name="config_enableGeofenceOverlay">true</bool>
-    <bool name="config_enableNetworkLocationOverlay">true</bool>
+    <bool name="config_enableNetworkLocationOverlay">false</bool>
     <bool name="config_useRoundIcon">true</bool>
 </resources>

```

