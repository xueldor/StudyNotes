Ninja的定位非常清晰，就是达到更快的构建速度。为了达到轻巧、速度快的优点，Ninja仅仅专注于核心的功能。

Ninja认为make有下面几点造成编译速度过慢：

- 隐式规则，make包含很多默认
- 变量计算，比如编译参与应该如何计算出来
- 依赖对象计算

ninja认为描述文件应该是这样的:

- 依赖必须显式写明(为了方便可以产生依赖描述文件)
- 没有任何变量计算
- 没有默认规则，没有任何默认值

所以ninja比make简单很多。相比Make，ninja有这些优势：

* 非常快（即瞬间）增量构建，即使是非常大的项目。
* "代码如何构建"的策略非常少。关于代码如何构建不同的项目和高级构建系统有不同的意见；例如，构建目标是应该与代码放在一起还是放在单独的目录里？是否有为项目构建一个可分发的“包”的规则？规避这些策略，而不是选择，否则只会变得复杂。
* 获取正确的依赖关系，在某些特定的情况下Makefiles很难获取正确的依赖关系（例如：输出隐式依赖于命令行生成，构建c代码你需要使用gcc的-M标志去生成其头文件依赖）

Makefile支持分支、循环等流程控制，而Ninja只支持一些固定形式的配置。ninja本质上是配置文件。

`make clean`需要自己实现, `ninja -t clean`是自带的。

ninja和make的比较：https://ninja-build.org/manual.html#_comparison_to_make

而本意上，Makefile是设计来给人手写的，而Ninja设计出来是给其它程序生成的。但是发展到今天，很多时候Makefile也不会手写，比如CMake就可以自动生成makefile。而ninja手写也并不复杂，毕竟ninja本来就比make简单。

二者的相同点是，都是为了控制编译流程而设计。 所以，他们的核心功能，都是指定目标，以及目标之间的依赖关系，自动计算执行顺序。

Ubuntu安装：

```shell
sudo apt install ninja-build
```

## 零. 概念

### 基本概念

- edge(边): 即build语句，指定目标(target)、规则(rule)与输入(input)，是编译过程拓扑图中的一条边（edge）。
- target(目标): 编译过程需要产生的目标，由build语句指定
- output(输出): build语句的前半段，是target的另一种称呼
- input(输入): build语句的后半段，用于产生output的文件或目标，另一种称呼是依赖
- rule(规则): 通过指定command与一些内置变量，决定如何从输入产生输出
- pool：一组rule或edge，通过指定其depth，可以控制并行上限
- scope(作用域)：变量的作用范围，有rule与build语句的块级，也有文件级别。

### 关键字

- build：定义一个edge
- rule：定义一个rule
- pool：定义一个pool
- default：指定默认的一个或多个target
- include：添加一个ninja文件到当前scope
- subninja：添加一个ninja文件，其scope与当前文件不同。
- phony：一个内置的特殊规则，指定非文件的target

### 变量

变量分两种，内置变量与自定义变量。二者都可以通过`var = str`的方式定义，通过`$var`或`${var}`的方式引用。变量类型只有字符串这一种。

内置变量只有两个：

- builddir : 指定一些文件的输出目录
- ninja_required_version : 指定ninja命令的最低版本

## 一. helloworld示例

足够简单，只有两个文件：

```shell
work@ubuntu-cts:~/ninja/01$ tree
.
├── build.ninja
└── Hello.cpp

0 directories, 2 files
```

Hello.cpp代码：

```c++
#include <stdio.h>

int main(int argc,char* argv[]) {
    printf("Hello");
    return 0;
}
```

build.ninja：

```
# nja最小需要版本
ninja_required_version = 1.5

# 变量
cflags = -Wall

# 规则
rule compile_hello
 command=g++ -c $cflags -MD  $in -o $out

build hello.o: compile_hello Hello.cpp
```

这里引出两个关键字：rule和build。

```
# 一个rule就是通过${in}输入的目标列表，生成${out}的输出目标列表
# command 是一个要有的，指定实际执行的命令
rule rule_name
    command = echo ${in} > ${out}
    var = str

#生成目标foo, 生成方式是“rule_name”，输入是str1和str2。foo依赖str1和str2
#执行这个build时，rule_name里的${in}就是“str1 str2”，${out}就是foo
build foo : rule_name str1 str2 
    var = str
```

执行ninja命令：

