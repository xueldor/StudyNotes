# 有Ubuntu主机

如果有ubuntu主机，仍然优先用asfp，便捷简单，无需多说。有这条件的话，下文全部不需要看。

# 无Ubuntu主机，ssh远程

通常是采用VSCODE+ssh或者VSCODE+ssh+Dev Containers(进docker)

我们的目的是支持C++和JAVA的语法解析、智能提示、跳转。

> ps: 条件允许也可以采用远程桌面，继续使用asfp。
>
> 为了解决远程桌面的卡顿，尽量选择轻量级桌面。
>
> 见：“docker内部部署桌面”。

## C++

* 按照clangd。可以apt install安装，也可以从github下载，然后添加PATH。

* vscode中安装插件clangd。

  > 对于比较旧的安卓版，clangd不支持debug，可用微软的官方C++插件, 此时使用的编译器是gcc，无需上一步的下载部署clangd。个人经验性能不如clangd。
  >
  > 高版本clangd应该也是可以debug的，暂时没试过。

  插件clangd和微软的C++插件是冲突的，只能启用一个。

* 后续有compile_commands.json和CMakeLists.txt两种方法

### compile_commands

生成compile_commands.json，拷贝到打开的目录。也可以在 settings.json 中指定绝对路径：

```json
  {
      "clangd.arguments": [
          "--compile-commands-dir=/home/用户名/android/aosp",  // 指定绝对路径
          // 其他参数可按需添加，正常不要管...
              "--background-index",
              "--all-scopes-completion",
              "--pch-storage=memory",
              "--header-insertion=never",
              "--limit-references=10000"
      ]，
      "C_Cpp.intelliSenseEngine": "disabled",  // 必须禁用，与 clangd 冲突
  }
```

生成compile_commands.json方法很多，后文介绍一些。

### CMakeLists.txt

* 安装CMake插件
* vscode打开的目录应该包含Android.bp,通过bp自动生成CMakeLists.txt
* 打开这个目录即可


## java

* 首先需要安装jdk，这里我直接引用aosp源码里自带的：

```
/home/xue/p32s/lagvm/LINUX/android/prebuilts/jdk/jdk11/linux-x86
```

方法是通过settings.json里添加 java.configuration.runtimes参数指定，无需记忆，vscode自动会提示

```
"java.configuration.runtimes": [
      {
          "name": "JavaSE-11",
          "path": "/home/xue/p32s/lagvm/LINUX/android/prebuilts/jdk/jdk11/linux-x86"
      },
  ]，
  // 后面大概率会报内存溢出，然后vscode会自动帮你更新下面参数，可酌情调大一点：
  "java.jdt.ls.vmargs": "-XX:+UseParallelGC -XX:GCTimeRatio=4 -XX:AdaptiveSizePolicyWeight=90 -Dsun.zip.disableMemoryMapping=true -Xmx4G -Xms100m -Xlog:disable"
```

* 插件的话，安装Extension Pack for Java里面的Language Support for Java(TM) by Red Hat和Project Manager for Java即可。其它不必要装，装越多越卡。

* 在vscode打开的目录下，创建.project和.classpath两个文件。可以手动创建、从别的地方拷贝过来，也可自动生成。

  `.project`的作用是，如果没有这个文件，`Language Support for Java(TM) by Red Hat`默认按照gradle和Maven的方式构建项目，会不停扫描寻找gradle文件和pom文件，并自动后台下载gradle依赖文件，浪费很多时间，性能低下。aosp不是gradle构建的。必然构建失败，所以我们不要触发gradle和maven。

  `.classpath`的作用是，告诉插件，src目录、依赖哪些libs等信息。

  .project和.classpath两个文件实际上是eclipse的项目配置文件。也就是说我们实质是用vscode识别eclipse项目。可能有人希望能识别Android Studio的格式（即gradle）或jetbrains idea（即iml），前者，没有办法把aosp里的项目从Android.bp自动生成build.gradle；后者，aosp倒是支持生成iml，但是目前vscode里没有插件能够识别解析iml。因此是不可行的。

  > "aosp倒是支持生成iml",所以可以导入idea。但是，如前文所言，如果有本地Ubuntu主机，那么优先用你asfp，用不到这种方法；如果没有，依赖ssh连接，这方法也没用。
  >
  > idea旗舰版貌似也提供了ssh远程开发的功能，经常卡死，远不如vscode的ssh插件，用不着。

  后文介绍一些自动生成这两个文件的方法。

# 无Ubuntu主机，windows本地

方案一，通过samba或类似机制，把远程linux目录映射到本地windows磁盘。网络延迟导致很卡，并不推荐。】

方案二，在远程服务器上生成iml、ipr文件，然后把这两个文件下载下来，编辑文件，去掉不关心的源码路径，以提升性能，然后用android studio打开。C++类似。

当然，如果你是用的eclipse打开，那就是生成.project和.classpath了。

# 如何生成compile_commands

* 已有cmake工程

  cmake命令指定 CMAKE_EXPORT_COMPILE_COMMANDS 即可。

  ```
  cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1
  ```


  方法2、在CMakeLists.txt中添加 set(CMAKE_EXPORT_COMPILE_COMMANDS ON) 也可以达到上面的效果。

