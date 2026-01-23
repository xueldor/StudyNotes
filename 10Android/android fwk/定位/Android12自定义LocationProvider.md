1. LocationManagerService修改

   ```
   diff --git a/services/core/java/com/android/server/location/LocationManagerService.java b/services/core/java/com/android/server/location/LocationManagerService.java
   index 427f77a1..5a402b26 100644
   --- a/services/core/java/com/android/server/location/LocationManagerService.java
   +++ b/services/core/java/com/android/server/location/LocationManagerService.java
   @@ -141,6 +141,7 @@ import com.android.server.location.provider.StationaryThrottlingLocationProvider
    import com.android.server.location.provider.proxy.ProxyLocationProvider;
    import com.android.server.location.settings.LocationSettings;
    import com.android.server.location.settings.LocationUserSettings;
   +import com.android.server.location.tboxgpsprovider.TboxLocationProvider;
    import com.android.server.pm.permission.LegacyPermissionManagerInternal;
    
    import java.io.FileDescriptor;
   @@ -202,6 +203,11 @@ public class LocationManagerService extends ILocationManager.Stub implements
                    // some providers rely on third party code, so we wait to initialize
                    // providers until third party code is allowed to run
                    mService.onSystemThirdPartyAppsCanStart();
   +            } else if (phase == PHASE_HOME_READY) {
   +                if (mService.mTboxLocationProvider != null) {
   +                    mService.mTboxLocationProvider.init();
   +                    mService.addLocationProviderManager(mService.mTboxGpsManager, mService.mTboxLocationProvider);
   +                }
                }
            }
    
   @@ -256,6 +262,9 @@ public class LocationManagerService extends ILocationManager.Stub implements
        private volatile @Nullable GnssManagerService mGnssManagerService = null;
        private GeocoderProxy mGeocodeProvider;
    
   +    LocationProviderManager mTboxGpsManager;
   +    private TboxLocationProvider mTboxLocationProvider;
   +
        private final Object mDeprecatedGnssBatchingLock = new Object();
        @GuardedBy("mDeprecatedGnssBatchingLock")
        private @Nullable ILocationListener mDeprecatedGnssBatchingListener;
   @@ -463,8 +472,13 @@ public class LocationManagerService extends ILocationManager.Stub implements
                Log.wtf(TAG, "no fused location provider found");
            }
    
   +//        mTboxLocationProvider = new TboxLocationProvider(mContext);
   +//        mTboxGpsManager = new LocationProviderManager(mContext, mInjector,
   +//                /*GPS_PROVIDER*/ TBOX_PROVIDER, mPassiveManager); // 新增TBOX_PROVIDER
   +
            // initialize gnss last because it has no awareness of boot phases and blindly assumes that
            // all other location providers are loaded at initialization
            if (GnssNative.isSupported()) {
                GnssConfiguration gnssConfiguration = new GnssConfiguration(mContext);
                GnssNative gnssNative = GnssNative.create(mInjector, gnssConfiguration);
   
   ```

   

