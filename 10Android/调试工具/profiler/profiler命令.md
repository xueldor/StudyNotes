集成在android studio里的profiler功能，如果没有环境，可以用命令获取.

![image-20251218151159264](./imgs/profiler%E5%91%BD%E4%BB%A4/image-20251218151159264.png)

* Method Recording

   Method Recording 的两种核心模式

  | 模式             | 原理                          | 开销             | 适用场景                   | 命令行支持 |
  | ---------------- | ----------------------------- | ---------------- | -------------------------- | ---------- |
  | Sampling（采样） | 每隔 N 毫秒记录栈顶方法       | 低（1-5% CPU）   | 日常性能分析、高频方法定位 | 全支持     |
  | Tracing（全量）  | 插桩记录所有方法的进入 / 退出 | 高（10-20% CPU） | 精准定位耗时、调用链       |            |

  ```bash
  am profile只支持java
  # 启动方法记录（采样模式）
  adb shell am profile start [--sampling <间隔毫秒>] <应用包名> <输出文件路径>
  # 停止方法记录
  adb shell am profile stop <应用包名>
  
  # 启动全量追踪（无 --sampling 参数，插桩模式）
  adb shell am profile start <应用包名> <输出文件路径>
  
  #示例
  am profile start com.android.systemui /data/local/tmp/1.trace
  am profile stop com.android.systemui
  ps -ef | grep system_server #假设system_server的pid是454
  am profile start 454 /data/local/tmp/2.trace
  ```

  然后再AS中打开：

  ![image-20251218153821384](./imgs/profiler%E5%91%BD%E4%BB%A4/image-20251218153821384.png)

  也可以用PerfettoUI在线网站打开。

* perfetto(推荐，Android 10+，支持 Java/Native/ 系统调用）

  编写 Perfetto 配置文件（trace_config.pbtxt）。另见笔记-perfetto的部分。

  搜索引擎和AI提供了一些配置文件语法，都是错的。自行查阅 https://perfetto.dev/docs，或者用在线adb的方式吧。

  ```
  命令示例（https://perfetto.dev/docs/reference/heap_profile-cli）：
  #10ms dump一次
  python tools/heap_profile -n system_server -c 10 --all-heaps -d 50000 -o output
  ```

  在线adb示例：

  ![image-20251218183926731](./imgs/profiler%E5%91%BD%E4%BB%A4/image-20251218183926731.png)

  * webusb需要https
  * 谷歌浏览器。火狐未连接成功，或许哪里需要打开。

* systrace、dmtracedump(老旧 traceview)

  安卓9之前的老旧方法，已被perfetto取代。如有老项目需要，参见笔记：briefblog中的“04-分析工具.md”。