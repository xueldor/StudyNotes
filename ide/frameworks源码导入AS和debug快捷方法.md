Android Studio新建一个安卓项目，在build.gradle里添加：

```groovy
sourceSets {
    main.java.srcDirs += 'D:\\sourcecode\\C801CSC1\\frameworks\\base\\core\\java'
    main.resources.srcDirs += 'D:\\sourcecode\\C801CSC1\\frameworks\\base\\core\\res\\res'
}
```

frameworks\\base\\core\\java代码拷贝到指定目录，即可。

debug时，attach到对应进程，调试framework一般是system_process进程。

此方法相比前面介绍的，更高效、快速准备好环境、不依赖linux和整个源码。不需要AS里面额外的configurations。