```shell
work@ubuntu-cts:~/ninja/01$ ninja
[1/1] g++ -c -Wall -MD  Hello.cpp -o hello.o
work@ubuntu-cts:~/ninja/01$ tree
.
├── build.ninja
├── Hello.cpp
├── hello.d
└── hello.o

0 directories, 4 files
```

执行clean：

```shell
work@ubuntu-cts:~/ninja/01$ ninja -t clean
Cleaning... 1 files.
work@ubuntu-cts:~/ninja/01$ ls
build.ninja  Hello.cpp  hello.d
```

为什么hello.d没有被清除？因为"build hello.o"只有hello.o没有hello.d。可以改一下：

```
build hello.o|hello.d: compile_hello Hello.cpp
#（ | 表示后面的hello.d不作为${out}的部分，下章介绍）
```

这样两个文件就都清了：

```shell
work@ubuntu-cts:~/ninja/01$ ninja -t clean
Cleaning... 2 files.

work@ubuntu-cts:~/ninja/01$ ls
build.ninja  Hello.cpp
```

还可以做个试验：

```
# nja最小需要版本
ninja_required_version = 1.5

# 变量
cflags = -Wall

# 规则
rule compile_hello
   command=g++ -c $cflags -MD  $in -o ${out}ut

build hello.o|hello.d: compile_hello Hello.cpp
```

我在command的${out}后面加了个ut，这样输出是hello.out而不是hello.o，然后build的目标名还是hello.o,这样clean也不会清掉，因为ninja系统不可能知道你在shell命令里是怎样生成的。

```shell
work@ubuntu-cts:~/ninja/01$ ninja
[1/1] g++ -c -Wall -MD  Hello.cpp -o hello.out

work@ubuntu-cts:~/ninja/01$ ls
build.ninja  Hello.cpp  hello.d  hello.out

work@ubuntu-cts:~/ninja/01$ ninja -t clean
Cleaning... 1 files.
work@ubuntu-cts:~/ninja/01$ ls
build.ninja  Hello.cpp  hello.out
work@ubuntu-cts:~/ninja/01$ 
```

所以说，${out}也好，${in}也罢，我们就用真实的文件名。

其它注意点：

* rule和command之间插入一行回车，执行报错

  ```shell
  ninja: error: build.ninja:16: expected 'command =' line
  ```

* command前面的空格全部去掉，执行报错，信息同上

* command前面的空格换成tab，执行报错，信息同上

* 第5行自定义变量cflags，前面加一个空格，报错

  ```shell
  ninja: error: build.ninja:5: unexpected indent
  ```

* 关键字rule前面加一个空格，同样报错

* build后面随便添加一个依赖，比如:

  ```ninja
  build hello : link_hello hello.o test
  ```

  目录下没有名叫 test 的文件，也没有定义生成 test 的规则，于是执行报错：`ninja: error: 'test', needed by 'hello', missing and no known rule to make it`。

可见在ninja里，空格不是可以随便加的。顶层的变量，还有rule、build等关键字，必须靠着最左边写。

解释一下空格的作用。一个行用空格来缩进，如果这一行的缩进比前面的更长，就认为是一个子作用域，相当于C语言的“{”；如果缩进比parent更短，就认为关闭了作用域，相当于C语言的“}”。

## 二. build edge

 build代码块形式可以很复杂：

```ninja
build output0 output1 | output2 output3: rule_name $
        input0 input1 $
        | input2 input3 $
        || input4 input5
    var0 = str0
    var1 = str1
```

行末的`$`是表示并未真正换行，就是书写时，一行太长，我们换行写，ninja语法上认为它们是一行。第二行前面的空格缩进会忽略。

`output0`和`output1`是显式（explicit）输出，会出现在`${out}`列表中；`|`后面的`output2`和`output3`是隐式（implicit）输出，不会出现在`${out}`列表中；

 `input0`和`input1`是显示依赖（输入），会出现在`${in}`列表中； 出现在`|`后面的`input2`和`input3`是隐式依赖，不会出现在`${in}`列表中； 出现在`||`后面的`input4`和`input5`是隐式**order-only**依赖，不会出现在`${in}`列表中。

