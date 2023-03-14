

对于多数初学者来说，C++ 的难度并不在语法语意等语言层面，而是没学过 C++ 所支持的面向对象、泛型编程、元编程、函数式编程等不同编程范式，以至于标准模板库（STL）里关于数据结构和算法的知识，甚至一些部分与计算机架构、编译原理、操作系统等知识相关。学习 C++ 可以同时学习、实践这些相关知识。

有人把C++和物理作类比。我很同意。理论物理是一场无尽的旅程，总有最前沿的东西。我对神经科学很感兴趣，也有幸与一个神经科学相关专业的学生交流过，她还给我发过资料，我很感激。然而我在知乎上看到过一个相关的讨论。一个人说“我小时候就想知道大脑是如何工作的，于是我学了神经科学，如今我已经是神经科学博士，依然不知道大脑是如何工作的”。所以我的求知欲只能暂且到此为止。C++亦是如此。

扯远了，我们来说C++有多难吧。

我们只谈构造函数。假如我们有一个类Teacher。

```c++
class Teacher
{
private:
    std::string name;
    std::string position;
};
```


我们考虑给Teacher类加上构造函数。

```c++
class Teacher
{
private:
    std::string name;
    std::string position;

public:
    Teacher(const std::string& n, const std::string& p)
        : name(n), position(p) {}
};
```


虽然语义正确，但是如果我们的实参只为了传递给Teacher，传递之后而没有其他作用的话，那么这个实现是效率低下的。字符串的拷贝花销可观（关于`std::string`的COW，SSO，view的讨论是另一个故事了）。我们在C++11里面有**右值引用**和**move语义**，所以呢，我们可以改成这样。

```c++
class Teacher
{
private:
    std::string name;
    std::string position;

public:
    Teacher(const std::string& n, const std::string& p)
        : name(n), position(p) {}

    Teacher(std::string&& n, std::string&& p)
        : name(std::move(n)), position(std::move(p)) {};
};
```


你可能觉得这样也已经不错了。不过我们还有可能第一个参数右值，第二个参数左值。或者第一个参数左值，第二个参数右值。所以实际上我们需要四个函数的重载。

```c++
class Teacher
{
private:
    std::string name;
    std::string position;

public:
    Teacher(const std::string& n, const std::string& p)
        : name(n), position(p) {}

    Teacher(std::string&& n, std::string&& p)
        : name(std::move(n)), position(std::move(p)) {};

    Teacher(const std::string&& n, const std::string& p)
        : name(std::move(n)), position(p) {}

    Teacher(const std::string& n, const std::string&& p)
        : name(n), position(std::move(p)) {}
};
```


代码有点多。我们有没有什么方法写一个通用的函数来实现这四个函数呢？有。我们在C++11中有**完美转发**。

```c++
class Teacher
{
private:
    std::string name;
    std::string position;

public:
    template <typename S1, typename S2>
    Teacher(S1&& n, S2&& p)
        : name(std::forward<S1>(n)), position(std::forward<S2>(p)) {};
};
```


完成了。美滋滋。然而事情没有这么简单。如果我们的position有默认值，然后我们写如下代码的话。

```c++
class Teacher
{
private:
    std::string name;
    std::string position;

public:
    template <typename S1, typename S2 = std::string>
    Teacher(S1&& n, S2&& p = "lecturer")
        : name(std::forward<S1>(n)), position(std::forward<S2>(p)) {};
};


int main()
{
    Teacher t1 = { "david", "assistant" };
    Teacher t2{ t1 };
}
```

我们出现了编译期错误。因为`Teacher t2{ t1 };`的**重载决议**的最佳匹配是我们的模板，而不是默认的拷贝构造函数，因为拷贝构造函数要求t1是const的。所以，我们可能需要**SFINAE**和**type traits**来修改我们的代码。注意，默认函数参数不能**类型推导**，所以我们才需要的S2的默认模板参数。

```c++
class Teacher
{
private:
    std::string name;
    std::string position;

public:
    template <typename S1, typename S2 = std::string,
    typename = std::enable_if_t<!std::is_same_v<S1, Teacher>>>
    Teacher(S1&& n, S2&& p = "lecturer")
        : name(std::forward<S1>(n)), position(std::forward<S2>(p)) {};
};
```


仍然不对哦，因为我们的完美转发有**引用折叠**机制，我们应该判断的S1是Teacher&而不是Teacher。其次，如果有类继承我们的Teacher的话，拷贝的时候依然会出现这个问题，所以我们需要的不是`is_same`而是`is_convertible`。然而，如果我们直接写`std::is_convertible_v<S1, Teacher>`的话，我们实际上判定是不是可以转换的时候，还是会去看我们的构造函数。也就是说我们自己依赖了自己，无穷递归。所以我们需要的是`std::is_convertible_v<S1, std::string>`。所以，我们修改我们的代码。

```c++
class Teacher
{
private:
    std::string name;
    std::string position;

public:
    template <typename S1, typename S2 = std::string,
    typename = std::enable_if_t<std::is_convertible_v<S1, std::string>>>
    Teacher(S1&& n, S2&& p = "lecturer")
        : name(std::forward<S1>(n)), position(std::forward<S2>(p)) {};
};
```


其次，因为我们的默认参数是字面量，字面量是`const char[]`类型的。我们调用构造函数的时候，也会用字面量。字面量不是`std::string`类型会造成很多问题。然而在C++14中，我们可以用**User-defined literals**来把字面量声明成`std::string`类型的。不过记得命名空间，这个名字空间不在std中。我们这里不再讨论了。

我们接下来讨论用构造函数初始化的问题。初始化有很多种写法，以下我列出有限的几种。

```c++
Teacher t1("david"s);
Teacher t2 = Teacher("david"s);

Teacher t3{ "lily"s };
Teacher t4 = { "lily"s };
Teacher t5 = Teacher{ "lily"s };

auto t6 = Teacher("david"s);
auto t7 = Teacher{ "lily"s };
```


我们用了**auto**。然而auto是**decay**的。而**decltype(auto)**不。所以，以下代码如果用auto的话可能不是你需要的。

```c++
const Teacher& t8 = t1; 
auto t10 = t8;
```

我们需要写`const auto&`。

此外，我们可以看出，用小括号和大括号好像没什么区别。不过，在一些情况下会有很大的差别。我们列出一些。

```c++
std::vector<int> vec1(30, 5); std::vector<int> vec2{ 30, 5 };
```

甚至因为C++17的**构造函数自动推导**，我们可以写出更加疯狂的代码。

```c++
std::vector vec3{ vec1.begin(), vec2.end() };
```

这个代码是用初始化列表初始化的，也就是说我们得到的vec3中有两个iterator。

好了，我们回过头来说auto。我们可以看到好像我们所有的初始化都可以用auto。是这样吗？如果我们写atomic的代码呢？

```c++
auto x = std::atomic<int>{ 10 };
```

是可以的。因为在C++17中我们有**Copy Elision**。所以这里没有拷贝函数的调用，和直接定义并初始化是一致的。但是atomic初始化是有问题的。

```text
std::atomic<int> x{};
```

这样是不能零初始化的。当然了，这显然是API的不一致，或者说错误。所以我们有**LWG issue 2334**。预计在C++20修复这个问题。嘻嘻。

以上内容基于《C++ templates》的作者Nicolai Josuttis的几场talk。