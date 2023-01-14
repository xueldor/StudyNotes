## 第一步

mm定义在 `build/envsetup.sh`

```shell
function mm()
(
    _trigger_build "modules-in-a-dir-no-deps" "$@"
)
function _trigger_build()
(
   #local: 定义一个局部变量； -r:只读； $1: 第一个参数，即"modules-in-a-dir-no-deps"
   # shift语句:参数左移一个位置，即将$1丢弃，$2变成$1,$3变成$2。
    local -r bc="$1"; shift
    if T="$(gettop)"; then
      _wrap_build "$T/build/soong/soong_ui.bash" --build-mode --${bc} --dir="$(pwd)" "$@"
    else
      echo "Couldn't locate the top of the tree. Try setting TOP."
    fi
)
```

`_wrap_build`只是包裹一下命令，在前面和结尾加一些辅助信息：

```shell
function _wrap_build()
{
    if [[ "${ANDROID_QUIET_BUILD:-}" == true ]]; then
      "$@"
      return $?
    fi
    local start_time=$(date +"%s")
    
    # $@是传进来参数，即 "$T/build/soong/soong_ui.bash" --build-mode --${bc} --dir="$(pwd)" "$@" 
    # 传进来的参数，当做一个命令来执行
    "$@"
    #执行结束后，打印结束时间
    local ret=$?
    local end_time=$(date +"%s")
    local tdiff=$(($end_time-$start_time))
    local hours=$(($tdiff / 3600 ))
    local mins=$((($tdiff % 3600) / 60))
    local secs=$(($tdiff % 60))
    local ncolors=$(tput colors 2>/dev/null)
    if [ -n "$ncolors" ] && [ $ncolors -ge 8 ]; then
        color_failed=$'\E'"[0;31m"
        color_success=$'\E'"[0;32m"
        color_reset=$'\E'"[00m"
    else
        color_failed=""
        color_success=""
        color_reset=""
    fi
    echo
    if [ $ret -eq 0 ] ; then
        echo -n "${color_success}#### build completed successfully "
    else
        echo -n "${color_failed}#### failed to build some targets "
    fi
    if [ $hours -gt 0 ] ; then
        printf "(%02g:%02g:%02g (hh:mm:ss))" $hours $mins $secs
    elif [ $mins -gt 0 ] ; then
        printf "(%02g:%02g (mm:ss))" $mins $secs
    elif [ $secs -gt 0 ] ; then
        printf "(%s seconds)" $secs
    fi
    echo " ####${color_reset}"
    echo
    return $ret
}
```

假设源代码放在`/work/aosp`目录，执行mm的当前目录是`packages/apps/TestApp`,那么把 `"$T/build/soong/soong_ui.bash" --build-mode --${bc} --dir="$(pwd)" "$@"`里的参数展开，命令就是：

`/work/aosp/build/soong/soong_ui.bash --build-mode --modules-in-a-dir-no-deps --dir=packages/apps/TestApp`

后文中，源码目录还用$TOP表示。

于是，关注`soong_ui.bash`,主要内容：

```shell
function gettop
{
。。。返回代码根路径
}

# Save the current PWD for use in soong_ui
export ORIGINAL_PWD=${PWD}
export TOP=$(gettop)
source ${TOP}/build/soong/scripts/microfactory.bash

#在out目录下生成 soong_ui 文件，soong_ui是一个 ELF 格式的可执行程序
soong_build_go soong_ui android/soong/cmd/soong_ui

#回到源码根目录
cd ${TOP}
#执行 out/soong_ui --build-mode --modules-in-a-dir-no-deps --dir=packages/apps/TestApp
exec "$(getoutdir)/soong_ui" "$@"
```

为了证明分析的正确性，我先把out/soong_ui文件删除，然后执行`soong_build_go soong_ui android/soong/cmd/soong_ui`,果然这个程序又生成出来了。

然后证明最终执行的命令就是“out/soong_ui --build-mode --modules-in-a-dir-no-deps --dir=XXX”,我随便找了一个工程，验证模块“vendor/hsae/hardware/interfaces/CustomSetting”：

```shell
 work@ubuntu-cts:~/ccs5.0/apps/LINUX/android$ $TOP/out/soong_ui --build-mode --modules-in-a-dir-no-deps --dir=vendor/hsae/hardware/interfaces/CustomSetting
```

编译通过，没有任何问题。

## `soong_ui` 程序

既然shell脚本的最后是执行soong_ui命令，所以有必要研究一下这个程序。

### 构建

```shell
soong_build_go soong_ui android/soong/cmd/soong_ui
```

`soong_build_go` 实际上是一个对 `build_go()` 函数的调用封装，定义了`build_go`需要用到的变量后，直接调用`build_go`函数。所以以上语句等价于：

 ```shell
build_go soong_ui android/soong/cmd/soong_ui
 ```