* AOSP里的单个项目

  先执行：

  ```
  $ export SOONG_GEN_CMAKEFILES=1
  $ export SOONG_GEN_CMAKEFILES_DEBUG=1
  ```

  可以配置到.bashrc里，这样每次打开shell默认就有。

  然后整编或单编后，会在out/development/ide/clion/目录生成CMakeLists.txt。然后用`cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1`生成compile_commands.json。

  既然这种方法也要先生成CMakeLists.txt，那么我觉得还不如通过cmake插件，省去从cmakelist到compile_commands这步。

* AOSP全体

  ```
  export SOONG_GEN_COMPDB=1
  export SOONG_GEN_COMPDB_DEBUG=1
  ```

  会输出out/soong/development/ide/compdb/compile_commands.json。文件大约200+M，解析、索引需要很长时间，因此不建议一次性全部导入。尽量采取上面单个项目导入的方法。

# 如何生成.classpath

## aidegen

用aidegen生成,以frameworks/base/为例：

```
// 先lunch target
aidegen -i e -n frameworks/base/ -s  // base目录下会生成.project和.classpath
aidegen -i e -n frameworks/base/ -s -a //-a会同时在aosp根目录生成总的.project和.classpath
```

由于frameworks/base里面文件很多，打开后会构建较长时间。可以打开classpath文件，手动删除一些内容。比如通常我们不关注test相关的代码，可以把这部分删掉。依赖库也可以酌情删。

“-i e”后面的e表示eclipse。可以aidegen --help查看还支持哪些其它ide。idea、clion、vscode都在其中。

## idegen(废弃)

谷歌已不维护idegen。aidegen可代替，且功能更加细化。

```
source build/envsetup.sh
lunch [你的目标]
mmm development/tools/idegen/
sudo development/tools/idegen/idegen.sh #生成整个aosp全部的
```



本文是对前些年，其它相关笔记的提炼总结和补充，并确保能用于最新项目。更细节的讲解参考当前目录下的其它笔记。+

# 补充信息

下面补充一些可能有用信息。

* 假如发现已经触发了gradle，settings.json添加配置可以手动关闭：

```json
{
  "java.maven.downloadSources": false,
  "java.maven.updateSnapshots": false,
  // 禁用 Gradle 自动检测和解析
  "java.import.gradle.enabled": false,
  "java.import.maven.enabled":false,

   "java.configuration.updateBuildConfiguration": "disabled",
  "java.autobuild.enabled": false,
  "java.configuration.runtimes": [
      {
          "name": "JavaSE-11",
          "path": "/home/xuexiangyu/p32s/lagvm/LINUX/android/prebuilts/jdk/jdk11/linux-x86"
      },
  ],
  "java.project.sourcePaths": [], // 清空，避免干扰 .classpath
  "files.exclude": {
        "**/build.gradle": false,
        "**/settings.gradle": false,
        "**/.project": true,
        "!/project/.project": false,
        "**/.classpath": true,
        "!/project/.classpath": false,
        "**/.settings": true,
        "!/project/.settings": false
  },
  "files.watcherExclude": {
    // 禁止 VS Code 监控子目录的配置文件变化，避免触发扫描
    "**/node_modules/**": true,
    "**/.git/**": true,
    "**/[^.]/.project": true,
    "**/[^.]/.classpath": true
  },
  "java.import.gradle.wrapper.enabled": false,
  "java.import.gradle.version": ""
}
```

重点是最前面的几个false，files.exclude不那么重要。重新打开。

* frameworks/base里面文件很多，通过我们只关注services/core和core/java少数几个目录，framework的核心接口大部分在这里。

  可以在这些包含bp的子目录里生成项目文件，然后vscode以workspace形式添加这些子项目。

* 手动在根目录新建.classpath文件和.project文件：

```
<?xml version="1.0" encoding="UTF-8"?>
<classpath>
    <classpathentry kind="src" path="core/java"/>
    <classpathentry kind="src" path="services/core/java"/>
    <classpathentry kind="src" path="media/java"/>
    <classpathentry kind="src" path="location/java"/>
    <classpathentry kind="con" path="org.eclipse.jdt.launching.JRE_CONTAINER"/>
    <!-- out目录不要放在当前路径，因为会导致bp文件重复(会拷贝一份bp到out),造成make失败， -->
    <classpathentry kind="output" path="../../out"/>
</classpath>
```

.project文件：

```
<?xml version="1.0" encoding="UTF-8"?>
<projectDescription>
        <name>frameworks-base</name>
        <comment>P32S Framework</comment>
        <projects>
        </projects>
        <buildSpec>
                <buildCommand>
                        <name>org.eclipse.jdt.core.javabuilder</name>
                        <arguments>
                        </arguments>
                </buildCommand>
        </buildSpec>
        <natures>
                <nature>org.eclipse.jdt.core.javanature</nature>
        </natures>
        <filteredResources>
                <filter>
                        <id>1774014295189</id>
                        <name></name>
                        <type>30</type>
                        <matcher>
                                <id>org.eclipse.core.resources.regexFilterMatcher</id>
                                <arguments>node_modules|\.git|__CREATED_BY_JAVA_LANGUAGE_SERVER__</arguments>
                        </matcher>
                </filter>
        </filteredResources>
</projectDescription>
```

* 关于.project文件的，插件市场搜eclipse,应该有插件能帮忙自动生成。嫌aidegen耗时的话，可以试试。适用于需要快速导入vscode快速上手。我想不出什么理由需要手动创建。

