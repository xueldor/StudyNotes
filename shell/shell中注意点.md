Linux 的 Shell 种类众多,Bash 是大多数Linux 系统默认的 Shell。所以本文关注的是 Bash，也就是 Bourne Again Shell。它是 Bourne shell 的扩展。

shell语法有许多细节非常反人类。其原因在于，shell是一个不断增增补补的语言，很多规则都是之前不断“补丁”上去的，导致混乱不堪。尤其是各种括号、空格的组合。

## 基础

* 以#!开头：

```shell
#!/bin/bash
echo "Hello World !"
```

告诉系统这个脚本用“/bin/bash”解释器来执行。ubuntu上不加默认就是bash。如果是“#!/bin/sh”，则使用的“dash”，因为：

```shell
work@S111-CCS2plus:~/xue$ ls -l `which sh`
lrwxrwxrwx 1 root root 4 Jan  2  2020 /bin/sh -> dash
```

但是执行的时候，明确解释器，则会忽略“#!”:

```shell
work@S111:~/xue$ sh ./test.sh
./test.sh: 2: ./test.sh: Syntax error: "(" unexpected
work@S111:~/xue$ bash ./test.sh 
Hello World !
```

以上`sh ./test.sh`使用dash，因为dash不支持数组，故报错。`bash ./test.sh`使用bash解释脚本。

