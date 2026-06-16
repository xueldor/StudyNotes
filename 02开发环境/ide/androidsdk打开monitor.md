monitor是eclipse的界面，以前用eclipse开发安卓可以直接从eclipse打开ddms界面。但是现在都用android studio。要打开monitor，需要手动去sdk/tools/lib/monitor-x86_64目录运行。

问题是，这个工具是，谷歌已放弃eclipse，这个工具直接启动会报错。原因是，这个工具需要java 8启动。而现在电脑上装的肯定是较高的版本。

因为这个工具是比较好用的，感觉studio提供的分析工具，对开发单个app还行，系统开发还不如monitor。所以不愿意放弃。

解决：

先下载一份jdk8,假设路径是：D:\soft\java8\jdk8u302-b08

进入目录android-sdk\tools\lib\monitor-x86_64

打开monitor.ini

在-vmargs的前面添加一行：

```
-vm
D:\soft\java8\jdk8u302-b08\bin\javaw.exe
```

好像有些地方不需要加javaw.exe，原因不明。比如我在一台ubuntu上面，这样就行：

```
@noDefault
-vm
/home/xue/softs/jdk8u422-b05/bin/
```

后面加javaw.exe反而启动报错。

完整：

```
-startup
plugins/org.eclipse.equinox.launcher_1.3.0.v20120522-1813.jar
--launcher.library
plugins/org.eclipse.equinox.launcher.win32.win32.x86_64_1.1.200.v20120913-144807
-data
@noDefault
-vm
D:\soft\java8\jdk8u302-b08\bin\javaw.exe
-vmargs
-XX:MaxPermSize=256m
-Xms512m
-Xmx1024m

```
