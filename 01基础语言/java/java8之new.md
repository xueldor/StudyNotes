已经知道*Java8*的新特性:*方法引用*。常用如下：

```java
registerListener(this::onEvent);
```

它其实仍然是Lambda表达式, 相当于：

```java
registerListener((参数)->{
    this.onEvent(参数);
})
```

As you see, 每个`this::onEvent`都是生成一个新的lambda。因此，这样移除listener是错的：

```java
unregisterListener(this::onEvent);//wrong
```

原因是，这里的`this::onEvent`和注册时的`this::onEvent`已经不是一个对象了。如果把`this::onEvent`赋给对象则没问题：

```java
XXXEvent ev = this::onEvent;
registerListener(ev);
unregisterListener(ev);//OK
```



有时会看到`XXX::new`, 是不是`XXX::new` 等于`new XXX()`？当然不是。`XXX::new` 和`XXX::funcX`类似，也是一个lambda。

错误写法：

```java
static class MyRunnable implements Runnable{
    public MyRunnable() {
        System.out.println("MyRunnable#constructor");
    }

    @Override
    public void run() {
        System.out.println("MyRunnable#run");
    }

}
public static void main(String[] args) {
		Thread thread = new Thread(MyRunnable::new);
		thread.start();
    
    //sleep 300ms
}
```

这段代码会打印`MyRunnable#constructor`，不会打印`MyRunnable#run`。因为:

```java
new Thread(MyRunnable::new);
//---->相当于
new Thread(()->{
    new MyRunnable();
});
//---->相当于
new Thread(new Runnable() {		
    @Override
    public void run() {
        new MyRunnable();
    }
});

//正确写法是：
Thread thread = new Thread(new MyRunnable()::run);
thread.start();
```



应用2：

```java
void fun1(Supplier<MyRunnable> supplier) {
    System.out.println(supplier.get());
}
fun1(MyRunnable::new);
```

为什么`supplier.get()`得到的是`MyRunnable`对象？因为fun1的参数是Supplier类型，Supplier是一个函数式接口：

```java
@FunctionalInterface
public interface Supplier<T> {
    T get();
}
```

so, `fun1(MyRunnable::new);`相当于：

```java
fun1(()-> new MyRunnable());
//相当于--》
fun1(new Supplier<MyRunnable>() {
    @Override
    public MyRunnable get() {
        return new MyRunnable();
    }
});
//note:
//(params) -> expression的返回值就是expression的结果，所以转成传统java语法后加了return
//( params ) -> { statements }返回值是statements的最后一个语句
```



