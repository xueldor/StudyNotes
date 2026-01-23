因为std::string对象代表的字符串，总归是要在内存里有个地方放的，因此显然std::string内部会维护一个字符数组。

问题：

```c++
    char cstr[5] = {'a', 'b', 'c', 'd', '\0'};
    std::string str = cstr;
```

这里std::string内部维护的char数组是直接用的cstr吗？还是std::string会独立开辟空间，把cstr的内容复制过来？

```c++
void testString(){
    char cstr[5] = {'a', 'b', 'c', 'd', '\0'};
    std::string str = cstr;
    std::cout<<&cstr<<" "<<std::endl;
    printf("%p  %p\n",cstr, str.c_str());//
    cstr[1] = 'h';
    std::cout<<cstr<<" "<<str<<std::endl;

    char* p = const_cast<char*>(str.c_str());
    p[2] = 'j';
    std::cout<<str<<std::endl;
}
结果输出：
0x35fb82 
000000000035fb82  000000000035fb70
ahcd abcd
abjd
```

通过这个例子，知道：

1. `std::string str = cstr`,用cstr来初始化str，但str内部不是直接指向cstr，而是自己的内存空间。后续修改cstr不会影响str
2. `str.c_str()`返回的指针是直接指向string内部的字符串的。所以修改c_str()，str也跟着变了。这是违规的写法。
3. 由上一条，可以推测：函数内声明的string对象，不能返回`str.c_str()`，因为string对象作为函数的局部变量，调用结束后就释放了。因此返回的c_str()是乱码。

下面验证第三点：

```c++
const char* testString2(){
    std::string test = "abcdefg";
    printf("testString2 %p %p \n", test.c_str(), test.c_str());
    return test.c_str();
}
//主函数里
const char* p = testString2();
printf("%p \n" , p);
std::cout<<p<<std::endl;//乱码

输出：
testString2 000000000035fb70 000000000035fb70 
000000000035fb70 
��5
```

奇怪的是，把printf("%p \n" , p);去掉后：

```c++
const char* p = testString2();
std::cout<<p<<std::endl;
```

乱码就消失了，正常打印"abcdefg"，说明函数调用完立即打印的话，局部变量还来不及释放。像下面这样：

```c++
    std::cout<<p<<std::endl;//不乱码,abcdefg
    std::cout<<p<<std::endl;//不能输出，表示p指向的内存已释放
```

总之这不影响结论。即：不要返回`str.c_str()`。直接返回std::string就好。



第二个问题：

std::string内部维护的字符数组显然是在堆上动态分配的。因为如果是栈上，必须在编译期间知道数组长度，而std::string没有这个限制。那么问题是，std::string内部是怎么释放堆内存的？什么时候？

答案是：

std::string内部有优秀高效的算法。如果把一个string对象赋给另外一个string对象，那么为了效率，避免堆内存的拷贝，他们可能会直接指向同一个堆内存的。而一旦修改其中一个的值，就会先拷贝内容再修改，使得两个string对象不相关。此所谓“CopyOnWrite”

如果有两个对象引用同一段堆内存，那么std::string会有一个引用计数，保证只有最后一个string对象释放时，才去delete内存。就像智能指针那样。