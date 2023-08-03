VS Code打开C++项目后，右下角会提示你安装几个C++的插件。但是使用一段时间发现，这个插件占用资源太大，动辄CPU一个核心100%或者IO占用100%。（我记不清是CPU还是硬盘百分百了。反正经常遇到，导致电脑卡顿，别的任务都做不了）

我打开的项目没几个cpp文件，按理不至于如此。但是我们的项目在AOSP里面，调用源码的其它模块，其它模块又调用其它模块。这样插件需要做的语法分析就非常大了。估计是这个原因。

听说不要用vs code推荐的那几个微软的插件，而是用安装`clangd`，性能好的多，据说代码补全等功能也好一些。具体如何我还没体验。

先把微软的C/C++插件卸了。然后就不是用aidegen命令生成项目配置文件（安卓AOSP里我们以前用aidegen命令生成c_cpp_properties.json，clangd不识别这个，这个微软的插件才用的）

然后搜索clangd:

![image-20230215190608712](_img/image-20230215190608712.png)

安装好后，如果你的电脑没有配置clangd软件，vscode里面会在右下角提示，点下载即可。

clangd下载在这个目录（windows）:

`C:\Users\xxxxxxyourname\AppData\Roaming\Code\User\globalStorage\llvm-vs-code-extensions.vscode-clangd\install\15.0.6\clangd_15.0.6`

嫌这个路径太长，我们可以手动复制到/opt目录下。



clangd 需要配合`compile_commands.json`，才能够有效提高代码跳转、补全等功能。

如果没有指定 compile_commands.json 位置，会在工程路径下进行查找，如需单独指定可以添加以下配置项 `--compile-commands-dir=${workspaceFolder}/build/`。下面给出配置文件的参考：

```json
//文件位置：<workspace>/.vscode/settings.json
{
  "files.associations": {
    "iostream": "cpp",
    "intrinsics.h": "c",
    "ostream": "cpp",
    "vector": "cpp"
  },
  // 开启粘贴保存自动格式化
  "editor.formatOnPaste": true,
  "editor.formatOnSave": true,
  "editor.formatOnType": true,
  "C_Cpp.errorSquiggles": "Disabled",
  "C_Cpp.intelliSenseEngineFallback": "Disabled",
  "C_Cpp.intelliSenseEngine": "Disabled",
  "C_Cpp.autocomplete": "Disabled", // So you don't get autocomplete from both extensions.
  "clangd.path": "/usr/bin/clangd",
  // Clangd 运行参数(在终端/命令行输入 clangd --help-list-hidden 可查看更多)
  "clangd.arguments": [
    // 让 Clangd 生成更详细的日志
    "--log=verbose",
    // 输出的 JSON 文件更美观
    "--pretty",
    // 全局补全(输入时弹出的建议将会提供 CMakeLists.txt 里配置的所有文件中可能的符号，会自动补充头文件)
    "--all-scopes-completion",
    // 建议风格：打包(重载函数只会给出一个建议）
    // 相反可以设置为detailed
    "--completion-style=bundled",
    // 跨文件重命名变量
    "--cross-file-rename",
    // 允许补充头文件
    "--header-insertion=iwyu",
    // 输入建议中，已包含头文件的项与还未包含头文件的项会以圆点加以区分
    "--header-insertion-decorators",
    // 在后台自动分析文件(基于 complie_commands，我们用CMake生成)
    "--background-index",
    // 启用 Clang-Tidy 以提供「静态检查」
    "--clang-tidy",
    // Clang-Tidy 静态检查的参数，指出按照哪些规则进行静态检查，详情见「与按照官方文档配置好的 VSCode 相比拥有的优势」
    // 参数后部分的*表示通配符
    // 在参数前加入-，如-modernize-use-trailing-return-type，将会禁用某一规则
    "--clang-tidy-checks=cppcoreguidelines-*,performance-*,bugprone-*,portability-*,modernize-*,google-*",
    // 默认格式化风格: 谷歌开源项目代码指南
    // "--fallback-style=file",
    // 同时开启的任务数量
    "-j=2",
    // pch优化的位置(memory 或 disk，选择memory会增加内存开销，但会提升性能) 推荐在板子上使用disk
    "--pch-storage=disk",
    // 启用这项时，补全函数时，将会给参数提供占位符，键入后按 Tab 可以切换到下一占位符，乃至函数末
    // 我选择禁用
    "--function-arg-placeholders=false",
    // compelie_commands.json 文件的目录位置(相对于工作区，由于 CMake 生成的该文件默认在 build 文件夹中，故设置为 build)
    "--compile-commands-dir=${workspaceFolder}/build"
  ]
}
```

一般我们没有这么专业的要求，不需要配。把**compile_commands.json**放到工程路径下即可。



**优点**
占用系统资源确实比 C/C++ 少了很多，无论是 CPU 还是 内存的使用。（最重要）

**缺点**
对于AOSP，操作相较于 C/C++ 确实也繁琐一点，因为不能用aidegen。

总结一下，就是对于大型项目来说，使用 clangd 能够明显降低系统资源的占用，能够减少出现系统卡顿的情况，不过使用门槛较高。C/C++ 更适合小项目的开发，使用起来更加便利，能够更好的进行调试。


## 如何生成`compile_commands.json`

### cmake工程

方法1、cmake工程生成 compile_commands.json 文件比较简单，定义 CMAKE_EXPORT_COMPILE_COMMANDS 即可。

```
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1
```


方法2、在CMakeLists.txt中添加 set(CMAKE_EXPORT_COMPILE_COMMANDS ON) 也可以达到上面的效果。

### makefile工程

不过很多(旧的)工程都是用 makefile 来编译的，没有现成的选项生成 compile_commands.json 文件。 虽然也可以使用 ctags, gtags 等，但是跳转其实不是很准确。 我们可以通过 Bear 来生成，而且不需要改动代码。 具体Bear的安装这里就不赘述了，按照 官方文档 来即可。 安装之后，执行以下命令即可生成：

```
bear 编译命令 # 比如说 bear make
```


也就是在原有编译命令之前加上 bear 即可，一般都是 bear make 。 生成之后我们就可以愉快地享受更精准的跳转和补全了。

### AOSP

先执行：

```
$ export SOONG_GEN_CMAKEFILES=1
$ export SOONG_GEN_CMAKEFILES_DEBUG=1
```

可以配置到.bashrc里，这样每次打开shell默认就有。

然后整编或单编后，会在out/development/ide/clion/目录生成CMakeLists.txt。然后用`cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1`生成compile_commands.json。

导入vscode时，先安装cmake插件：

![image-20230802112539838](_img/image-20230802112539838.png)

装好后，ctrl+shift+p,选择CMake:Quick Start,然后选择生成的CMakeLists.txt。然后会生成`.vscode/settings.json`。

![image-20230802112815344](_img/image-20230802112815344.png)

内容类似：

```
{
	"cmake.sourceDirectory":"xxxxx/xxx/xxx"//CMakeLists.txt的位置
}
```

我们自己把“--compile-commands-dir”加进去,最后内容如下：

```
{
	"cmake.sourceDirectory":"xxxxx/xxx/xxx",
    "clangd.arguments": [
        "--compile-commands-dir=build"
     ]
}
```

不过我觉得，指定`compile-commands`就已经够用了，不一定非要装cmake插件，也不一定需要配置“cmake.sourceDirectory”。
