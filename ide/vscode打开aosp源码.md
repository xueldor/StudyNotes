https://github.com/amezin/aosp-vscode

1. 整编AOSP源码，make成功

2. 将c_cpp_properties.json、generate_compdb.py、settings.json三个文件拷贝到`<AOSP>/.vscode/`

3. 在make的那个窗口下，执行`python .vscode/generate_compdb.py out/build-${TARGET_PRODUCT}.ninja`

4. ,然后用vscode打开源码目录

5. "compilerPath": "${workspaceFolder}/prebuilts/clang/host/linux-x86/clang-3289846/bin/clang++",改成项目实际的clang++路径，一般就是clang-3289846后面的数字不一样。

6. 如果有个别的项目，配置里没有，可以手动添加:

   ```json
   "includePath": [
       "${workspaceFolder}/**"
   ]
   ```

   如此包含了所有文件

7. 理论上，对所有使用ninja编译的aosp版本都适用。



Google官方方法

可以导入特定模块

```shell
source build/envsetup.sh
lunch <target>
aidegen -i v -n frameworks/base/
aidegen -i v -n vendor/hsae/hardware/interfaces/
```

通过这个命令可以在模块目录下生成vscode的配置代码，和一个workspace文件。打开workspace方法：file-从文件打开工作区。

-i v :指定ide为vscode

-n： not launch，如果不加-n，执行完会自动打开vscode。

-s： 如果前面整编过，那么可以加-s，跳过一些步骤，速度快一点。

-l:   Language， j: Java, c: C/C++, r: Rust

问题：

生成的配置文件里，会有大量${ANDROID_BUILD_TOP}，这是source build/envsetup.sh导入的。直接启动vscode的话，不能识别此变量。

解决方法：

方法一： 我们应该在终端里先执行source build/envsetup.sh&&lunch，然后执行code命令打开vscode。

方法二：配置文件，全局替换，${ANDROID_BUILD_TOP}替换为实际路径。



很多时候，我们的桌面环境是windows，不具备编译源码的条件，但是可以用ssh连接远程代码服务器，这种情况下，解决办法有：

1. 模块代码下载到windows的情况：代码服务端安装Samba，这样windows里就可以用`\\192.168.20.115\Share\xxx`的形式访问。然后在模块代码目录建一个“start vscode.bat”，内容如下：

```bash
@echo off
echo make sure you have installed vs code in your computer
echo and run this file,then will automatically open this workspace

@echo off
if "%1" == "h" goto begin
mshta vbscript:createobject("wscript.shell").run("""%~nx0"" h",0)(window.close)&&exit
:begin

#set ANDROID_BUILD_TOP=\\192.168.22.230\share\work\ccs5.0\apps\LINUX\android
set ANDROID_BUILD_TOP=\\192.168.20.115\Share\xuexiangyu\apps\LINUX\android

code -n -a vendor.hsae.hardware.interfaces.code-workspace
#把-a的参数修改为你自己的、aidegen生成的工作空间文件
```

用此脚本打开vscode。

2. 模块代码不下载到本地，vscode用remote-ssh插件的情况：

这种情况没什么好的办法。要么全局替换配置文件；要么编辑`~/.bashrc`, 加上

```shell
cd ~/apps/LINUX/android
source build/envsetup.sh
lunch 35
```

然后重启，这样你打开的每个会话都自动执行以上命令，vscode连接上后，自然就有ANDROID_BUILD_TOP变量。这样做，如果有多个人用同一个账号连接此服务器，会影响到别人。