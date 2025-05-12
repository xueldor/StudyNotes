`adb shell 'touch /data/bootchart/enabled'`

touch /data/bootchart/enabled

reboot重启后，执行命令

`$ANDROID_BUILD_TOP/system/core/init/grab-bootchart.sh`

因为grab-bootchart.sh是在安卓源码里。需要linux和源码环境。我们在windows上面可以这样：

adb shell, 然后到`/data/bootchart`目录下。

```
adb shell
cd /data/bootchart
#将bootchart目录里的文件压缩成bootchart.tgz文件
tar -zcf bootchart.tgz *

#退出adb shell,回到windows，将tgz文件pull出来
adb pull /data/bootchart/bootchart.tgz
```

然后将windows上的程序“pybootchartgui.exe”跟“bootchart.tgz”放到一个目录里，执行“pybootchartgui.exe”，即自动生成bootchart.png。

在Ubuntu上，需要安卓pybootchartgui：

`sudo apt install pybootchartgui`

然而ubuntu22移除了这个软件，理论上可以自己从github下载源码，make、make install安装，然而我并不成功，总是有这样那样问题，要不make报错，要不make install成功后，指令执行有问题。

因此高版本的Ubuntu建议使用docker模拟一个Ubuntu18的环境。



#### 问题

disk那栏没有的问题：

原因是没有正确解析diskstats。

我们试着在Ubuntu上面用指令生成bootchart.png:

`pybootchartgui bootchart.tgz`, 你发现报一个错误：

除0错误：ZeroDivisionError: float division by zero

Traceback (most recent call last):
  File "/usr/bin/pybootchartgui", line 23, in <module>
    sys.exit(main())
  File "/usr/lib/python2.7/dist-packages/pybootchartgui/main.py", line 137, in main
    render()
  File "/usr/lib/python2.7/dist-packages/pybootchartgui/main.py", line 128, in render
    batch.render(writer, res, options, filename)
  File "/usr/lib/python2.7/dist-packages/pybootchartgui/batch.py", line 41, in render
    draw.render(ctx, options, *res)
  File "/usr/lib/python2.7/dist-packages/pybootchartgui/draw.py", line 282, in render
    draw_chart(ctx, IO_COLOR, True, chart_rect, [(sample.time, sample.util) for sample in disk_stats], proc_tree)
  File "/usr/lib/python2.7/dist-packages/pybootchartgui/draw.py", line 201, in draw_chart
    yscale = float(chart_bounds[3]) / max(y for (x,y) in data)

原因是从我的机器上抓出来的/proc/diskstats是20列，而pybootchartgui是按照14列的解析的。

做以下修改：

/usr/share/pyshared/pybootchartgui/目录的draw.py，parsing.py，samples.py三个文件

```python
draw.py：
将200，201行由：    
xscale = float(chart_bounds[2]) / max(x for (x,y) in data)    
yscale = float(chart_bounds[3]) / max(y for (x,y) in data)    
改为：    
xscale = float(chart_bounds[2]) / max(0.00001, max(x for (x,y) in data))    
yscale = float(chart_bounds[3]) / max(0.00001, max(y for (x,y) in data))   

parsing.py：
在156行后添加：    
    if interval == 0:    
        interval = 1  
修改后如下：    
    sums = [ a - b for a, b in zip(sample1.diskdata, sample2.diskdata) ]   
    if interval == 0:  
        interval = 1  



samples.py：
在81行后添加：    
    if interval == 0:    
        interval = 1  
修改后如下：  
    def calc_load(self, userCpu, sysCpu, interval):  
        if interval == 0: 
            interval = 1  
            

```

这样就解决了除0错误。但这样还不够，这样只是可以生成png，但是disk那栏没有。还需要把diskstats文件的列数改成我们实际的列数20：

```
parsing.py：
这里14改成20:
def _parse_proc_disk_stat_log(file, numCpu):
...
  def is_relevant_line(linetokens):
    return len(linetokens) == 14 and re.match(DISK_REGEX, linetokens[2])

进一步修正。实际上不同平台到底是多少列是有差异的。郑州日产ATC平台是20列，高通8155 N626项目是18列。所以需要智能地根据实际情况判断。或者可以干脆把这个判断去掉。

```

windows上“pybootchartgui.exe”解析出来没有disk也是同样原因（认为列数应该是14，当判断文件里列数是20时，直接跳过解析diskstats ）。



第二个问题，有些bootchart.tgz 文件，部分正确解析，报如下错：

```
parsing 'bootchart.tgz'
parsing '.'
parsing './proc_diskstats.log'
parsing './header'
parsing './enabled'
parsing './proc_stat.log'
parsing './proc_ps.log'
Parse error: empty state: 'bootchart.tgz' does not contain a valid bootchart
```

这是因为我们这个压缩文件是在linux下用命令压缩生成的。windows下用压缩软件生成没这个问题。

代码层面原因是：linux的压缩文件，在python里遍历压缩包是，取到的name是：“./header”格式，而代码判断是：“name=='header'”。把前面的“./”去掉即可：

