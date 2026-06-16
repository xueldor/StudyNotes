（基于android-14.0.0_r20）

## 链路

我们只关心启动链路，因此其他代码忽略。

当代码里直行`mContext.startActivity`时，

1. `ContextImpl.startActivity()`方法将请求传递给`Instrumentation.execStartActivity()`

2. execStartActivity里：

   ```java
   int result = ActivityTaskManager.getService().startActivity(whoThread,
                       who.getOpPackageName(), who.getAttributionTag(), intent,
                       intent.resolveTypeIfNeeded(who.getContentResolver()), token,
                       target != null ? target.mEmbeddedID : null, requestCode, 0, null, options);
   ```

   `ActivityTaskManager.getService()`获取到一个 IActivityTaskManager, 它是一个 AIDL 类，目录为： `frameworks/base/core/java/android/app/IActivityTaskManager.aidl`。

   在开机过程中， startBootstrapServices 中启动的其中一个服务，代码片段如下:

   ```java
   //frameworks/base/services/java/com/android/server/SystemServer.java
   private void startBootstrapServices(@NonNull TimingsTraceAndSlog t) {
           // ...
   
           // Activity manager runs the show.
           t.traceBegin("StartActivityManager");
           // TODO: Might need to move after migration to WM.
           ActivityTaskManagerService atm = mSystemServiceManager.startService(
                   ActivityTaskManagerService.Lifecycle.class).getService();
           mActivityManagerService = ActivityManagerService.Lifecycle.startService(
                   mSystemServiceManager, atm);
           mActivityManagerService.setSystemServiceManager(mSystemServiceManager);
           mActivityManagerService.setInstaller(installer);
           mWindowManagerGlobalLock = atm.getGlobalLock();
           t.traceEnd();
   
           // ...
       }
   
   // add到servicemanager在ActivityTaskManagerService.Lifecycle 里
   publishBinderService(Context.ACTIVITY_TASK_SERVICE, mService);
   
   protected final void publishBinderService(String name, IBinder service,
           boolean allowIsolated, int dumpPriority) {
       ServiceManager.addService(name, service, allowIsolated, dumpPriority);
   }
   ```

   从TODO 注释看，后面这块可能会有变动，谷歌想整合到windowmanager里。

   然后是获取binder：

   ```java
   final IBinder b = ServiceManager.getService(Context.ACTIVITY_TASK_SERVICE); // activity_task
   return IActivityTaskManager.Stub.asInterface(b);
   ```

   所以实现类是ActivityTaskManagerService，运行在system_server 进程里，这个调用进入system_server 进程。

   ```java
   //frameworks/base/services/core/java/com/android/server/wm/ActivityTaskManagerService.java
   public class ActivityTaskManagerService extends IActivityTaskManager.Stub {
       ...
               @Override
       public final int startActivity(IApplicationThread caller, String callingPackage,
               String callingFeatureId, Intent intent, String resolvedType, IBinder resultTo,
               String resultWho, int requestCode, int startFlags, ProfilerInfo profilerInfo,
               Bundle bOptions) {
           return startActivityAsUser(caller, callingPackage, callingFeatureId, intent, resolvedType,
                   resultTo, resultWho, requestCode, startFlags, profilerInfo, bOptions,
                   UserHandle.getCallingUserId());
       }
       
       进入startActivityAsUser，关键：
       return getActivityStartController().obtainStarter(intent, "startActivityAsUser")
                   .setCaller(caller)
                   .setCallingPackage(callingPackage)
                   .setCallingFeatureId(callingFeatureId)
                   .setResolvedType(resolvedType)
                   .setResultTo(resultTo)
                   .setResultWho(resultWho)
                   .setRequestCode(requestCode)
                   .setStartFlags(startFlags)
                   .setProfilerInfo(profilerInfo)
                   .setActivityOptions(bOptions)
                   .setUserId(userId)
                   .execute();
       ...
   }
   ```

   `getActivityStartController().obtainStarter(intent, "startActivityAsUser")`不过就是从对象池取一个ActivityStarter对象：

   ```java
   ActivityStarter starter = mStarterPool.acquire();
   if (starter == null) {
       starter = new ActivityStarter(mController, mService, mSupervisor, mInterceptor);
   }
   return starter;
   ```

   

    execute后面的流程：

   ```java
   exexute-》executeRequest-》startActivityUnchecked-》startActivityInner
   ```

   - **exexute**: 处理 Activity 启动请求的接口；
   - **executeRequest**: 执行一系列权限检查，对于合法的请求才继续；
   - **startActivityUnchecked**: 调用该方法时表示大部分初步的权限检查已经完成，执行 Trace，以及异常处理；
   - **startActivityInner**: 启动 Activity，并更新全局的 task 栈帧信息；

   

3. 启动activity分为：

   * attach到已经存在的process
   * 通过zygote孵化新的进程





## 相关的类和文件

有许多个Controller，但一次全列出来既记不住，也没意思。我们先关注当下，关注目前对于分析startActivityInner貌似重要的一部分。其它的后面慢慢熟悉。



但不是所以现在都要关注：

* ActivityStartController

  持有ActivityTaskManagerService、ActivityTaskSupervisor。通过工厂创建ActivityStart，维护了一个ActivityStart的对象池，

* BackNavigationController

* TaskChangeNotificationController

* LockTaskController

* IActivityController

* WindowProcessController

* WindowOrganizerController

* TaskOrganizerController

* TaskFragmentOrganizerController

* VrController

* KeyguardController

* ActivityClientController

* AnrController

* SurfaceControl

* TransitionController

* HideDisplayCutoutController

* ......



TaskFragment

Task

ActivityRecord

WindowToken

RootWindowContainer



DisplayManager

DisplayContent

SurfaceControl

TaskDisplayArea

RootDisplayArea



DisplayFrames



DefaultTaskDisplayArea