>  Android Shell脚本遵循 [MirBSD Korn Shell](https://www.mirbsd.org/mksh.htm) (mksh) 语法。参阅https://developer.android.google.cn/ndk/guides/wrap-script?hl=zh_cn
>
> mksh应该是兼容Bourne shell的，本文大部分知识都应该同样适用android。

* 声明变量**不需要**像高级语言那样声明变量

  ```shell
  your_name="zhangsan"
  
  #如果你多此一举声明类型,会认为var是一个命令，从而到bin目录下找这个命令
  var your_name="zhangsan"
  Command 'var' not found
  ```

  不同于高级语言，shell对空格敏感。不能加空格`your_name   =   "zhangsan"` <------报错。

  包括后面介绍if语句时，if后面有没有空格，执行情况完全不一样。

  使用变量用美元符：

  ```shell
  your_name="qinjx"
  echo $your_name
  echo ${your_name}
  ```

  使用 unset 命令可以删除变量：

  ```shell
  work@S111-CCS2plus:~/xue$ book=Android
  work@S111-CCS2plus:~/xue$ product=Car
  work@S111-CCS2plus:~/xue$ readonly product 
  work@S111-CCS2plus:~/xue$ unset product
  -bash: unset: product: cannot unset: readonly variable
  work@S111-CCS2plus:~/xue$ unset book
  work@S111-CCS2plus:~/xue$ echo $book
  #没有任何输出,因为book变量已经删除
  work@S111-CCS2plus:~/xue$ 
  
  ```

* 字符串分单引号和双引号

  双引号里面可以有变量和转义字符

  ```shell
  #有变量
  str="I know you are $your_name"
  #有转义字符
  work@S111-CCS2plus:~/xue$ hello="hello,\"Baby\""
  work@S111-CCS2plus:~/xue$ echo $hello
  hello,"Baby"
  ```

  获取字符串长度

  ```shell
  string="abcd"
  echo ${#string} #输出 4
  ```

  提取子字符串

  ```shell
  string="runoob is a great site"
  echo ${string:1:4} # 第一个字符的索引值为 0。输出 unoo
  ```

  

* 数组

  类似于 C 语言，数组元素的下标由 0 开始编号.

```shell
arr=(A B C)
echo "${arr[0]}   ${arr[2]}"

#输出A   C
#这里${arr[0]}大括号不能少，因为arr[0]是作为一个整体。如果少了，$与arr结合,而不是与arr[0]结合，输出A[0] A[2]
```

[*]和[@]：

```shell
arr=(A B C)
echo "${arr[*]}"
echo "${arr[@]}"

$ ./runsh.sh
A B C
A B C
```



数组长度

```shell
str[0]=AA
str[1]=B
str[2]=C
echo ${#str[@]}   #数组长度
echo ${#str[*]}   #数组长度，同上。
echo ${#str}      #str[0]的长度，即AA长度为2
echo ${#str[1]}   #str[1]的长度

输出：
$ /f/Log/runsh.sh
3
3
2
1

```

* 几个特殊字符、

  $0表示执行的文件名（与命令里输的参数一致）。

  从$1开始，依次表示第n个参数。$1表示第一个参数，依次类推.

  ```shell
  echo $0
  echo $1
  
  #执行
  $ ./runsh.sh ab 
  ./runsh.sh
  ab
  
  ```

  另外，

| 参数处理 | 说明                                                         |
| :------- | :----------------------------------------------------------- |
| $#       | 传递到脚本的参数个数。比如`runsh.sh ab  cd`,参数个数为2。    |
| $*       | 以一个单字符串显示所有向脚本传递的参数。 如"$*"用「"」括起来的情况、以"$1 $2 … $n"的形式输出所有参数。 |
| $$       | 脚本运行的当前进程ID号                                       |
| $!       | 后台运行的最后一个进程的ID号                                 |
| $@       | 与$*相同，但是使用时加引号，并在引号中返回每个参数。 如"$@"用「"」括起来的情况、以"$1" "$2" … "$n" 的形式输出所有参数。 |
| $-       | 显示Shell使用的当前选项，与[set命令](https://www.runoob.com/linux/linux-comm-set.html)功能相同。 |
| $?       | 显示最后命令的退出状态。0表示没有错误，其他任何值表明有错误。其实就是main函数的返回值。 |

`$*`和`$@`可能会看的迷惑。很多时候他们是一样的：

```shell
echo $*
echo $@
tmp1=$*
tmp2=$@
echo $tmp1
echo $tmp2

#执行
$ ./runsh.sh ab cd ef
ab cd ef
ab cd ef
ab cd ef
ab cd ef
```

完全看不出区别。但它们是有区别的：

```shell
echo "-- \$* 演示 ---"
for i in "$*"; do
    echo $i
done

echo -e "\n-- \$@ 演示 ---"
for i in "$@"; do
    echo $i
done


#执行
$ ./runsh.sh ab cd ef
-- $* 演示 ---
ab cd ef

-- $@ 演示 ---
ab
cd
ef
```

可见，$*是把“ab cd ef”作为一个整体，而$@则是当做三个片段，循环了三次。

## 运算

简单的数学运算，可以通过expr命令来实现。也可以用双小括号。

```shell
#加法，+号前后空格，因为3、+、4都是expr命令的参数
mek_8q:/sdcard # expr 3 + 4                                                    
7

#减法
mek_8q:/sdcard # expr 5 - 6                                                  
-1

#乘法， *号要通过转义字符
mek_8q:/sdcard # expr 3 \* 4                                                   
12

#除法，因为expr只能针对整数，所以商是0。
mek_8q:/sdcard # expr 3 / 4                                                  
0

#取余
mek_8q:/sdcard # expr 11 % 3                                                 
2

#复合，注意每个元素之间都要空格。括号需要转义
mek_8q:/sdcard # expr 2 + 3 \* 4                                             
14
mek_8q:/sdcard # expr \( 2 + 3 \) \* 4                                       
20
```

要在shell运用它，需要先知道一个知识点，就是 “\`" (反引号)符号。语句里被\`command`包起来的命令是一个单独的命令，执行完的结果拼到外面的语句上。比如前面开头写过这个命令：

```shell
ls -l `which sh`
```

which sh的执行结果是/system/bin/sh，所以这个命令相当于：ls -l /system/bin/sh

所以在shell 里，expr命令可以这么用：

```shell
a=10
b=20

val=`expr $a + $b`
echo "a + b : $val"
```

用双小括号是这样的：

```shell
mek_8q:/ $ a=10
mek_8q:/ $ b=30
mek_8q:/ $ ((c=a+b+1))
mek_8q:/ $ echo $c
41
```

用**$((exp))** 起到类似\`expr $a + $b\`效果：

```shell
work@S111-CCS2plus:~/xue/plus_qm$ echo $((3+4))
7
#$((3+4))相当于`expr 3 + 4`
work@S111-CCS2plus:~/xue/plus_qm$ echo `expr 3 + 4`
7
```

对于$((3+4))，“((”之间以及“))”之间不应有空格，但是里面的3+4，空格则无所谓。

后面统一介绍小括号、中括号、大括号，以及单括号、双括号他们的区别和各种用法。

## 赋值

还用这个例子

```shell
a=10
b=20

val=`expr $a + $b`
echo "a + b : $val"
```

等号前面不能有空格。

## 条件判断

与后面介绍的if语句，结合一起理解。先给出语句。

### 比较

 [ $a == $b ]           ======》 相等

[ $a != $b ]     =====》不相等

[ $a -eq $b ] 	a equal b，检测两个数是否相等
 [ $a -ne $b ] 	a not equal b ,检测两个数是否不相等
-gt	greater than, 检测左边的数是否大于右边的
-lt	 less than, 检测左边的数是否小于右边的
-ge	greater and queal than, 检测左边的数是否大于等于右边的
-le	less and equal than, 检测左边的数是否小于等于右边的

**注意点：**

1. 条件表达式要放在方括号之间
2. "["、“$a”、“ == ”、“$b”、“]”之间各有一个空格，共4个空格，少一个就报错。
3. 关于“=”、”==“、”-eq“，都能表示”相等“，但是不同场景有区别，后文会介绍。

### 布尔运算

| 运算符 | 说明                                                | 举例                                     |
| :----- | :-------------------------------------------------- | :--------------------------------------- |
| !      | 非运算，表达式为 true 则返回 false，否则返回 true。 | [ ! false ] 返回 true。                  |
| -o     | 或运算，有一个表达式为 true 则返回 true。           | [ $a -lt 20 -o $b -gt 100 ] 返回 true。  |
| -a     | 与运算，两个表达式都为 true 才返回 true。           | [ $a -lt 20 -a $b -gt 100 ] 返回 false。 |

### 逻辑运算

假定变量 a 为 10，变量 b 为 20:

| 运算符 | 说明       | 举例                                       |
| :----- | :--------- | :----------------------------------------- |
| &&     | 逻辑的 AND | [[ $a -lt 100 && $b -gt 100 ]] 返回 false  |
| \|\|   | 逻辑的 OR  | [[ $a -lt 100 \|\| $b -gt 100 ]] 返回 true |

### 字符串运算

假定变量 a 为 "abc"，变量 b 为 "efg"：

| 运算符 | 说明                                         | 举例                     |
| :----- | :------------------------------------------- | :----------------------- |
| =      | 检测两个字符串是否相等，相等返回 true。      | [ $a = $b ] 返回 false。 |
| !=     | 检测两个字符串是否不相等，不相等返回 true。  | [ $a != $b ] 返回 true。 |
| -z     | 检测字符串长度是否为0，为0返回 true。        | [ -z $a ] 返回 false。   |
| -n     | 检测字符串长度是否不为 0，不为 0 返回 true。 | [ -n "$a" ] 返回 true。  |
| $      | 检测字符串是否为空，不为空返回 true。        | [ $a ] 返回 true。       |

### 文件测试运算符

用于检测 Unix 文件的各种属性

| 操作符  | 说明                                                         | 举例                      |
| :------ | :----------------------------------------------------------- | :------------------------ |
| -b file | 检测文件是否是块设备文件，如果是，则返回 true。              | [ -b $file ] 返回 false。 |
| -c file | 检测文件是否是字符设备文件，如果是，则返回 true。            | [ -c $file ] 返回 false。 |
| -d file | 检测文件是否是目录，如果是，则返回 true。                    | [ -d $file ] 返回 false。 |
| -f file | 检测文件是否是普通文件（既不是目录，也不是设备文件），如果是，则返回 true。 | [ -f $file ] 返回 true。  |
| -g file | 检测文件是否设置了 SGID 位，如果是，则返回 true。            | [ -g $file ] 返回 false。 |
| -k file | 检测文件是否设置了粘着位(Sticky Bit)，如果是，则返回 true。  | [ -k $file ] 返回 false。 |
| -p file | 检测文件是否是有名管道，如果是，则返回 true。                | [ -p $file ] 返回 false。 |
| -u file | 检测文件是否设置了 SUID 位，如果是，则返回 true。            | [ -u $file ] 返回 false。 |
| -r file | 检测文件是否可读，如果是，则返回 true。                      | [ -r $file ] 返回 true。  |
| -w file | 检测文件是否可写，如果是，则返回 true。                      | [ -w $file ] 返回 true。  |
| -x file | 检测文件是否可执行，如果是，则返回 true。                    | [ -x $file ] 返回 true。  |
| -s file | 检测文件是否为空（文件大小是否大于0），不为空返回 true。     | [ -s $file ] 返回 true。  |
| -e file | 检测文件（包括目录）是否存在，如果是，则返回 true。          | [ -e $file ] 返回 true。  |
| -S file | 判断某文件是否 socket                                        |                           |
| -L file | 检测文件是否存在并且是一个符号链接。                         |                           |

## 一行多条命令

* 一行如果有多个命令，直接用分号隔开，每条命令都会执行。

  ```shell
  mek_8q:/proc # echo 1 ; echo 2                                               
  1
  2
  ```

* 命令之间用 && 分隔，一旦之间有错误的命令，后面的就不执行。

  ```shell
  mek_8q:/proc # ls aaa && ls bbb                                                
  ls: aaa: No such file or directory
  1|mek_8q:/proc # 
  1|mek_8q:/proc # ls kmsg && ls bbb                                             
  kmsg
  ls: bbb: No such file or directory
  ```

  第一个语句，因为aaa文件不存在，命令报错，$?的值是1，故后面的ls bbb不执行。

  第二条语句，因为kmsg是存在的，故后面的ls bbb执行。

* 命令被双竖线 || 所分隔，是"或"的关系，只要有一个已经执行成功，后面的就不要执行了。

## 流程

### if语句

```shell
if condition
then
    command1 
    command2
    ...
    commandN 
fi
```

这里condition就是前面的**条件判断**章节介绍的。比如：

```shell
if [ 1 == 2 ]
then
   echo 1
else
   echo 2
fi

#执行
$ ./runsh.sh
2
```

显然[ 1 == 2 ]的结果是false，所以走到else里。

我们建议这里的condition，用(())来处理整型数字，用[[]]来处理字符串或者文件。

```shell
mek_8q:/ $ if [[ 1 + 1 < 3 ]] ; then echo 1; else echo 2; fi                   
/system/bin/sh: syntax error: unexpected operator/operand '+'
1|mek_8q:/ $ 
1|mek_8q:/ $ if (( 1 + 1 < 3 )) ; then echo 1; else echo 2; fi                              <
1
#从上面示例看到，[]和[[]]对数字不友好，涉及整数计算的，还是用(())比较好。
```

关于（）、(())、[]、[[]]这些可以参考后文。但是这里要说一下，[]和[[]]才是做条件判断的，(())虽然这里能用，但本质上是数字运算，只是数字也对应布尔值，所以这里才能用。我看通过例子看下区别：

```shelll
mek_8q:/ $ [ 1 ] && echo true
true
mek_8q:/ $ [ 0 ] && echo true                                                  
true
mek_8q:/ $ 
mek_8q:/ $ ((1)) && echo true
true
mek_8q:/ $ ((0)) && echo true                                                  
1|mek_8q:/ $

mek_8q:/ $ ((ccc=1<2)) ;echo $ccc                                            
1
mek_8q:/ $ 
mek_8q:/ $ ((ccc=1>2)) ;echo $ccc                                              
0

```

可见，((xxx))的运算结果是0，代表false；其它数字代表true。而[ 0 ]、[ 1 ]都是true。

**注意点**：

* 还是要注意**空格**，if后面要有空格，[ 1 == 2 ]之间也有4个空格。

  假如少了一个空格，报错事小，更严重的是可能会让程序走到错误的分支里，而这很难发现。如：

  ```shell
  if [ 1 == 1 ]
  then
     echo 1
  else
     echo 2
  fi
  #这里ok
  $ ./runsh.sh
  1
  
  #少打一个空格
  if [1 == 1 ]
  then
     echo 1
  else
     echo 2
  fi
  #先报错，然后执行到else里了
  $ ./runsh.sh
  ./runsh.sh: line 2: [1: command not found
  2
  
  
  #例子2， 1==2少打了空格
  if [ 1==2 ] ; then echo 1; fi                                       
  1
  #1==2不成立，不应该执行if里的，但这里执行了。
  ```

* 末尾fi和开头if对应，表示if语句块的结束，不可少。

* else不一定要有。

* if、then、else、fi虽然一起出现，但是视为独立的语句，也就是说，如果出现在同一行，之间要用分号隔离：

  ```shell
  mek_8q:/proc # if [ 1 == 1 ]; then echo 1 ;else echo 2;fi
  1
  #注意之间的分号
  ```

  假如少了分号，那么敲下回车后，控制台继续等你输入：

  ```shell
  mek_8q:/proc # if [ 1 == 1 ]; then echo 1 ;else echo 2  fi                 
  > 
  > 
  > 
  > 
  > 
  > 
  > 
  #敲了这么多回车。最后用Ctrl+C结束
  ```

### for 循环

格式为：

```shell
for var in item1 item2 ... itemN
do
    command1
    command2
    ...
    commandN
done
```

例子：

```shell
for i in `ls`
do echo $i
done

#执行
$ ./runsh.sh
Audio
Customer
Training
2014
Oct
HZH.pdf
会打印当前目录所有文件名
```

```shell
for i in "a" "b" "c"
do echo $i
done

$ ./runsh.sh
a
b
c
```

同理，把for、do、done放到一行，需要用分号隔开。

### while 语句

例子：

```shell
mek_8q:/ # int=1
mek_8q:/ # while(( $int<=5 ))
> do
>     echo $int
>     let "int++"
> done
1
2
3
4
5
mek_8q:/ #
```

这里对空格倒是不敏感。while后面敲个空格也没事，括号里空格删掉也没事。

通过执行死循环模拟CPU高负载：

```shell
while :;do :;done&

或者
while true;do :;done&
```

可以看到，while 、 do、 done在一行，需要分号。

末尾加&表示进程挂在后台。

### until 循环、case ... esac

因为if、for、while已经够用了，故这两不说了。

### break、continue

跳出循环，参考java和C++里的同名词。

## 函数

```shell
demoFun(){
    echo "第一个 shell 函数!"
}
function demoFun2(){
    echo "第二个 shell 函数!"
}
echo "-----函数开始执行-----"
demoFun
demoFun2
echo "-----函数执行完毕-----"

#执行
$ ./runsh.sh
-----函数开始执行-----
第一个 shell 函数!
第二个 shell 函数!
-----函数执行完毕-----

```

shell里面写函数时，缩进排版尽量用空格，不用tab符。虽说用tab一般也问题，但防止tab触发shell里的命令补全。

### 函数参数

Shell 中的函数在定义时不能指明参数，所以`function xxx()`括号里总是空的，但是函数里面却可以获得参数。

和命令行参数一样，函数体内用$1表示第一个参数，$2表示第二个参数...

用 $@ 来遍历函数参数，正如用$@遍历数组一样：

```shell
function testFun(){
	for par in $@
    do
       echo $par
    done
}

work@S111-CCS2plus:~/xue/plus_qm$ testFun 1 2 3 4
1
2
3
4
work@S111-CCS2plus:~/xue/plus_qm$ testFun ab cd efg
ab
cd
efg

```

### 调用传参和返回结果

调用函数的，直接把参数排在后面。

```shell
function testFun(){
	echo 第一个参数：$1
	echo 第二个参数：$2
}

work@S111-CCS2plus:~/xue/plus_qm$ testFun 1 2
第一个参数：1
第二个参数：2

```

要想让函数返回一个结果，有两种方式。

1. return语句

   只能return一个数字，并且是一字节的数字。然后用$?获取返回值。

   ```shell
   function testFun(){
   	return 1
   }
   testFun
   echo 结果是$?
   
   #执行
   $ ./test.bash
   结果是1
   ```

   可以试下把return 1改成返回一个大于255的数，你会发现返回结果是`val%256`,总是在0~255之间。

2. echo 语句

   原理是，通过输出到标准输出返回。因为子进程会继承父进程的标准输出，因此，子进程的输出也就直接反应到父进程。

   **通过$( )获取返回值。**

   ```shell
   function testFun()  
   {  
    echo "arg1 = $1"  
       if [ $1 = "1" ] ;then  
           echo "return 123"  
       else  
           echo "0"  
       fi  
   }
   testFun 1
   retVal=$(testFun 1)
   echo 函数返回: $retVal
   
   #run
   $ ./runsh.sh
   arg1 = 1
   return 123
   函数返回: arg1 = 1 return 123
   ```
   
   

## 文件包含

```shell
. filename   # 注意点号(.)和文件名中间有一空格

或

source filename
```

封装一些公用的代码作为一个独立的文件。

大家都编译过安卓源码：

```shell
work@S111-CCS2plus:~/xue/plus_qm$ source build/envsetup.sh 

或者
work@S111-CCS2plus:~/xue/plus_qm$ . build/envsetup.sh 

```

这样后面就可以使用envsetup.sh里定义的变量和函数。比如编译命令“mm”，实际上是

envsetup.sh里面的一个函数。

## 括号的作用

* 单小括号

  1. 命令组  	括号中的命令将会新开一个子shell顺序执行，多个命令之间用分号隔开，最后一个命令可以没有分号，各命令和括号之间不必有空格

     ```shell
     work@S111-CCS2plus:~/xue/plus_qm$ var=out;(var=inner;echo $var);echo $var
     inner
     out
     
     work@S111-CCS2plus:~/xue/plus_qm$ var=out;{ var=inner;echo $var; };echo $var
     inner
     inner
     ```

     我们发现，小括号包起来的命令，里面变量不影响外面。大括号则不然。

  2. $(cmd)等同于\`cmd\`。
  
  3. 用于初始化数组。如：array=(a b c d)

* 双小括号

  1. 单纯用 ((exp))，整数型的计算，不支持浮点型。

     ```shell
     mek_8q:/ $ ((0))
     1|mek_8q:/ $     <-----------表达式的结果为0，那么返回的退出状态码为1
     1|mek_8q:/ $ ((3+4))   <--------内部已经计算了3+4，但是没有把它赋给谁，也没有打印出来，所以你什么也看不到
     mek_8q:/ $ 
     mek_8q:/ $ var=3
     mek_8q:/ $ ((var=var+3)) <-----(())里的变量可以不加$。
     mek_8q:/ $ echo $var
     6
     mek_8q:/ $ ((var=$var+3)) <--------same as ((var=var+3))
     mek_8q:/ $ echo $var
     9
     mek_8q:/ $ ((var++))
     mek_8q:/ $ echo $var
     10
     ```

  2. $((exp)),不但计算里面的表达式，还把结果替换命令行，比如$((2+3))相当于\`expr 2 + 3\`。

     ```shell
     mek_8q:/ $ $((3+4))                                                        
     /system/bin/sh: 7: not found
     127|mek_8q:/ $ 
     127|mek_8q:/ $ echo $((3+4))  <-----相当于 echo 7
     7
     
     ```

     

* 单中括号

  1. 条件判断，相当于test命令：

  ```shell
  mek_8q:/ $ if [ 1 == 1 ] ; then echo 1; else echo 2; fi                        
  1
  mek_8q:/ $ 
  mek_8q:/ $ if test 1 == 1  ; then echo 1; else echo 2; fi                      
  1
  
  ```

  2. 用作正则表达式的一部分
  3. 数组中用来访问数组的元素

* 双中括号

  功能上，单中括号相当于test命令；而双中括号是 **test 的升级版**，对细节进行了优化，并且扩展了一些功能。

  [[ ]]是Shell 内置关键字，不是命令，在使用时没有给函数传递参数的过程，所以 test 命令的某些注意事项在 [[ ]] 中就不存在了。

  ```shell
  mek_8q:/ $ if [[ a < b ]] ; then echo 1; else echo 2; fi                       
  1
  mek_8q:/ $ if [[ a > b ]] ; then echo 1; else echo 2; fi                       
  2
  mek_8q:/ $ if [ a > b ] ; then echo 1; else echo 2; fi                         
  /system/bin/sh: can't create b: Read-only file system
  2
  mek_8q:/ $ if [ a \> b ] ; then echo 1; else echo 2; fi                        
  2
  ```

  看出端倪没，[]里的`>符号`，需要转义，或者用`-gt`，而[[]]不需要转义，这是其一。

  [[]]可以正常使用&&、||逻辑符号，[[ condition && condition ]]如果用单括号，得写成[ condition ] && [ condition ]。

* 大括号

  1. 以逗号分割的列表进行拓展

     ```shell
     mek_8q:/ $ aa={a,b}.txt
     mek_8q:/ $ echo $aa
     a.txt b.txt
     ```

     这里把{a,b}.txt，展开成了a.txt b.txt。

  2. 以点点分割的顺序文件列表起拓展作用

     ```shell
     work@S111-CCS2plus:~/xue/tmp$ ls -l
     total 0
     work@S111-CCS2plus:~/xue/tmp$ touch {a..d}.txt
     work@S111-CCS2plus:~/xue/tmp$ ls -l
     total 0
     -rw-rw-r-- 1 work work 0 Mar  4 15:54 a.txt
     -rw-rw-r-- 1 work work 0 Mar  4 15:54 b.txt
     -rw-rw-r-- 1 work work 0 Mar  4 15:54 c.txt
     -rw-rw-r-- 1 work work 0 Mar  4 15:54 d.txt
     work@S111-CCS2plus:~/xue/tmp$ 
     ```

  3. 代码块，又被称为内部组，这个结构事实上创建了一个匿名函数 。和小括号包起来的命令组比较，有以下区别：

     1） 小括号包起来的命令，新开一个子shell运行，里面变量不影响外面。大括号则不然。

     2）大括号的左括号与第一个命令，之间要有空格；

     3） 大括号里的命令，最后一个命令末尾要加分号。小括号不需要。

## 关于”=“、”==“、”-eq“

在shell脚本中，单等号和双等号属于算数运算符；“-eq”属于关系运算符。

(1) =和==能用于字符string类型和整型integer的相等判断。“-eq”仅能用于整型integer的相等比较。

(2) “-eq”不能在算数运算表达式“(( ))”中

(3) 在条件表达式“[ ]”中，单等号和双等号等价，都是相等算数运算符。

(4) 在算数运算表达式“(( ))”中，单等号表示**赋值**，双等号表示相等。类似C语言。

## 关于 env、export、set命令

env：显示所有的环境变量

```shell
work@S111-CCS2plus:~$ env
SSH_CONNECTION=192.168.22.24 10316 192.168.20.111 22
LESSCLOSE=/usr/bin/lesspipe %s %s
LANG=en_US.UTF-8
MAVEN_HOME=/home/work/xue/apache-maven-3.8.3
JAVA_HOME=/home/work/xue/jdk-11.0.10
S_COLORS=auto
XDG_SESSION_ID=2049
USER=work
PWD=/home/work
HOME=/home/work
SSH_CLIENT=192.168.22.24 10316 22
ARMGCC_DIR=/home/work/opt/gcc-arm-none-eabi-7-2018-q2-update
XDG_DATA_DIRS=/usr/local/share:/usr/share:/var/lib/snapd/desktop
SSH_TTY=/dev/pts/2
MAIL=/var/mail/work
TERM=xterm
SHELL=/bin/bash
SHLVL=1
LOGNAME=work
XDG_RUNTIME_DIR=/run/user/1000
PATH=/home/work/.local/bin:/home/work/xue/jdk-11.0.10/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games:/usr/lib/sonar-scanner-4.7.0.2747-linux/bin:/snap/bin:/home/work/xue/apache-maven-3.8.3/bin
...省略部分结果
```

export: 不带参数, 显示哪些变量被导出成了用户变量

```shell
work@S111-CCS2plus:~$ export
declare -x ARMGCC_DIR="/home/work/opt/gcc-arm-none-eabi-7-2018-q2-update"
declare -x HOME="/home/work"
declare -x JAVA_HOME="/home/work/xue/jdk-11.0.10"
declare -x LANG="en_US.UTF-8"
declare -x LESSCLOSE="/usr/bin/lesspipe %s %s"
declare -x LESSOPEN="| /usr/bin/lesspipe %s"
declare -x LOGNAME="work"
declare -x MAIL="/var/mail/work"
declare -x MAVEN_HOME="/home/work/xue/apache-maven-3.8.3"
declare -x OLDPWD
declare -x PATH="/home/work/.local/bin:/home/work/xue/jdk-11.0.10/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games:/usr/lib/sonar-scanner-4.7.0.2747-linux/bin:/snap/bin:/home/work/xue/apache-maven-3.8.3/bin"
declare -x PWD="/home/work"
declare -x SHELL="/bin/bash"
declare -x SHLVL="1"
declare -x SSH_CLIENT="192.168.22.24 10316 22"
declare -x SSH_CONNECTION="192.168.22.24 10316 192.168.20.111 22"
declare -x SSH_TTY="/dev/pts/2"
declare -x S_COLORS="auto"
declare -x TERM="xterm"
declare -x USER="work"
declare -x XDG_DATA_DIRS="/usr/local/share:/usr/share:/var/lib/snapd/desktop"
declare -x XDG_RUNTIME_DIR="/run/user/1000"
declare -x XDG_SESSION_ID="2049"
```

加参数，添加环境变量，仅对当前shell有效：

```shell
export CLASSPATH=./JAVA_HOME/lib;$JAVA_HOME/jre/lib
export PATH=/usr/local/mongodb/bin:$PATH 
```

**set**

1. 设置当前所使用 shell 的执行方式。比如，set -e表示指令返回值不等于0，则立即退出shell

   ```shell
   set -e
   ls aaaaa
   echo haha
   
   #执行
   $ ./runsh.sh
   ls: cannot access 'aaaaa': No such file or directory
   
   xuexiangyu@xuexiangyu-PC MINGW64 /f/Log
   #《------我们看到haha没有打印。因为ls aaaaa返回异常，直接退出shell
   
   #然后去掉set -e再执行
   $ ./runsh.sh
   ls: cannot access 'aaaaaaaaaaa': No such file or directory
   haha
   #《=----可见正常情况下，一行命令返回异常是不会影响后面命令执行的。
   ```

   | 参数      | 说明                                                         |
   | :-------- | :----------------------------------------------------------- |
   | -a        | 标示已修改的变量，以供输出至环境变量                         |
   | -b        | 使被中止的后台程序立刻回报执行状态                           |
   | -d        | Shell预设会用杂凑表记忆使用过的指令，以加速指令的执行。使用-d参数可取消 |
   | -e        | 若指令传回值不等于0，则立即退出shell                         |
   | -f        | 取消使用通配符                                               |
   | -h        | 自动记录函数的所在位置                                       |
   | -k        | 指令所给的参数都会被视为此指令的环境变量                     |
   | -l        | 记录for循环的变量名称                                        |
   | -m        | 使用监视模式                                                 |
   | -n        | 测试模式，只读取指令，而不实际执行                           |
   | -p        | 启动优先顺序模式                                             |
   | -P        | 启动-P参数后，执行指令时，会以实际的文件或目录来取代符号连接 |
   | -t        | 执行完随后的指令，即退出shell                                |
   | -u        | 当执行时使用到未定义过的变量，则显示错误信息                 |
   | -v        | 显示shell所读取的输入值                                      |
   | -H shell  | 可利用”!”加<指令编号>的方式来执行 history 中记录的指令       |
   | -x        | 执行指令后，会先显示该指令及所下的参数                       |
   | +<参数>   | 取消某个set曾启动的参数。与-<参数>相反                       |
   | -o option | 特殊属性有很多，大部分与上面的可选参数功能相同，这里就不列了 |

2. **初始化位置参数**

   ```shell
   #依次将set后面的列表设置给参数，即$n
   work@S111-CCS2plus:~$ set a bb 3
   work@S111-CCS2plus:~$ echo $1
   a
   work@S111-CCS2plus:~$ echo $2
   bb
   work@S111-CCS2plus:~$ echo $3
   3
   
   ```

3. 显示一列已设置的 shell 变量，包括用户定义的变量、关键字变量、函数等

   ```shell
   work@S111-CCS2plus:~/xue/plus_qm$ set
   ANDROID_BUILD_PATHS=/home/work/xue/plus_qm/out/soong/host/linux-x86/bin:/home/work/xue/plus_qm/out/host/linux-x86/bin:/home/work/xue/plus_qm/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin:/home/work/xue/plus_qm/prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.9/bin:/home/work/xue/plus_qm/development/scripts:/home/work/xue/plus_qm/prebuilts/devtools/tools:/home/work/xue/plus_qm/external/selinux/prebuilts/bin:/home/work/xue/plus_qm/prebuilts/misc/linux-x86/dtc:/home/work/xue/plus_qm/prebuilts/misc/linux-x86/libufdt:/home/work/xue/plus_qm/prebuilts/android-emulator/linux-x86_64:
   ANDROID_BUILD_TOP=/home/work/xue/plus_qm
   。。。省略大量内容
   __get_cword_at_cursor_by_ref () 
   { 
       local cword words=();
       __reassemble_comp_words_by_ref "$1" words cword;
       local i cur index=$COMP_POINT lead=${COMP_LINE:0:$COMP_POINT};
       if [[ $index -gt 0 && ( -n $lead && -n ${lead//[[:space:]]} ) ]]; then
           cur=$COMP_LINE;
           for ((i = 0; i <= cword; ++i ))
           do
               while [[ ${#cur} -ge ${#words[i]} && "${cur:0:${#words[i]}}" != "${words[i]}" ]]; do
                   cur="${cur:1}";
                   [[ $index -gt 0 ]] && ((index--));
               done;
               if [[ $i -lt $cword ]]; then
                   local old_size=${#cur};
                   cur="${cur#"${words[i]}"}";
                   local new_size=${#cur};
                   index=$(( index - old_size + new_size ));
               fi;
           done;
           [[ -n $cur && ! -n ${cur//[[:space:]]} ]] && cur=;
           [[ $index -lt 0 ]] && index=0;
       fi;
       local "$2" "$3" "$4" && _upvars -a${#words[@]} $2 "${words[@]}" -v $3 "$cword" -v $4 "${cur:0:$index}"
   }
   __git_eread () 
   { 
       test -r "$1" && IFS='
   ' read "$2" < "$1"
   }
   。。。省略大量内容
   ```

   4. unset清除变量

## 反斜杠

一种用于转义字符，应该已经很熟悉。

一种是行的末尾用来续行，即一个长命令分几行输入。

一种是在命令前面加一个反斜杠，这是为了取消别名的调用，使用原命令。比如：

```shell
#原来的ls
work@S111-CCS2plus:~/xue$ ls
apache-maven-3.8.3  apache-maven-3.8.3-bin.tar.gz  jdk-11.0.10	mvnEnv.sh
#设置别名
work@S111-CCS2plus:~/xue$ alias ls='ls -l'
work@S111-CCS2plus:~/xue$ ls
total 585204
drwxrwxr-x  6 work work      4096 Nov  5 09:45 apache-maven-3.8.3
-rw-rw-r--  1 work work   9042049 Nov  5 09:45 apache-maven-3.8.3-bin.tar.gz
drwxr-xr-x  9 work work      4096 Jan 20  2021 jdk-11.0.10
-rw-rw-r--  1 work work        74 Nov  5 10:43 mvnEnv.sh
# 使用\ls调用原来的ls
work@S111-CCS2plus:~/xue$ \ls
apache-maven-3.8.3  apache-maven-3.8.3-bin.tar.gz  jdk-11.0.10	mvnEnv.sh
```

再比如：

```shell
work@S111-CCS2plus:~/xue$ alias aaa='ls'
work@S111-CCS2plus:~/xue$ aaa
apache-maven-3.8.3  apache-maven-3.8.3-bin.tar.gz  jdk-11.0.10	mvnEnv.sh 

work@S111-CCS2plus:~/xue$ \aaa
Command 'aaa' not found
```

许多shell脚本里都会加反斜杠，是为了保证你使用的命令和作者使用的一致，而不是其他的命令的别名。



## 参数替换符$和命令替换符`

反引号是命令替换，美元符是参数替换，前文已述。