第一参数 `soong_ui` 是指定了编译生 成的可执行程序的名字；第二个参数意义不明。跟进到build_go函数内部，关键一行：

```shell
GOROOT=$(cd $GOROOT; pwd) ${mf_cmd} -b "${mf_bin}" \
         -pkg-path "github.com/google/blueprint=${BLUEPRINTDIR}" \
         -trimpath "${SRCDIR}" \
         ${EXTRA_ARGS} \
         -o "${built_bin}" $2
```

$GOROOT=/home/work/ccs5.0/apps/LINUX/android/prebuilts/go/linux-x86/，空格后的命令是：

```shell
/work/android/out/microfactory_Linux -b /work/android/out/microfactory_Linux -pkg-path github.com/google/blueprint=/work/android/build/blueprint -trimpath /work/android -pkg-path android/soong=/work/android/build/soong -pkg-path github.com/golang/protobuf=/work/android/external/golang-protobuf -o /work/android/out/soong_ui android/soong/cmd/soong_ui
```

/work/android是源码路径，即`$TOP=/work/android`。

我们不去深研go语言，也暂时不探讨microfactory_Linux是什么。循着这个已有线索，找到soong_ui代码位于`build/soong/cmd/soong_ui/`, 打开这个目录的`Android.bp`文件

```
blueprint_go_binary {
    name: "soong_ui",
    deps: [
        "soong-ui-build",
        "soong-ui-logger",
        "soong-ui-terminal",
        "soong-ui-tracer",
    ],
    srcs: [
        "main.go",
    ],
}
```

所以，soong_ui 的主文件（程序入口）是 build/soong/cmd/soong_ui/main.go。

回顾前面的命令：

```
soong_ui --build-mode --modules-in-a-dir-no-deps --dir=vendor/hsae/hardware/interfaces/CustomSetting
```

代码里搜索“--build-mode”、“--modules-in-a-dir-no-deps”关键字，有以下几处描述：

```go
flag:        "--build-mode",
description: "build modules based on the specified build action",


"usage: %s --build-mode --dir=<path> <build action> [<build arg 1> <build arg 2> ...]\n\n"


// This is redirecting to mma build command behaviour. Once it has soaked for a
// while, the build command is deleted from here once it has been removed from the
// envsetup.sh.
name:        "modules-in-a-dir-no-deps",
description: "Build action: builds all of the modules in the current directory without their dependencies.",
action:      build.BUILD_MODULES_IN_A_DIRECTORY,
```

因为不懂go语言，暂时无法深入。根据网上查阅到的一些资料，main函数解析命令行参数，最后执行的是“build/soong/ui/build/build.go”里的“Build”方法：

```go
// Build the tree. The 'what' argument can be used to chose which components of
// the build to run.
func Build(ctx Context, config Config, what int) {
	......
	if config.SkipMake() {
		ctx.Verboseln("Skipping Make/Kati as requested")
		what = what & (BuildSoong | BuildNinja)
	}
   ......
	if what&BuildProductConfig != 0 {
		// Run make for product config
		runMakeProductConfig(ctx, config)
	}
    ......

	if what&BuildSoong != 0 {
		// Run Soong
		runSoong(ctx, config)
	}

	if what&BuildKati != 0 {
		// Run ckati
		genKatiSuffix(ctx, config)
		runKatiCleanSpec(ctx, config)
		runKatiBuild(ctx, config)
		runKatiPackage(ctx, config)

		ioutil.WriteFile(config.LastKatiSuffixFile(), []byte(config.KatiSuffix()), 0777)
	} else {
		...
	}

	// Write combined ninja file
	createCombinedBuildNinjaFile(ctx, config)

	if what&RunBuildTests != 0 {
		testForDanglingRules(ctx, config)
	}

	if what&BuildNinja != 0 {
		if !config.SkipMake() {
			installCleanIfNecessary(ctx, config)
		}
		// Run ninja
		runNinja(ctx, config)
	}
}
```

1. runSoong, 汇总和解析Android.bp, 和`out/soong/.bootstrap/build.ninja`、`out/soong/.minibootstrap/build.ninja`一起经`soong_build`汇总到out/soong/build.ninja文件；

2. runKatiBuild， 加载所有的makefile文件如“main.mk”、“Android.mk”,转成“build-*.ninja”文件。系统整编时，文件命名是`build-<product_name>.ninja`；

3. runKatiPackage， 生成“build-*-package.ninja”文件，这个文件是什么未知。

4. createCombinedBuildNinjaFile， 生成`combined-<product_name>.ninja`文件，这个文件就是简单的包含了前面的几个文件：

   ```ninja
   
   builddir = out
   pool highmem_pool
    depth = 1
   build _kati_always_build_: phony
   subninja out/build-msmnile_au.ninja
   subninja out/build-msmnile_au-package.ninja
   subninja out/soong/build.ninja
   ```

   

5. runNinja，执行ninja命令，构建模块