所谓**order-only**依赖，就是指**仅仅是顺序需要的**依赖。怎么理解呢？正常的依赖，如果`input4`或`input5`有缺失，那么会先生成`input4`或`input5`，然后重新执行这条edge更新`output0`和`output1`。但是**order-only**依赖，不会重新生成`output0`和`output1`，因为只是顺序上，需要先执行`input4`或`input5`，并不存在真正的依赖。

 在首次编译时，和其它依赖的表现相同； 再次编译时，如果`input4`或`input5`有缺失，[Ninja](https://ninja-build.org/)会更新它们，但不会执行这条edge，更新`output0`和`output1`。 order-only的依赖，不是真的依赖，而是可有可无、只是需要在当前edge之前执行而已。

## 三. rule 

rule有以下内置变量：

| 内置变量                 | 作用                                                         |
| ------------------------ | ------------------------------------------------------------ |
| command                  | 定义一个规则所**必备**的变量，指定实际执行的命令。           |
| description              | command的说明，会替代command在无`-v`时打印。                 |
| generator                | 指定后，这条rule生成的文件不会被默认清理。                   |
| in                       | 空格分割的input列表。                                        |
| in_newline               | 换行符分割的input列表。                                      |
| out                      | 空格分割的output列表。                                       |
| depfile                  | 指定一个Makefile文件作为额外的显式依赖。                     |
| deps                     | 指定gcc或msvc方式的依赖处理。                                |
| msvc_deps_prefix         | 在`deps=msvc`情况下，指定需要去除的msvc输出前缀。            |
| restat                   | 在command执行结束后，如果output时间戳不变，则当作未执行。    |
| rspfile, rspfile_content | 同时指定，在执行command前，把rspfile_content写入rspfile文件，执行成功后删除。 |

此外可以自定义变量。

## 四. helloworld示例2

完善前面的hello示例，build.ninja:

```ninja
# nja最小需要版本
ninja_required_version = 1.5

# 变量
cflags = -Wall

# 规则
rule compile_hello
   command=g++ -c $cflags -MD -MF $out.d $in -o ${out}
   description = 编译 $in 成为 $out
   depfile = $out.d
   deps = gcc
build hello.o: compile_hello Hello.cpp

rule link_hello
   command = g++ $DEFINES $INCLUDES $cflags $in -o $out
   description = 链接 $in 成为 $out
build hello : link_hello hello.o

# 伪目标all,执行的是hello
build all: phony hello

# 默认编译什么(单独运行ninja)
default all
```

解释一下depfile和deps两个内置变量。

gcc、clang等一些编译器可以以makefile的语法，生成依赖信息。这个例子中，”command=g++ -c $cflags -MD -MF $out.d $in -o ${out}“，其中”-MD -MF $out.d“即把dependency 信息导出到

hello.o.d文件中。为了把这个信息导到ninja里面，ninja可以提供了depfile属性，它的值一定要指向依赖信息输出的文件。

至于deps属性，是因为加载depfile（依赖信息文件）比较慢，对应大型项目，可能性能受到影响。于是，Ninja 1.3可以指定deps,效果是depfile生成后，ninja立马把depfile转换成一种紧凑的格式，保存在ninja内置数据库中。然后把原来的depfile文件删除。

deps的取值有gcc和msvc两种。

执行ninja命令：

```shell
work@ubuntu-cts:~/ninja/02$ ninja -v
[1/2] g++ -c -Wall -MD -MF hello.o.d Hello.cpp -o hello.o
[2/2] g++   -Wall hello.o -o hello
work@ubuntu-cts:~/ninja/02$ 
work@ubuntu-cts:~/ninja/02$ ls
build.ninja  hello  Hello.cpp  hello.o
```

可以看到hello.o.d文件没了。

## 五 pool

pool的意义，在于限制一些非常消耗硬件资源的edge同时执行

```ini
pool example
    depth = 2

rule echo_var
    command = echo ${var} >> ${out}
    pool = example

build a : echo_var
    var = a

build b : echo_var
    var = b

build c : echo_var
    var = c
```

以上代码，通过`pool = example`，在rule或build代码块中指定对应的edge所属的pool为`example`。 由于example的depth = 2，所以a、b、c三个target最多只有2个可以同时生成。
 目前，ninja只有一个内置的pool，名为`console`。 这个pool的depth等于1，只能同时执行1个edge。 它的特点是，**可以直接访问stdin、stdout、stderr三个特殊stream**。

## 六 子文件

引用其它ninja文件，有两种方法：`subninja ninja_file` or `include ninja_file`. 

它们的区别在于作用域（scope）。

include是在当前作用域包含其他.ninja文件。相当于把子文件的内容直接替换到这里。

subninja关键字引入了一个新的作用域，被包含的文件里，虽然可以使用外面文件的变量和rules ，但是只对它的这个文件有效，影响不到外面。

## 七 作用域

示例一

全局定义变量var1为0,在目标test1下面，var1赋值为1。执行时，先执行test1，再执行test2。但是test2里面，var1的值依然为0。

```ninja
# nja最小需要版本
ninja_required_version = 1.5

# 变量
var1 = 0

# 规则
rule test
   command=echo $var1 > $out
build test1: test
 var1=1

build test2 : test

build all: phony test1 test2

# 默认编译什么(单独运行ninja)
default all
```

执行：

```shell
work@ubuntu-cts:~/ninja/03$ ninja -v
[1/2] echo 1 > test1
[2/2] echo 0 > test2
```

示例二

```ninja
# nja最小需要版本
ninja_required_version = 1.5

# 变量
var1 = 0

# 规则
rule test
   command=echo $var2 > $out
build test1: test
 var1=2
 var2=${var1}

build all: phony test1

# 默认编译什么(单独运行ninja)
default all
```

在build test1下面，我先给var1赋值2，燃油和var1赋给var2。但是test里面还是0。所以var2=${var1}这里用的还是顶层定义的var1。不错意外的话，如果把顶层的var1声明去掉，var2应该是空的：

```ninja
# nja最小需要版本
ninja_required_version = 1.5

# 变量
#var1 = 0

# 规则
rule test
   command=echo $var2 > $out
build test1: test
 var1=2
 var2=${var1}



build all: phony test1

# 默认编译什么(单独运行ninja)
default all
```

```shell
work@ubuntu-cts:~/ninja/03$ ninja -v
[1/1] echo  > test1
```



## ninja参数（ninja -help）

ninja命令语法：

```shell
ninja [options] [targets...]
```

如果没有指定target，执行的是default。如果脚本里也没有定义default，ninja会构建所有的“没有在其它地方定义为input”的output。

如果没有用“-f FILE”指定ninja构建文件，默认执行的当前目录下的build.ninja。

```mipsasm
--version  # 打印版本信息
-v         # 显示构建中的所有命令行（这个对实际构建的命令核对非常有用）

-C DIR     # 在执行操作之前，切换到`DIR`目录
-f FILE    # 制定`FILE`为构建输入文件。默认文件为当前目录下的`build.ninja`。如 ./ninja -f demo.ninja

-j N       # 并行执行 N 个作业。默认N=3（需要对应的CPU支持）。如 ./ninja -j 2 all
-k N       # 持续构建直到N个作业失败为止。默认N=1
-l N       # 如果平均负载大于N，不启动新的作业
-n         # 排练（dry run）(不执行命令，假执行，视其成功执行。如 ./ninja -n -t clean)

-d MODE    # 开启调试模式 (用 -d list 罗列所有的模式)
-t TOOL    # 执行一个子工具(用 -t list 罗列所有子命令工具)。如 ./ninja -t query all
-w FLAG    # 控制告警级别
```

ninja -d list相关：

```bash
Copydebugging modes:
  stats        print operation counts/timing info 打印统计信息
  explain      explain what caused a command to execute 解释导致命令执行的原因
  keepdepfile  don't delete depfiles after they're read by ninja 读取depfile后，不删除它
  keeprsp      don't delete @response files on success 读取@response后，不删除它
  nostatcache  don't batch stat() calls per directory and cache them 不对每个目录批量处理stat()调用和缓存它们
multiple modes can be enabled via -d FOO -d BAR 多模式调用可以接着几个-d
```

ninja -w list相关，主要指定几种情况下告警级别是多少：

```mipsasm
Copywarning flags:
  dupbuild={err,warn}  multiple build lines for one target
  phonycycle={err,warn}  phony build statement references itself
  depfilemulti={err,warn}  depfile has multiple output paths on separate lines
```

ninja -t list相关，主要集成了graphviz等一些对开发非常有用的工具。

```mipsasm
Copyninja subtools:
    browse  # 在浏览器中浏览依赖关系图。（默认会在8080端口启动一个基于python的http服务）
     clean  # 清除构建生成的文件
  commands  # 罗列重新构建制定目标所需的所有命令
      deps  # 显示存储在deps日志中的依赖关系
     graph  # 为指定目标生成 graphviz dot 文件。如 ninja -t graph all |dot -Tpng -o graph.png
     query  # 显示一个路径的inputs/outputs
   targets  # 通过DAG中rule或depth罗列target
    compdb  # dump JSON兼容的数据库到标准输出
 recompact  # 重新紧凑化ninja内部数据结构
```

## 常用命令举例

* -n,假执行，用来在真正执行前，先观察执行结果。比如：`ninja -n -t clean`

```shell
work@ubuntu-cts:~/ninja/02$ ninja -n -t clean
Cleaning...
Remove hello.o
Remove hello
2 files.
work@ubuntu-cts:~/ninja/02$ ls
build.ninja  hello  Hello.cpp  hello.o
```

* -v,打印每个任务执行了哪些指令

*  ninja -t targets all，显示目标

  ```shell
  work@ubuntu-cts:~/ninja/02$ ninja -t targets all
  hello.o: compile_hello
  hello: link_hello
  all: phony
  ```

* 依赖图 ninja -t graph

  ```shell
  work@ubuntu-cts:~/ninja/02$ ninja -t graph
  digraph ninja {
  rankdir="LR"
  node [fontsize=10, shape=box, height=0.25]
  edge [fontsize=10]
  "0x564ab853c880" [label="all"]
  "0x564ab853c730" -> "0x564ab853c880" [label=" phony"]
  "0x564ab853c730" [label="hello"]
  "0x564ab853bda0" -> "0x564ab853c730" [label=" link_hello"]
  "0x564ab853bda0" [label="hello.o"]
  "0x564ab853bea0" -> "0x564ab853bda0" [label=" compile_hello"]
  "0x564ab853bea0" [label="Hello.cpp"]
  }
  ```

  这是dot文本，可读性差。安装graphviz，把它转成png或svg。

  ```shell
  ninja -t graph | dot -Tpng -o hello.png
  ```

  打开hello.png

  ![hello](_img/hello.png)

  



## 附言

gcc参数“-MD“ ，M是生成文件关联的信息。包含目标文件所依赖的所有文件；D是把这些信息写到hello.d文件里。这个例子，hello.d的内容是：

```shell
work@ubuntu-cts:~/ninja/01$ cat hello.d
hello.o: Hello.cpp /usr/include/stdc-predef.h /usr/include/stdio.h \
 /usr/include/x86_64-linux-gnu/bits/libc-header-start.h \
 /usr/include/features.h /usr/include/x86_64-linux-gnu/sys/cdefs.h \
 /usr/include/x86_64-linux-gnu/bits/wordsize.h \
 /usr/include/x86_64-linux-gnu/bits/long-double.h \
 /usr/include/x86_64-linux-gnu/gnu/stubs.h \
 /usr/include/x86_64-linux-gnu/gnu/stubs-64.h \
 /usr/lib/gcc/x86_64-linux-gnu/7/include/stddef.h \
 /usr/lib/gcc/x86_64-linux-gnu/7/include/stdarg.h \
 /usr/include/x86_64-linux-gnu/bits/types.h \
 /usr/include/x86_64-linux-gnu/bits/timesize.h \
 /usr/include/x86_64-linux-gnu/bits/typesizes.h \
 /usr/include/x86_64-linux-gnu/bits/time64.h \
 /usr/include/x86_64-linux-gnu/bits/types/__fpos_t.h \
 /usr/include/x86_64-linux-gnu/bits/types/__mbstate_t.h \
 /usr/include/x86_64-linux-gnu/bits/types/__fpos64_t.h \
 /usr/include/x86_64-linux-gnu/bits/types/__FILE.h \
 /usr/include/x86_64-linux-gnu/bits/types/FILE.h \
 /usr/include/x86_64-linux-gnu/bits/types/struct_FILE.h \
 /usr/include/x86_64-linux-gnu/bits/types/cookie_io_functions_t.h \
 /usr/include/x86_64-linux-gnu/bits/stdio_lim.h \
 /usr/include/x86_64-linux-gnu/bits/sys_errlist.h
```

再加一个-MF指定输出目录，像这样：‘-MD -MF $out.d“

## 参考

Ninja文件的基本语法 https://note.qidong.name/2017/08/ninja-syntax/

ninja文件语法学习https://juejin.cn/post/7121638397466132516

官方文档：https://ninja-build.org/manual.html