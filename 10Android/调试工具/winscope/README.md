平台： ATC AC8025，android 12

路径：

> development/tools/winscope
> prebuilts/misc/common/winscope/

* 离线模式：adb shell里用命令抓trace，然后网页上打开trace文件
* 在线模式：网页上连接设备，点击界面按钮实时抓取trace。

离线模式Ubuntu和windows都可以直接使用。

在线模式Ubuntu可以直接使用，而windows上面，`winscope_proxy.py`原生未适配windows。需要按以下手顺：

1. 将上面两个路径下载到windows上面
2. winscope_proxy.py打上windows.patch，以便在windows上执行。
3. 安装python，版本不低于3.5
4. adb添加到环境变量（打开cmd，能直接找到adb和python命令）
5. 执行python ./tools/winscope/adb_proxy/winscope_proxy.py，不要关闭这个窗口。
6. 浏览器打开winscope.html，按界面提示操作。过程傻瓜式，无需多讲。

按需勾选，全勾上可能会导致车机运行不流畅。



windows.patch处理了两个问题：

1. windows和linux换行符差异。
2. windows不支持信号量



手动模式命令：

```
#window manager trace
wm tracing start 
wm tracing stop

# 输入法 trace
ime tracing start 
ime tracing stop

# SurfaceFlinger trace
service call SurfaceFlinger 1025 i32 1 #启用
service call SurfaceFlinger 1025 i32 0 #停用
```

adb pull /data/misc/wmtrace,
然后winscope.html打开文件