```
def _do_parse(writer, state, name, file):
    name = os.path.basename(name)  <-----------here,去掉./
    writer.status("parsing '%s'" % name)
    t1 = perf_counter()
    if name == "header":
```

#### 总结

bootchart的最新版本已经修复了除0问题，我们只需要修复列数是14问题以及格式是"./name"的问题。

我们可以下载最新版本（0.14.9， 更新于2020年，应该不会继续维护）。然后修改一下列数即可：

```
    def is_relevant_line(linetokens):
        if len(linetokens) != 20:
            return False
        disk = linetokens[2]
        return disk_regex_re.match(disk)
        
改成
    def is_relevant_line(linetokens):
        if len(linetokens) == 14 or len(linetokens) == 18 or len(linetokens) == 20:
                disk = linetokens[2]
                return disk_regex_re.match(disk)
        else: return False
或者干脆直接去掉判断列数：
    def is_relevant_line(linetokens):
#        if len(linetokens) == 14 or len(linetokens) == 18 or len(linetokens) == 20:
                disk = linetokens[2]
                return disk_regex_re.match(disk)
#        else: return False


然后修改name格式问题：
def _do_parse(writer, state, name, file):
    name = os.path.basename(name)  <-----------here,去掉./
    writer.status("parsing '%s'" % name)
    t1 = perf_counter()
    if name == "header":
```

使用命令：

`./pybootchartgui.py bootchart.tgz`

这里有几个警告，不影响运行，如果有强迫症，可以修改一下：

```
/media/xue/workspace/work/bootchart-0.14.9/pybootchartgui/parsing.py:454: SyntaxWarning: invalid escape sequence '\d'
  disk_regex_re = re.compile ('^([hsv]d.|mtdblock\d|mmcblk\d|cciss/c\d+d\d+.*)$')
/media/xue/workspace/work/bootchart-0.14.9/pybootchartgui/parsing.py:523: SyntaxWarning: invalid escape sequence '\['
  timestamp_re = re.compile ("^\[\s*(\d+\.\d+)\s*]\s+(.*)$")
/media/xue/workspace/work/bootchart-0.14.9/pybootchartgui/parsing.py:524: SyntaxWarning: invalid escape sequence '\S'
  split_re = re.compile ("^(\S+)\s+([\S\+_-]+) (.*)$")
/media/xue/workspace/work/bootchart-0.14.9/pybootchartgui/parsing.py:569: SyntaxWarning: invalid escape sequence '\@'
  p = re.match ("\@ (\d+)", rest)
/media/xue/workspace/work/bootchart-0.14.9/pybootchartgui/parsing.py:101: SyntaxWarning: "is" with 'int' literal. Did you mean "=="?
  if pid is 0:

```

前几个意思是，正则表达式里的'\d'，可能会被解释为转义字符。我们在前面加个r就可以了，\d 就不会被解释为转义字符，而是作为正则表达式的一部分。

`re.compile (r'^([hsv]d.|mtdblock\d|mmcblk\d|cciss/c\d+d\d+.*)$')`  <-------------------单引号前面加个r

最后一个意思是，比较数字轻易==而不是is。is 在 Python 中用于比较两个对象的身份（即它们是否是同一个对象），而不是它们的值。不懂为什么维护代码的人会写出这样的代码。

(附上改好的pybootchartgui-0.14.9压缩包)



最后，我们把修改好的pybootchartgui.py， 用pyinstaller打包成windows系统格式的可执行文件“pybootchartgui.exe”,即前文使用的“pybootchartgui.exe”程序。

1. 需要在windows系统安装python3、pip、pyinstaller。因为windows上打包出来是windows格式，linux上面打包出来是linux格式，不支持跨平台打包。开始因为不知道这点，我在ubuntu上面解决了很多环境的问题，最后搞出来一个linux的可执行程序，白费劲。

2. 安装颇费了些周折，下面是一些指令：

   ```
   C:\Users\xxx\AppData\Local\Programs\Python\Python313\python.exe -m pip install --upgrade pip
   C:\Users\xxx\AppData\Local\Programs\Python\Python313\python.exe -m pip install pyinstaller
   C:\Users\xxx\AppData\Local\Programs\Python\Python313\python.exe -m pip install pycairo
   
   # 一般直接执行pip install就可以了，但有时你的电脑环境没配置好，直接执行pip找不到，这就需要前面加python.exe -m
   ```

3. 找不到PyInstaller命令，python.exe -m PyInstaller代替：

   ```
   # 注意确保python命令指向的是python3
   python.exe -m PyInstaller -F pybootchartgui.py
   ```

   -F打包成单文件。-i 0.ico指定图标。

4. 执行3之前，先编辑main.py, main函数里修改一下不传参数时的默认文件。默认是/var/log/bootchart.tgz，在windows上肯定不行，改成默认当前目录。



更新：

ubuntu18上的旧版pybootchartgui好像是用python2.7写的，PyInstaller环境搞不起来各种异常，不要折腾了。

https://gitee.com/os-workspace/pybootchartgui/

https://github.com/xrmx/bootchart
