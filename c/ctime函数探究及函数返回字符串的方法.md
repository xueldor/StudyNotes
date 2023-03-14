一段平平无奇的代码：

```c
time_t tt = time(0);
char* date = ctime(&tt);
printf("date is:%s" , date);
```

怪在哪？我们知道C语言的函数，如果返回是一个数组（包括字符串数组，char*），一般都需要传一个参数用来存放结果：

```c
int[] test(int a, int b, int result[]) {//多定义了一个result参数
    ...
    return result;
}
```

因为局部变量调用完成后是会立即释放的。假如这样写：

```c
int[] test(int a, int b) {
    int result[100];
    ...
    return result;
}
```

那么函数返回后，result的内存已经释放，外面拿到的是随机数。

所以如果不想声明result这个参数，就要用动态分配：

```c
int* test(int a, int b) {
    int* result = (int *)malloc(100 * sizeof(int));
    ...
    return result;
}
int *p = test(1,2);
...
free(p);//用完后需要释放
```

这样就怪异了。别人提供给我的函数库，我怎么知道还需要free一下？我又不关心内部的实现。 

假如我的逻辑比较复杂，有多个分支，是不是还需要写多个free？ 

原则上，谁分配的，就应该谁释放。  

所以，宁愿加一个`int result[]`参数, 没有人会用动态分配方式来做。

那么`ctime`是怎么回事？

其实，除了以上两种外，还有两种方式（有隐患，见后文）：

* 返回函数内定义的静态变量

* 返回全局变量

如果是C++， 还可以是类的成员函数返回类的数据成员数组指针。

C99标准,第7.23.3.2节`ctime`函数声明调用等同于`asctime(localtime(timer))`,而`asctime()`实现(如同一文档中所示)等效于：

```c
char *asctime(const struct tm *timeptr)
{
    static const char wday_name[7][3] = {
        "Sun","Mon","Tue","Wed","Thu","Fri","Sat"
    };
 
    static const char mon_name[12][3] = {
        "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"
    };
 
    static char result[26];
    sprintf(result,"%.3s %.3s%3d %.2d:%.2d:%.2d %d\n",wday_name[timeptr->tm_wday],mon_name[timeptr->tm_mon],timeptr->tm_mday,timeptr->tm_hour,timeptr->tm_min,timeptr->tm_sec,1900 + timeptr->tm_year);
 
    return result;
}
```

看到了`static char result[26]`，果然是用的static变量。

那么可以推断，副作用是，所有的调用都是共享的内存。对这段内存的修改会反应到每个调用者。下面是验证：

```c
    time_t tt = time(0);
    char* date = ctime(&tt);//因为指针也是整形，time_t也是整形，所以要注意传地址，编译器不会提示。
    printf("date is:%s" , date);
    time_t tt2 = tt + 100;
    char* date2 = ctime(&tt2);
    printf("date2 is: %s" , date2);
    printf("date is: %s" , date);//date的打印变成了和date2一样的值，证明了date和date2同是一块内存。
```

某一次的执行输出如下：

```
date is:Mon Feb  6 17:23:29 2023
date2 is: Mon Feb  6 17:25:09 2023
date is: Mon Feb  6 17:25:09 2023
```

可以看到，当第二次调用ctime后，第一次返回的`char* date`的数据也变成了和date2一样的了。

如果不注意的话，很容易写出有问题的代码。