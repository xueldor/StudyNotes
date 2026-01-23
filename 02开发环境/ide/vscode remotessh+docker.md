在前文remote ssh插件配置好的基础上，安装插件“Dev Containers”,微软官方提供。

然后，ctrl+P，输入`>Dev Containers:Attach to Running Container`,选择容器，进入。

这时是用root身份进入的。可以选择：“>Dev Containers:Open Attached Container Configuration File”，打开配置文件：

```
// 加上remoteUser
{
	"workspaceFolder": "/home/xuexiangyu/h60a/source/android",
	"remoteUser": "zhangsan"
}
```

则下次以用户zhangsan进入。

