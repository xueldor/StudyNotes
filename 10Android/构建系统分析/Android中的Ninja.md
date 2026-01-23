# Android中的Ninja

## Android中的Ninja文件

从Android 7开始，编译时默认使用[Ninja](https://www.codeprj.com/link/aHR0cHM6Ly9uaW5qYS1idWlsZC5vcmcv)。 但是，Android项目里是没有`.ninja`文件的。 遵循[Ninja](https://www.codeprj.com/link/aHR0cHM6Ly9uaW5qYS1idWlsZC5vcmcv)的设计哲学，编译时，会先把Makefile通过[kati](https://www.codeprj.com/link/aHR0cHM6Ly9naXRodWIuY29tL2dvb2dsZS9rYXRp)转换成`.ninja`文件，然后使用`ninja`命令进行编译。 这些`.ninja`文件，都产生在`out/`目录下，共有三类。

一类是`build-*.ninja`文件，通常非常大，几十到几百MB。 对`make`全编译，命名是`build-<product_name>.ninja`。 如果Makefile发生修改，需要重新产生[Ninja](https://www.codeprj.com/link/aHR0cHM6Ly9uaW5qYS1idWlsZC5vcmcv)文件。

这里Android有一个bug，或者说设计失误。 `mm`、`mma`的Ninja文件，命名是`build-<product_name>-<path_to_Android.mk>.ninja`。 而`mmm`、`mmma`的Ninja文件，命名是`build-<product_name>-_<path_to_Android.mk>.ninja`。 显然，不同的单模块编译，产生的也是不同的[Ninja](https://www.codeprj.com/link/aHR0cHM6Ly9uaW5qYS1idWlsZC5vcmcv)文件。

这个设计本身就有一些问题了，为什么不同模块不能共用一个总的Ninja文件？ 这大概还是为了兼容旧的Makefile设计。 在某些Android.mk中，单模块编译与全编译时，编译内容截然不同。 如果说这还只能算是设计失误的话，那么`mm`与`mmm`使用不同的编译文件，就是显然的bug了。 二者相差一个下划线`_`，通过`mv`或`cp`，可以通用。

第二类是`combined-*.ninja`文件。 在使用了[Soong](https://www.codeprj.com/link/aHR0cHM6Ly9hbmRyb2lkLmdvb2dsZXNvdXJjZS5jb20vcGxhdGZvcm0vYnVpbGQvc29vbmcv)后，除了`build-*.ninja`之外，还会产生对应的`combined-*.ninja`，二者的`*`内容相同。 以下以AOSP的aosp_arm64-eng为例，展示`out/combined-aosp_arm64.ninja`文件的内容。

```ninja
builddir = out
include out/build-aosp_arm64.ninja
include out/soong/build.ninja
build out/combined-aosp_arm64.ninja: phony out/soong/build.ninja
```

这类是组合文件，是把`build-*.ninja`和`out/soong/build.ninja`组合起来。 所以，使用[Soong](https://www.codeprj.com/link/aHR0cHM6Ly9hbmRyb2lkLmdvb2dsZXNvdXJjZS5jb20vcGxhdGZvcm0vYnVpbGQvc29vbmcv)后，`combined-*.ninja`是编译执行的真正入口。

第三类是`out/soong/build.ninja`文件，它是从所有的`Android.bp`转换过来的。

`build-*.ninja`是从所有的Makefile，用[Kati](https://www.codeprj.com/link/aHR0cHM6Ly9naXRodWIuY29tL2dvb2dsZS9rYXRp)转换过来的，包括`build/core/*.mk`和所有的`Android.mk`。 所以，在不使用[Soong](https://www.codeprj.com/link/aHR0cHM6Ly9hbmRyb2lkLmdvb2dsZXNvdXJjZS5jb20vcGxhdGZvcm0vYnVpbGQvc29vbmcv)时，它是唯一入口。 在使用了[Soong](https://www.codeprj.com/link/aHR0cHM6Ly9hbmRyb2lkLmdvb2dsZXNvdXJjZS5jb20vcGxhdGZvcm0vYnVpbGQvc29vbmcv)以后，会新增源于`Android.bp`的`out/soong/build.ninja`，所以需要`combined-*.ninja`来组合一下。

可以通过以下命令，单独产生全编译的[Ninja](https://www.codeprj.com/link/aHR0cHM6Ly9uaW5qYS1idWlsZC5vcmcv)文件。

```sh
make nothing
```

## 用ninja编译

在产生全编译的[Ninja](https://www.codeprj.com/link/aHR0cHM6Ly9uaW5qYS1idWlsZC5vcmcv)文件后，可以绕过Makefile，单独使用`ninja`进行编译。

全编译（7.0版本），相当于`make`：

```sh
ninja -f out/build-aosp_arm64.ninja
```

单独编译模块，比如Settings，相当于`make Settings`：

```sh
ninja -f out/build-aosp_arm64.ninja Settings
```

在8.0以上，上面的文件应该替换为`out/combined-aosp_arm64.ninja`，否则可能找不到某些Target。

另外，还有办法不用输入`-f`参数。 如前所述，如同Makefile之于`make`，`ninja`默认的编译文件是`build.ninja`。 所以，使用软链接，可以避免每次输入繁琐的`-f`。

```sh
ln -s out/combined-aosp_arm64.ninja build.ninja
ninja Settings
```

用`ninja`进行单模块编译的好处，除了更快以外，还不用生成单模块的[Ninja](https://www.codeprj.com/link/aHR0cHM6Ly9uaW5qYS1idWlsZC5vcmcv)文件，省了四五分钟。

## 总结

在以[Ninja](https://www.codeprj.com/link/aHR0cHM6Ly9uaW5qYS1idWlsZC5vcmcv)在实际编译中替换Makefile以后，Android在编译时更快了一些。 不过，在首次生成、或重新生成[Ninja](https://www.codeprj.com/link/aHR0cHM6Ly9uaW5qYS1idWlsZC5vcmcv)文件时，往往额外耗时数分钟，反而比原先使用Makefile更慢了。

在增量编译方面，原先由于其Makefile编译系统的实现问题，是不完善的。 也就是说，在`make`编译完一个项目后，如果再执行`make`，会花费较长时间重新编译部分内容。 而使用[Ninja](https://www.codeprj.com/link/aHR0cHM6Ly9uaW5qYS1idWlsZC5vcmcv)以后，增量编译做得比较完善，第二次`make`将在一分钟内结束。

除此之外，由于[Ninja](https://www.codeprj.com/link/aHR0cHM6Ly9uaW5qYS1idWlsZC5vcmcv)的把编译流程集中到了一个文件，并且提供了一些工具命令。 所以编译信息的提取、编译依赖的分析，变得更加方便了。



## CCS5.0的ninja文件

由于我同时编译过8155平台和6155平台，所有build-*.ninja既有8155和又有6155的。

```shell
work@ubuntu-cts:~/ccs5.0/apps/LINUX/android/out$ find . -name "*.ninja"       
./combined-msmnile_au.ninja
./build-msmnile_au-cleanspec.ninja
./build-msmnile_au.ninja
./build-sm6150_au-cleanspec.ninja
./build-sm6150_au.ninja
./soong/.bootstrap/build-globs.ninja
./soong/.bootstrap/build.ninja
./soong/build.ninja
./soong/.minibootstrap/build-globs.ninja
./soong/.minibootstrap/build.ninja
./build-sm6150_au-package.ninja
./combined-sm6150_au.ninja
./build-msmnile_au-package.ninja
```

combined-msmnile_au.ninja

```shell
work@ubuntu-cts:~/ccs5.0/apps/LINUX/android/out$ cat combined-sm6150_au.ninja 

builddir = out
pool highmem_pool
 depth = 1
build _kati_always_build_: phony
subninja out/build-sm6150_au.ninja
subninja out/build-sm6150_au-package.ninja
subninja out/soong/build.ninja
```

