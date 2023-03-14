C 的标准库里，读取一个字符，返回类型都是int而不是char。如：

```c
int getc(FILE *stream);
int getchar(void);   
```

下面解释原因

1. C/C++语言char就是其它语言里的byte，表示一个字节，而不是单单指字符。其实char就是一个数字。

2. 当没有输入(读取到文件末尾)时返回EOF，EOF在头文件stdio.h中被定义-1。用户用`if(c==-1)`判断是否到文件末尾。那么问题来了，-1无非就是0xFFFF...，万一文件内容就是这个呢？所以如果要读一个字节，返回类型一定要一个字节大一点，以容纳异常返回值。返回定义成int的话，如果读取到0xFF，放到int里是255（-1是0xFFFFFFFF, 读取的数据只会是0x00~0xFF,不存在交叉）；而如果返回类型定义成`signed char`，0xFF就是-1，无法区分是读到这个数据，还是表示文件末尾。
   
3. 姑且不考虑上一点，`if(c==-1)`判断是否到文件末尾依然是有问题的。char类型有些编译器中定义为无符号数，范围是0~255，另外一些编译器中定义为有符号数，范围是-128~127。如果用char做返回类型，万一char是0~255，`if(c==-1)`这个过程会发生隐式转换，向上转成int，c是255，-1还是-1，所以真正比较的是`if(255==-1)`,而不是用户期望的，就出错了。用更大范围区间的int型接收，才能使程序更加稳健。

4. 那为什么不用short呢？当然没问题，但是也没有什么好处。

   CPU读数据是根据本机宽度来的，32位计算机，CPU宽度是32位，地址线为32根，寄存器都是32的，那么一次读的就是32位即4个字节。虽然定义char是1字节，但是在那根总线上面传输的一定是4字节；把char类型的变量送进ALU中运算，那必然是需要把char类型变成32位，然后通过32位寄存器送入ALU。

   所以，长度低于`int`的整数类型在大部分运算之前都会被转成`int`/`unsigned int`，拓展到四个字节 ，用`char`的话反而还凭空多出好几次类型转换。 就是C/C++语言里的`Integer Promotion`概念，文章末尾介绍规则。

   而且这种局部变量，一个说得过去的编译器都会把它弄到寄存器里的。比如，

   ```c
   int i;
   for(i=0; i < 255; i++) {...}
   ```

   大家想一下，有的时候i的循环次数可以确认比较小，完全可以用`char i`,但是为什么没人这么写呢？不仅仅是习惯问题，事实，编译器会使用寄存器来放这个i，所以弄成char没什么用。

      >  C的整型算术运算总是至少以缺省整型类型，也就是默认整形类型的精度来进行的。为了获得这个精度，表达式中的字符和短整型操作数在使用之前被转换为普通整型，这种转换称为**整型提升**。
      >
      >  char c1 = 1 ;
      >
      >  提升之后的结果是：
      >
      >  00000000000000000000000000000001
      >
      >  signed char c2 = - 1 ;
      >
      >  提升之后的结果是： 
      >
      >  11111111111111111111111111111111
      >
      >  ```c
      >  	signed char c = -1;
      >   unsigned char cc = -1;
      >   printf("%d %d\n",c, cc);//-1 255
      >  ```
      >
      >  

   另外，还有一个`内存对齐`的概念。(不妨以32位计算机为例) char类型占用一个字节，short类型占用两个字节，int类型占用4个字节，但是在内存中分配存储空间的时候，并不是紧挨着。有的时候，明明只定义了char，但是消耗的空间是一个int。

   ```c
   unsigned char c = 0;
   int i = 0;
   ```

   可以反汇编查看存储空间，发现变量c和变量i之间闲置了三个字节。在我写的demo：CBasic里面的test_alignof.c我详细解释了内存对齐，这里不赘述。之所以要做内存对齐，也是和CPU寻址有关，不对齐对性能影响极大，所以不管哪个编译器，一定会牺牲这一点点内存空间。



# 关于Integer Promotion

**C99 SPEC(6.3.1.1)：**

> If an int can represent all values of the original type, the value is converted to an int; otherwise, it is converted to an unsigned int. These are called the integer promotions. All other types are unchanged by the integer promotions.

在算术运算中，如果原始类型的取值范围都能用int型表示，则其值被提升为int型，如果表示不了就提升为unsigned int型。

有符号、无符号的 char, short都可以用signed int表示，所以运算时转成signed int。

有无符号int和无符号int运算, 有符号数会自动转换为无符号数。

```c
#include <stdio.h>

void compareInt() {
    signed int a = -1;
    unsigned int b = a;
    printf("a=%d,b=%u  ;", a, b);

    if(a == b)
        printf("a==b");
    else if(a<b)
        printf("a<b");
    else
        printf("a>b");
    printf("\n");
}
void compareChar() {
    signed char a = -1;
    unsigned char b = a;
    printf("a=%d,b=%u  ;", a, b);

    if(a == b)
        printf("a==b");
    else if(a<b)
        printf("a<b");
    else
        printf("a>b");
    printf("\n");
}
void compareInt2() {
    signed int a = -1;
    unsigned int b = 1;
    printf("a=%d,b=%u  ;", a, b);

    if(a == b)
        printf("a==b");
    else if(a<b)
        printf("a<b");
    else
        printf("a>b");
    printf("\n");
}
int main() {
    compareInt();//a==b
    compareChar();//a<b
    compareInt2();//a>b
    return 0;
}
输出：
a=-1,b=4294967295  ;a==b
a=-1,b=255  ;a<b
a=-1,b=1  ;a>b
```

在compareInt()里，虽然a=-1,b=4294967295,但是由于内存中的位形式是相同的都是0xffffffff（补码），所以输出a==b。

compareChar()里，a、b在内存中的二进制都是0xFF, 在判断a == b时，编译器首先会对char型变量进行Integer Promotion，signed char a转成signed int 后是0xffffffff（符号位扩展），即-1；unsigned char b转成signed int后是0x000000ff（零扩展），即255。所以a < b。

compareInt2()里，a = -1，b = 1，应该是a<b。但实际上，signed int会先转换成unsigned int，-1变成4294967295，所以a>b。



另外，我注意到，底层到了硬件那一级，是只认二进制，不区分有符号无符号的。比如，-1和4294967295的二进制补码表示相同，那么不管是`-1*3`，还是`4294967295*3`,送到ALU里的，都是ffffffff和3，出来的都是fffffffd。外面语言把结果识别成有符号数（-3）或无符号数（4294967293）。ALU同样的运算过程，即表示计算`-1*3`，又表示计算`4294967295*3`,结果都是对的。这也是计算机采用补码的科学之处。

```c
    int aa = -1;
    int bb = 3;
    printf("%d %u\n", aa*bb, aa*bb);

    unsigned int cc = 4294967295;
    unsigned int dd = 3;
    printf("%d %u\n", cc*bb, cc*bb);
```

输出是：

-3 4294967293
-3 4294967293

（解释一下，`4294967295*3=12884901885`，超出了32位的表示范围，会发生溢出。12884901885=4294967296+4294967296+4294967293，所以对于计算机，溢出后 `4294967295*3`的结果应该是4294967293）。