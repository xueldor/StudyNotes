`adb shell 'touch /data/bootchart/enabled`

reboot重启后，执行命令

`$ANDROID_BUILD_TOP/system/core/init/grab-bootchart.sh`

因为grab-bootchart.sh是在安卓源码里。需要linux和源码环境。我们在windows上面可以这样：

adb shell, 然后到`/data/bootchart`目录下。

```
adb shell
cd /data/bootchart
#将bootchart目录里的文件压缩成bootchart.tgz文件
tar -czf bootchart.tgz .

#退出adb shell,回到windows，将tgz文件pull出来
adb pull /data/bootchart/bootchart.tgz
```

然后将windows上的程序“pybootchartgui.exe”跟“bootchart.tgz”放到一个目录里，执行“pybootchartgui.exe”，即自动生成bootchart.png。