2. 新建TboxLocationProvider.java，

   ```
   services/core/java/com/android/server/location/tboxgpsprovider$ cat TboxLocationProvider.java 
   package com.android.server.location.tboxgpsprovider;
   
   import static android.location.provider.ProviderProperties.ACCURACY_FINE;
   import static android.location.provider.ProviderProperties.POWER_USAGE_LOW;
   
   
   import android.content.Context;
   import android.location.Location;
   import android.location.LocationManager;
   import android.location.LocationManagerInternal;
   import android.location.LocationResult;
   import android.location.provider.ProviderProperties;
   import android.location.provider.ProviderRequest;
   import android.location.util.identity.CallerIdentity;
   import android.os.Build;
   import android.os.Bundle;
   import android.os.Handler;
   import android.os.Message;
   import android.os.RemoteException;
   import android.util.Slog;
   
   import com.android.server.FgThread;
   import com.android.server.location.provider.AbstractLocationProvider;
   
   import com.lanyou.tcu.protocolservicelibrary.ITcuDeviceManager;
   import com.lanyou.tcu.protocolservicelibrary.ServiceProxy;
   
   import java.io.FileDescriptor;
   import java.io.PrintWriter;
   import java.util.Collections;
   
   public class TboxLocationProvider extends AbstractLocationProvider implements
           LocationManagerInternal.ProviderEnabledListener {
   
       private static final String TAG = "TboxLocationProvider";
   
       private float lat = 1.0f;
       private float lon = 10.0f;
   
       private final Context mContext;
       private ServiceProxy mServiceProxy;
       private ITcuDeviceManager mTcuDeviceManager;
   
       private static final ProviderProperties PROPERTIES = new ProviderProperties.Builder()
               .setHasSatelliteRequirement(false)
               .setHasNetworkRequirement(false)
               .setHasCellRequirement(false)
               .setHasMonetaryCost(false)
               .setHasAltitudeSupport(true)
               .setHasSpeedSupport(false)
               .setHasBearingSupport(true)
               .setPowerUsage(POWER_USAGE_LOW)
               .setAccuracy(ACCURACY_FINE)
               .build();
   
       private final ITcuDeviceManager.ITcuDeviceStatusListener mTcuDeviceStatusListener = new ITcuDeviceManager.ITcuDeviceStatusListener() {
           @Override
           public void onTCUdeviceReady() {
               Slog.d(TAG, "TCU设备就绪，开始注册");
               startRegist();
           }
   
           @Override
           public void onTCUdeviceAbnormal(String s) {
               Slog.d(TAG, "TCU设备异常: " + s);
           }
   
           @Override
           public void onError(int i) {
               Slog.e(TAG, "TCU设备错误码: " + i);
           }
   
           @Override
           public void onRegisterFail() {
               Slog.d(TAG, "TCU注册失败，重试注册");
               startRegist();
           }
   
           @Override
           public void onRegisterSuccess(int i, String sn, String softwareVer, String hardwareVer) {
               Slog.e(TAG, "TCU注册成功: SN=" + sn + ", 软件版本=" + softwareVer + ", 硬件版本=" + hardwareVer);
               startMock();
   //            try {// tbox 自动1s上报一次，因此不需要主动获取
   //                mTcuDeviceManager.getGpsData();
   //            } catch (RemoteException e) {
   //                throw new RuntimeException(e);
   //            }
           }
       };
       // GPS传感器监听器
       private final ITcuDeviceManager.ITcuSensorListener mTcuSensorListener = new ITcuDeviceManager.ITcuSensorListener() {
           @Override
           public void onGPSUpdate(String s) {
               Slog.i(TAG, "GPS数据更新: " + s);
           }
       };
   
       public TboxLocationProvider(Context context) {
           super(FgThread.getExecutor(), CallerIdentity.fromContext(context), PROPERTIES,
                   Collections.emptySet());
           mContext = context;
       }
   
       public void init() {
           initTcuService();
       }
   
       /**
        * 初始化TCU服务连接
        * author: 张晶
        */
       private void initTcuService() {
           mServiceProxy = ServiceProxy.getInstance(mContext, new ServiceProxy.ServiceProxyListener() {
               @Override
               public void onServiceConnected(ITcuDeviceManager deviceManager) {
                   Slog.d(TAG, "TCU服务连接成功");
                   mTcuDeviceManager = deviceManager;
                   // 注册监听器
                   mTcuDeviceManager.setTcuDeviceStatusListener(mTcuDeviceStatusListener);
                   mTcuDeviceManager.setTcuSeneorListener(mTcuSensorListener);
                   Slog.i(TAG, "TCU监听器注册完成");
   
                   // 检查注册状态
                   checkTcuRegistStatus();
               }
   
               @Override
               public void onServiceDisconnected() {
                   Slog.d(TAG, "TCU服务断开，重新绑定");
                   mTcuDeviceManager = null;
                   ServiceProxy.getInstance(mContext, this).rebind(mContext);
               }
   
               @Override
               public void onServiceConnectError() {
                   Slog.e(TAG, "TCU服务连接失败");
               }
           });
       }
   
       /**
        * 启动TCU注册流程
        */
       private void startRegist() {
           try {
               if (mTcuDeviceManager == null || !mTcuDeviceManager.isDeviceReady()) {
                   Slog.e(TAG, "TCU设备未就绪，无法注册");
                   return;
               }
               Slog.d(TAG, "开始TCU注册");
               mTcuDeviceManager.startRegist(0, "0000", Build.DISPLAY);
           } catch (RemoteException e) {
               Slog.e(TAG, "注册Remote异常: " + e.getMessage());
           }
       }
       /**
        * 检查TCU注册状态
        */
       private void checkTcuRegistStatus() {
           try {
               if (mTcuDeviceManager != null) {
                   boolean isRegisted = mTcuDeviceManager.isDeviceRegisted();
                   Slog.d(TAG, "TCU当前注册状态: " + isRegisted);
                   if (!isRegisted) {
                       startRegist();
                   }
               }
           } catch (RemoteException e) {
               Slog.e(TAG, "检查注册状态异常: " + e.getMessage());
           }
       }
   
       private Handler mHandler = new Handler(new Handler.Callback() {
   
           @Override
           public boolean handleMessage(Message msg) {
               Slog.i(TAG, "handleMessage: reportLocation");
               Location location = new Location(LocationManager.GPS_PROVIDER);
               location.setLatitude(lat + 0.1);
               location.setLongitude(lon + 0.2);
               location.makeComplete();
               LocationResult result = LocationResult.create(location);
               reportLocation(result);
               if (!mHandler.hasMessages(1)) {
                   mHandler.sendEmptyMessageDelayed(1, 1000);
               }
               return false;
           }
       });
   
       public void startMock() {
           Slog.i(TAG, "startMock");
           if (!mHandler.hasMessages(1)) {
               mHandler.sendEmptyMessageDelayed(1, 1000);
           }
       }
   
       @Override
       protected void onSetRequest(ProviderRequest request) {
   
       }
   
       @Override
       protected void onFlush(Runnable callback) {
   
       }
   
       @Override
       protected void onExtraCommand(int uid, int pid, String command, Bundle extras) {
   
       }
   
       @Override
       protected void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
   
       }
   
       @Override
       public void onProviderEnabledChanged(String provider, int userId, boolean enabled) {
           Slog.i(TAG, "onProviderEnabledChanged provider=" + provider + ", userId=" + userId + ",enabled=" + enabled);
       }
   }
   
   ```

   

3. 系统集成TCU提供的jar包。