当app进入前台、退至后台，oomAdj的值会变化。以设置为例：

```shell
emulator_x86_64:/ # ps -ef | grep setting
system        2065   285 13 16:19:58 ?    00:00:00 com.android.settings

# 打开设置界面
emulator_x86_64:/proc/2065 # cat oom_score_adj 
0
# 按下home，设置退到后台
emulator_x86_64:/proc/2065 # cat oom_score_adj
700
```

关于oom_score_adj、oom_adj、oom_score，在lowmemorykiller那里讲解过。它决定了在lowmemorykiller那里杀后台的级别。其实它还影响进程的cgroups分组。调用轨迹：

```
01-03 16:19:59.245   502   538 W System.err: java.lang.Exception:  OomAdjuster
01-03 16:19:59.246   502   538 W System.err: 	at com.android.server.am.OomAdjuster.lambda$new$0(OomAdjuster.java:448)
01-03 16:19:59.246   502   538 W System.err: 	at com.android.server.am.OomAdjuster.$r8$lambda$G9qaeCQ1bE6cG3uK32c_XCnZvYk(OomAdjuster.java:0)
01-03 16:19:59.246   502   538 W System.err: 	at com.android.server.am.OomAdjuster$$ExternalSyntheticLambda3.handleMessage(R8$$SyntheticClass:0)
01-03 16:19:59.246   502   538 W System.err: 	at android.os.Handler.dispatchMessage(Handler.java:102)
01-03 16:19:59.246   502   538 W System.err: 	at android.os.Looper.loopOnce(Looper.java:205)
01-03 16:19:59.246   502   538 W System.err: 	at android.os.Looper.loop(Looper.java:294)
01-03 16:19:59.246   502   538 W System.err: 	at android.os.HandlerThread.run(HandlerThread.java:67)
01-03 16:19:59.246   502   538 W System.err: 	at com.android.server.ServiceThread.run(ServiceThread.java:46)
01-03 16:19:59.246   502   538 W Process : android_os_Process_setProcessGroup tid=2065 grp=5

```

OomAdjuster里面调用android.os.Process setProcessGroup方法。这是个native函数，实现在：android_util_Process.cpp,  {"setProcessGroup", "(II)V", (void*)android_os_Process_setProcessGroup}.

而OomAdjuster的堆栈是

```
01-03 16:19:59.246   502  1435 W System.err: java.lang.Exception: applyOomAdjLSP
01-03 16:19:59.246   502  1435 W System.err: 	at com.android.server.am.OomAdjuster.applyOomAdjLSP(OomAdjuster.java:3134)
01-03 16:19:59.246   502  1435 W System.err: 	at com.android.server.am.OomAdjuster.performUpdateOomAdjLSP(OomAdjuster.java:561)
01-03 16:19:59.246   502  1435 W System.err: 	at com.android.server.am.OomAdjuster.performUpdateOomAdjLSP(OomAdjuster.java:660)
01-03 16:19:59.247   502  1435 W System.err: 	at com.android.server.am.OomAdjuster.updateOomAdjLSP(OomAdjuster.java:628)
01-03 16:19:59.247   502  1435 W System.err: 	at com.android.server.am.OomAdjuster.updateOomAdjLocked(OomAdjuster.java:610)
01-03 16:19:59.247   502  1435 W System.err: 	at com.android.server.am.ActivityManagerService.updateOomAdjLocked(ActivityManagerService.java:16572)
01-03 16:19:59.247   502  1435 W System.err: 	at com.android.server.am.ActivityManagerService.attachApplicationLocked(ActivityManagerService.java:4830)
01-03 16:19:59.247   502  1435 W System.err: 	at com.android.server.am.ActivityManagerService.attachApplication(ActivityManagerService.java:4870)
01-03 16:19:59.247   502  1435 W System.err: 	at android.app.IActivityManager$Stub.onTransact(IActivityManager.java:2711)
01-03 16:19:59.247   502  1435 W System.err: 	at com.android.server.am.ActivityManagerService.onTransact(ActivityManagerService.java:2764)

applyOomAdjLSP里：
mProcessGroupHandler.sendMessage(mProcessGroupHandler.obtainMessage(
                        0 /* unused */, app.getPid(), processGroup, app.processName));
```



进入android_util_Process.cpp里的setProcessGroup函数

```
void android_os_Process_setThreadGroupAndCpuset(JNIEnv* env, jobject clazz, int tid, jint grp)
{
...
   int res = SetTaskProfiles(tid, {get_cpuset_policy_profile_name((SchedPolicy)grp)}, true) ? 0 : -1;
...
}
```

get_cpuset_policy_profile_name返回下面的名字：

```
static constexpr const char* kCpusetProfiles[SP_CNT + 1] = {
        "CPUSET_SP_DEFAULT", "CPUSET_SP_BACKGROUND", "CPUSET_SP_FOREGROUND",
        "CPUSET_SP_SYSTEM",  "CPUSET_SP_FOREGROUND", "CPUSET_SP_FOREGROUND",
        "CPUSET_SP_TOP_APP", "CPUSET_SP_DEFAULT",    "CPUSET_SP_RESTRICTED"};
```

然后到task_profiles.cpp里，TaskProfiles::SetTaskProfiles函数。

TaskProfiles构造函数会读这些文件：“/etc/task_profiles.json"、"/vendor/etc/task_profiles.json"、"/etc/task_profiles/task_profiles_%u.json";

第三个%u是当前系统的api level，通过getprop ro.product.first_api_level得到。读取后生成数据结构保存在profiles_成员里，解析文件的逻辑在bool TaskProfiles::Load(const CgroupMap& cg_map, const std::string& file_name) 。

最后执行TaskProfiles::SetTaskProfiles，关键两行：

```
TaskProfile* profile = GetProfile(name);
profile->ExecuteForTask(tid)

bool TaskProfile::ExecuteForTask(int tid) const {
    if (tid == 0) {
        tid = GetThreadId();
    }
    for (const auto& element : elements_) {
        if (!element->ExecuteForTask(tid)) {
            LOG(VERBOSE) << "Applying profile action " << element->Name() << " failed";
            return false;
        }
    }
    return true;
}
```

elements_的成员类型是ProfileAction，SetCgroupAction是其子类，实现了ExecuteForTask。细节不深究了，无非调用write函数往/dev/cpuset/xxx写入pid。分组的路径也算从task_profiles.json获得。







