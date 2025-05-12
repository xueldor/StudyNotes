android.bp的文档官网不太好找。一是源码的build目录有一些MD文件可以辅助理解编译系统；二是可以直接阅读build目录的Blueprint和Soong源码；三是源码编译通过后，out/soong/docs/下有一组html文件，相当于android.bp的api，有所有属性和配置项的解释，编写android.bp时可以直接参考。



下面简介一下android的编译系统。

在Android7.0以前，Android都是使用make来组织各模块的编译，对应的编译配置文件就是Android.mk。由于Android系统越来越庞大复杂，Make 构建系统变得缓慢、容易出错、无法扩展、难以测试。

Android7.0引入Soong构建系统，旨在取代 Make。可以认为Kati、Ninja、bluesprint都是它的一部分。

它们之间的关系，ninja是最终执行编译的，相当于Makefile。但是Android.mk是手写的，而Ninja文件被设计成应由其它工具生成。虽然Ninja的构建文件是可读的，但是手写不是特别方便，所以由Blueprint+Soong来生成ninja文件。对应的配置文件就是Android.bp。bp可以视为blueprint的缩写。

blueprint解析Android.bp语法，soong解读Android.bp语义，最终生成Ninja的构建文件。

其它工具如下：

- kati可以将Android.mk文件转换成ninja文件。

- androidmk是辅助开发者的工具，可以将Android.mk文件转换成Android.bp文件，但Android.bp本来就比mk弱，所以不是所有mk都能转成bp。

- bpfmt    用于格式化Android.bp文件


