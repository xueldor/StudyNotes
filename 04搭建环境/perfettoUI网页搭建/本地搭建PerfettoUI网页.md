https://ui.perfetto.dev/#!/settings

这个网页有时国内打不开，刷很久才出来。

clone地址：

https://github.com/google/perfetto

或者从AOSP源码里找到：

```
platform/external/perfetto
```



按照ui/README.md指引即可。

```shell
$ git clone https://github.com/google/perfetto/
$ cd perfetto

# Install build dependencies
tools/install-build-deps --ui

# Will build into ./out/ui by default. Can be changed with --out path/
# The final bundle will be available at ./ui/out/dist/.
# The build script creates a symlink from ./ui/out to $OUT_PATH/ui/.
ui/build

# This will automatically build the UI. There is no need to manually run
# ui/build before running ui/run-dev-server.
ui/run-dev-server
```

注意，

1、 需要Ubuntu环境，20以上，18不行。window上编译不了。

2、 第一个指令执行时需要访问外网，服务器不能访问外网的话，可以在本地ubuntu上执行，然后拷贝到服务器

3、 Ubuntu18的话，可以用docker

4、最后一个命令`ui/run-dev-server`改成`./ui/build -s --serve-host 0.0.0.0 -w`。因为默认监听127.0.0.1，那么只能本地访问。改成0.0.0.0才能从外面访问。

5、网页转圈很久，通过浏览器F12 debug，发现是storage.cloud.google.com访问不了。打开

`ui/src/frontend/is_internal_user_script_loader.ts `,搜索这个地址，把tryLoadIsInternalUserScript函数内部注释掉即可,函数内部使用到的变量也要注掉。或者把storage.cloud.google.com改成127.0.0.1，或改成一个随意的不存在地址都能减少转圈的时间。

6、如果在docker内部运行，除了容器的端口映射（端口号10000）之外，退出容器可能导致./ui/build一起退出。需要用nohup运行。

```
# 转圈的问题 
const SCRIPT_URL =
  'https://xxxxxxxxxstorage.cloud.google.com/perfetto-ui-internal/internal-data-v1/amalgamated.js';

#docker内部
nohup ./ui/build -s --serve-host 0.0.0.0 -w &
```

