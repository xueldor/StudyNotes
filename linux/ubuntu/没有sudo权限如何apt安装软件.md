背景：

公司分配的服务器，每个人有自己的用户，但是不知道root密码。

apt install命令因为会往 /usr/bin，/usr/lib，/usr/share等目录写文件，所以需要sudo apt。

1. `apt download lrzsz`

   这个命令把lrzsz这个软件的deb下载到当前目录

2.  `dpkg -x lrzsz_0.12.21-10_amd64.deb ~/aptsoft/`

   这个命令从deb软件包中提取文件，解压到指定目录。

3. .bashrc文件追加：

   ```shell
   export PATH="$PATH:~/aptsoft/usr/bin/"
   export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:~/aptsoft/usr/lib"
   ```

   path是为了能找到可执行文件；LD_LIBRARY_PATH是为了其他程序能使用动态库(.so)文件。表示这个目录成为系统动态库搜索目录。

参考: https://blog.csdn.net/qq_41314786/article/details/114273847