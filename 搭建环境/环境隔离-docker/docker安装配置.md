## Ubuntu 24.04, 安装命令：

```shell
sudo apt-get update
# 安装 apt 依赖包，用于通过HTTPS来获取仓库
sudo apt-get install apt-transport-https ca-certificates curl gnupg-agent software-properties-common
# 添加 Docker 的官方 GPG 密钥：
curl -fsSL https://mirrors.ustc.edu.cn/docker-ce/linux/ubuntu/gpg | sudo apt-key add -
# 验证您现在是否拥有带有指纹的密钥
sudo apt-key fingerprint 0EBFCD88
# 设置稳定版仓库
sudo add-apt-repository "deb [arch=amd64] https://mirrors.ustc.edu.cn/docker-ce/linux/ubuntu/ $(lsb_release -cs) stable"

sudo apt-get update
# 安装最新版本的 Docker Engine-Community 和 containerd
sudo apt-get install docker-ce docker-ce-cli containerd.io
#用户添加至docker组
sudo gpasswd -a ${USER} docker
# 或
sudo usermod -aG docker ${USER}

# 官方镜像源国内访问不了，修改镜像源和存储位置，国内镜像源到网上找
vi /etc/docker/daemon.json
{
  "registry-mirrors": [
    "https://docker.m.daocloud.io",
    "https://mirror.baidubce.com",
    "http://hub-mirror.c.163.com",
    "https://docker.nju.edu.cn"
  ],
  "data-root": "/mnt/docker" # 改到某个你的空间较大的盘上，默认是/var/bin/docker，在系统盘上。
}

# 重启PC
```



## 验证：

```shell
# 查看信息
docker info #如果docker服务起不来，这个指令会报错。

# 查看上面修改的data-root是否生效
 docker info | grep "Docker Root Dir"
 
# 查看当前有哪些镜像
docker images

# 查看有哪些容器
docker ps -a

# 看看data-root现在占用多大
sudo du -sh /mnt/docker #docker目录只有root能访问

# 查看启动情况
sudo service docker status

# 查看docker版本
docker version
```



## 镜像的操作指令：

```shell
# 获取一个新的镜像
docker pull ubuntu:18.04 # 如果不加18.04，下载的是ubuntu:lastest
然后就会看到：
$ docker images
REPOSITORY   TAG       IMAGE ID       CREATED         SIZE
ubuntu       18.04     f9a80a55f492   14 months ago   63.2MB

# 删除镜像
docker rmi ubuntu:18.04

# 从零开始来创建一个新的镜像
使用命令 docker build。因为我们完全可以从仓库pull镜像，然后在基础上修改。所有这个技能暂时不需要掌握。
```



## 容器的操作指令：

容器与镜像的关系类似于面向对象编程中的对象与类。先有镜像，然后根据镜像启动一个容器。

每次执行docker run都会生成一个新的容器。容器里的修改，影响不到下次docker run的新容器。所以我们只run一次，后面用start指令和exec指令进入容器。

```shell
# 查看指令用法
docker run --help

# 使用 ubuntu 镜像启动一个容器
docker run -it ubuntu:18.04 /bin/bash
记住要加 -it，否则没有交互式终端

然后执行：
docker ps -a
CONTAINER ID   IMAGE          COMMAND       CREATED          STATUS          PORTS     NAMES
3db1d4d112d6   ubuntu:18.04   "/bin/bash"   28 minutes ago   Up 28 minutes             blissful_faraday

我们发现NAMES是随机的。虽然后面可以根据CONTAINER ID 或 NAMES 进入这个容器，但是不方便。因为我们在run的时候就指定NAMES。并且大部分的场景下，我们希望 docker 的服务是在后台运行的，加 -d 参数。
docker run -itd --name dev_aosp ubuntu:18.04 /bin/bash # 加了 -d 参数默认不会进入容器

并且，很多时候我们要用的宿主的目录，使用-v。多个目录就多个-v。
docker run -itd --name dev_3ya -v /home/xue/3ya:/host_share ubuntu:18.04

查看容器：
$ docker ps -a
CONTAINER ID   IMAGE          COMMAND       CREATED          STATUS          PORTS     NAMES
b2af805e32df   ubuntu:18.04   "/bin/bash"   12 seconds ago   Up 11 seconds             dev_3ya
419cb8ff66d8   ubuntu:18.04   "/bin/bash"   3 minutes ago    Up 3 minutes              dev_aosp

dev_3ya的status是运行状态，所以直接用exec进入：
docker exec -it dev_3ya /bin/bash

检查容器和宿主，有没有共享目录成功，
在容器的shell里：
root@b2af805e32df:/# cd /host_share/
oot@b2af805e32df:/host_share# ll
total 8
drwxrwxr-x 2 1000 1000 4096 Aug 19 09:40 ./
drwxr-xr-x 1 root root 4096 Aug 19 09:36 ../
-rw-rw-r-- 1 1000 1000    0 Aug 19 09:40 test1
-rw-r--r-- 1 root root    0 Aug 19 09:40 test2

在宿主的shell里：
xue@xue-VMware-Virtual-Platform:~/3ya$ ll
总计 8
drwxrwxr-x  2 xue  xue  4096  8月 19 17:40 ./
drwxr-x--- 23 xue  xue  4096  8月 19 16:49 ../
-rw-rw-r--  1 xue  xue     0  8月 19 17:40 test1
-rw-r--r--  1 root root    0  8月 19 17:40 test2

-----------------》其中test2是容器里面新建的，test1是宿主创建的。可以看到他们的user不一样。

# 停止一个容器
docker stop dev_3ya
$ docker ps -a
CONTAINER ID   IMAGE          COMMAND       CREATED          STATUS                     PORTS     NAMES
b2af805e32df   ubuntu:18.04   "/bin/bash"   8 minutes ago    Exited (0) 5 seconds ago             dev_3ya

# 再次启动，进入
docker start dev_3ya
docker exec -it dev_3ya /bin/bash
# 或者start的时候直接进去
docker start -i dev_3ya

# 容器没有启动，是进不去的
$ docker exec -it dev_3ya /bin/bash
Error response from daemon: container b2af805e32df49a524c172cd2d38fa23e7a2bc27cfc4dfdd924aea7517fb2e1c is not running

# 删除容器
docker rm -f 1e560fca3906
docker rm -f 381520f56638 51a69fcfcee0 be935278e4e6 b01c70cd1bcf 3f8e982a44d5 # 一次性删除多个

```



## 安装常用工具

```
# 容器内什么都没有，需要手动安装很多常用工具包
apt update
# ping
apt install iputils-ping
# ifconfig
apt install net-tools

apt install gcc g++ git lrzsz # and so on

#安装中文字符集和设置中文字符支持
apt install language-pack-zh-hans
locale-gen zh_CN.UTF-8
```

## 将容器的当前状态保存为一个新的 Docker 镜像

```
docker commit -m="updated tools" -a="xue" dev_3ya 3ya:V1.0

$ docker images
REPOSITORY   TAG       IMAGE ID       CREATED         SIZE
3ya          V1.0      1c551453a088   4 seconds ago   111MB

docker run -it 3ya:V1.0

```

- **-a :**提交的镜像作者。
- **-c :**使用 Dockerfile 指令来创建镜像。
- **-m :**提交时的说明文字。

## 把修改后的镜像分享给别人

```
docker save导出镜像:
docker save -o D:\docker-images\springbootapp2-latest.tar lyhero11/springbootapp2

然后U盘拷到别的电脑上
docker load -i D:\docker-images\springbootapp2-latest.tar
```

###  docker save和docker export的区别：

1. docker save保存的是镜像（image），docker export保存的是容器（container）；
2. docker load用来载入镜像包，docker import用来载入容器包，但两者都会恢复为镜像；
3. docker load不能对载入的镜像重命名，而docker import可以为镜像指定新名称。
4. **两种方法不可混用**

## 中文乱码问题

默认情况下，下载的ubuntu18.04镜像，是不支持中文路径的，显示为乱码；文件内容也不能是中文，无法正常显示; 编译时，python默认语言已经是utf-8,但还是报编码错误。。。等等各种与编码有关的错误。

```
输入locale -a，查看一下现在已安装的语言，已经有C.UTF-8字符集
xuexiangyu@ft24mm:~$ locale -a
C
C.UTF-8
POSIX

输入locale查看下语言情况，发现全部是POSIX：
LANG=
LANGUAGE=
LC_CTYPE="POSIX"
LC_NUMERIC="POSIX"
LC_TIME="POSIX"
LC_COLLATE="POSIX"
LC_MONETARY="POSIX"
。。。
```

设置一下`LANG、LANGUAGE、LC_ALL`这三个就行了。LANG默认设置，`LC_*`没设值的时候就拿LANG；LANGUAGE是程序语言设置；LC_ALL强制设置所有`LC_*`


1.  配置Dockerfile镜像时，永久修改，在 Dockerfile 中添加

   ```
   ENV LANG C.UTF-8
   ENV LANGUAGE C.UTF-8
   ENV LC_ALL C.UTF-8
   ```

2. 方法1可能有些困难，因为我们业余用户不想去配置Dockerfile。那么下载的镜像，容器已经运行起来了，我们可以在容器内修改：

   ```
   #安装中文字符集和设置中文字符支持。 应该不是必要的，但也没什么坏处
   apt install language-pack-zh-hans
   locale-gen zh_CN.UTF-8
   # 执行后locale -a多了zh_CN.utf8 zh_SG.utf8
   root@d6e997049683:/# locale -a
   C
   C.UTF-8
   POSIX
   zh_CN.utf8
   zh_SG.utf8
   
   # 在/etc/bash.bashrc加入
   export LANG="C.UTF-8"
   export LC_ALL="C.UTF-8"
   export LANGUAGE="C.UTF-8"
   
   #重新运行容器，用locale检查
   docker restart  ft24mm
   docker exec -it ft24mm /bin/bash
   
   xuexiangyu@ft24mm:/$ locale
   LANG=C.UTF-8
   LANGUAGE=C.UTF-8
   LC_CTYPE="C.UTF-8"
   LC_NUMERIC="C.UTF-8"
   LC_TIME="C.UTF-8"
   LC_COLLATE="C.UTF-8"
   LC_MONETARY="C.UTF-8"
   LC_MESSAGES="C.UTF-8"
   LC_PAPER="C.UTF-8"
   LC_NAME="C.UTF-8"
   LC_ADDRESS="C.UTF-8"
   LC_TELEPHONE="C.UTF-8"
   LC_MEASUREMENT="C.UTF-8"
   LC_IDENTIFICATION="C.UTF-8"
   LC_ALL=C.UTF-8
   ```

   

3. 启动时或进入bash时候，设置字符集

   ```shell
   docker run -i -t --env LANG=C.UTF-8 --env LANGUAG=C.UTF-8 --env LC_ALL=C.UTF-8 ubuntu /bin/bash
   ```

   这个方法也是推荐的，不需要在容器里折腾了。如果忘了在docker run时指定uft8,后续也可以在docker exec进入容器是指定。但是每次exec命令都要加，就比较烦了，那就不如用方法2了。

## 时区问题

docker里的时区可能是0，希望设置成东八区。

```
#docker里面
mkdir -p /usr/share/zoneinfo/Asia/
#docker外面
docker cp /usr/share/zoneinfo/Asia/Shanghai imagename:/usr/share/zoneinfo/Asia/Shanghai
docker cp /etc/localtime imagename:/etc/localtime
docker cp /etc/timezone imagename:/etc/timezone

cat /etc/timezone 
Asia/Shanghai

```



## 用vscode看代码

由于不好在docker里面直接启动vscode，

1. ifconfig查看docker里面的ip，然后安装ssh服务，在外面用vscode+remote插件访问
2. 或者，创建容器的时候，-v参数，宿主的路径和容器里的路径使用一样的。直接在外面用vscode打开。

## 不同容器之间的网络访问

 Docker 中常见的两种网络模式下容器间网络访问的原理：

1. **桥接模式（Bridge）**：
   - 在桥接模式下，Docker 使用 Linux 桥接技术创建一个虚拟的网络桥接设备，所有容器都连接到该桥接设备上。
   - 每个容器都被分配一个唯一的 IP 地址，并且它们可以通过这些 IP 地址相互通信。
   - Docker 主机上运行的容器可以使用 NAT（Network Address Translation）机制来实现与外部网络的通信。
   - 当容器之间需要通信时，数据包会在它们之间通过 Linux 桥接设备转发。
2. **主机模式（Host）**：
   - 在主机模式下，容器与宿主机共享网络命名空间，它们直接使用宿主机的网络栈和接口。
   - 因此，在主机模式下，容器之间的网络访问就像是它们运行在同一台主机上的进程之间的通信，无需进行额外的网络转发或 NAT。
   - 容器可以直接使用宿主机上的 IP 地址和端口，因此在主机模式下，容器之间的通信速度可能会更快，但也可能会导致端口冲突和网络配置更改的困扰。

示例：

```
docker network create -d bridge test-net
```

暂不深究。有些需求，先考虑通过端口映射实现。实在不行再研究配网络。

## 网络端口映射

docker run 使用 **-P** 或 -p 绑定端口号, 容器端口绑定到主机端口。

两种方式的区别是:

- **-P：**是容器内部端口**随机**映射到主机的端口。
- **-p：**是容器内部端口绑定到**指定**的主机端口。

默认都是绑定 tcp 端口，如果要绑定 UDP 端口，可以在端口后面加上 **/udp**

### 网络其它机器通过ssh连接到docker里

首先，docker里面把网络工具和openssh-server包安装好，并完成ssh配置。宿主机是可以直接ping通容器的。先用宿主机尝试ssh连接试一下，确保容器内的ssh服务正常。

然后，网络上其它机器可以访问docker的宿主机，但是默认不能直接访问docker容器的ip。我们通过端口映射，访问宿主机的IP，并把端口映射到docker里。

```shell
docker run -itd -p 2222:22 --name dev_3ya 2222:22 ubuntu:18.04
docker exec -it dev_3ya /bin/bash
# 配好ssh
service ssh restart

# 假设宿主的IP是192.168.22.128。
ssh -p 2222 root@192.168.22.128 # 连到docker ssh
# ssh默认端口是22
ssh xue@192.168.22.128 # 连到宿主的ssh
```

当然也可以研究一下上面的配网桥的方法。

> docker容器里配置ssh，登录到root。需要：
>
> vim /etc/ssh/sshd_config
>
> PermitRootLogin yes
>
> UsePAM no

## docker容器内启动GUI

Docker 本身不支持直接运行 GUI 应用程序，因为它被设计为服务器端应用程序。可以通过一些特殊的方法来运行 GUI 程序。

一种常见的方法是使用 `X11` server 转发。这允许你在 Docker 容器内运行 GUI 程序，并且它们会在宿主机上显示。

## docker容器已经起来后，添加目录映射或端口映射

场景是，docker run创建容器时没想到需要映射，后来才意识到。

对于已经运行的 Docker 容器，不能直接添加端口映射。Docker 的端口映射是在容器创建时设置的，一旦容器运行，就不能动态地添加或更改端口映射。

目录映射也是一样的。

所以这种情况，应该这么做：

- 停止当前容器
- 使用 `docker commit` ，把当前容器保存为新的镜像
- 用新的镜像重新运行容器，并添加端口映射或目录映射

```shell
docker stop <container_id>
docker commit <container_id> my_new_image
docker run -p <host_port>:<container_port> my_new_image
```



## 和宿主用同样的用户

假设有一个共享的目录`-v /home/xue/3ya:/host_share`。docker里面创建的文件，用户是root，而宿主创建的文件所有者是xue。假设我在容器里面下载代码，宿主用IDE（比如vscode或Android Studio）打开代码。那么会存在权限问题。

首先，为了减少麻烦，容器内外使用同样的路径：`-v /home/xue/3ya:/home/xue/3ya`。

方案是，docker里面创建一个同样的用户，并默认进这个用户即可。

```shell
$ id
uid=1000(xue) gid=1000(xue) 组=1000(xue),4(adm),24(cdrom),27(sudo),30(dip),46(plugdev),100(users),114(lpadmin),988(docker)
```

内核是不区分用户来自docker还是宿主的。只要在docker里面创建用户，这个用户的id和group id也是1000。

```bash
# 运行容器时映射用户.$(id -u)的结果是1000，即userid
docker run -it --user $(id -u):$(id -g) your_image /bin/bash

#这时默认以用户1000进去，而docker里面没有这个用户，所以进去后是这样：
$ docker exec -it c3c58f0632a8 /bin/bash
I have no name!@c3c58f0632a8:/$ 

# 看着别扭， 我们以 root 身份进入容器
docker exec -it -u root your_container /bin/bash

# 在容器内创建用户。如果/home/xue已经存在，比如前面-v /home/xue/3ya会创建目录，那么这里会有些问题，没法在/home/xue下面创建必须得用户文件，并且/home/xue文件夹的权限也不对（-v映射时创建的用户是root，实际应该是xue）。所以应该先备份里面的文件，删除目录，再执行。或者前面先不要-v，后面再添加。
useradd -d "/home/xue" -m -u 1000 -s "/bin/bash" xue
```



最后，汇总前面的知识。我们常用的run指令，完整版参考如下：

```shell
docker run -itd --user $(id -u):$(id -g) --name dev_3ya --hostname dev_3ya -p 2222:22 -v /home/xue/3ya:/home/xue/3ya --env LANG=C.UTF-8 --env LANGUAG=C.UTF-8 --env LC_ALL=C.UTF-8 ubuntu:18.04
```

项目中每次使用时，直接从这拷贝，改改名字和路径。



## 指令大全

### 容器生命周期管理

- [run - 创建并启动一个新的容器。](https://www.runoob.com/docker/docker-run-command.html)
- [start/stop/restart - 这些命令主要用于启动、停止和重启容器。](https://www.runoob.com/docker/docker-start-stop-restart-command.html)
- [kill - 立即终止一个或多个正在运行的容器](https://www.runoob.com/docker/docker-kill-command.html)
- [rm - 于删除一个或多个已经停止的容器。](https://www.runoob.com/docker/docker-rm-command.html)
- [pause/unpause - 暂停和恢复容器中的所有进程。](https://www.runoob.com/docker/docker-pause-unpause-command.html)
- [create - 创建一个新的容器，但不会启动它。](https://www.runoob.com/docker/docker-create-command.html)
- [exec - 在运行中的容器内执行一个新的命令。](https://www.runoob.com/docker/docker-exec-command.html)

### 容器操作

- [ps - 列出 Docker 容器](https://www.runoob.com/docker/docker-ps-command.html)
- [inspect - 获取 Docker 对象（容器、镜像、卷、网络等）的详细信息。](https://www.runoob.com/docker/docker-inspect-command.html)
- [top - 显示指定容器中的正在运行的进程。](https://www.runoob.com/docker/docker-top-command.html)
- [attach - 允许用户附加到正在运行的容器并与其交互。](https://www.runoob.com/docker/docker-attach-command.html)
- [events - 获取 Docker 守护进程生成的事件。](https://www.runoob.com/docker/docker-events-command.html)
- [logs - 获取和查看容器的日志输出。](https://www.runoob.com/docker/docker-logs-command.html)
- [wait - 允许用户等待容器停止并获取其退出代码。](https://www.runoob.com/docker/docker-wait-command.html)
- [export - 将容器的文件系统导出为 tar 归档文件。](https://www.runoob.com/docker/docker-export-command.html)
- [port - 显示容器的端口映射信息。](https://www.runoob.com/docker/docker-port-command.html)
- [stats - 实时显示 Docker 容器的资源使用情况。](https://www.runoob.com/docker/docker-stats-command.html)

### 容器的root文件系统（rootfs）命令

- [commit - 允许用户将容器的当前状态保存为新的 Docker 镜像。](https://www.runoob.com/docker/docker-commit-command.html)
- [cp - 用于在容器和宿主机之间复制文件或目录。](https://www.runoob.com/docker/docker-cp-command.html)
- [diff - 显示 Docker 容器文件系统的变更。](https://www.runoob.com/docker/docker-diff-command.html)

### 镜像仓库

- [login/logout - 管理 Docker 客户端与 Docker 注册表的身份验证。](https://www.runoob.com/docker/docker-login-command.html)
- [pull - 从 Docker 注册表（例如 Docker Hub）中拉取（下载）镜像到本地。](https://www.runoob.com/docker/docker-pull-command.html)
- [push - 将本地构建的 Docker 镜像推送（上传）到 Docker 注册表（如 Docker Hub 或私有注册表）。](https://www.runoob.com/docker/docker-push-command.html)
- [search - 用于在 Docker Hub 或其他注册表中搜索镜像。](https://www.runoob.com/docker/docker-search-command.html)

### 本地镜像管理

- [images - 列出本地的 Docker 镜像。](https://www.runoob.com/docker/docker-images-command.html)
- [rmi - 删除不再需要的镜像。](https://www.runoob.com/docker/docker-rmi-command.html)
- [tag - 创建本地镜像的别名（tag）。](https://www.runoob.com/docker/docker-tag-command.html)
- [build - 从 Dockerfile 构建 Docker 镜像。](https://www.runoob.com/docker/docker-build-command.html)
- [history - 查看指定镜像的历史层信息。](https://www.runoob.com/docker/docker-history-command.html)
- [save - 将一个或多个 Docker 镜像保存到一个 tar 归档文件中。](https://www.runoob.com/docker/docker-save-command.html)
- [load - 从由 docker save 命令生成的 tar 文件中加载 Docker 镜像。](https://www.runoob.com/docker/docker-load-command.html)
- [import - 从一个 tar 文件或 URL 导入容器快照，从而创建一个新的 Docker 镜像。](https://www.runoob.com/docker/docker-import-command.html)

### info|version

- [info - 显示 Docker 的系统级信息，包括当前的镜像和容器数量。](https://www.runoob.com/docker/docker-info-command.html)
- [version - 显示 Docker 客户端和服务端的版本信息。](https://www.runoob.com/docker/docker-version-command.html)

### 网络命令

- **`docker network ls`**: 列出所有网络。
- **`docker network create <network>`**: 创建一个新的网络。
- **`docker network rm <network>`**: 删除指定的网络。
- **`docker network connect <network> <container>`**: 连接容器到网络。
- **`docker network disconnect <network> <container>`**: 断开容器与网络的连接。

详细内容查看：[docker network 命令](https://www.runoob.com/docker/docker-network-command.html")

### 卷命令

- **`docker volume ls`**: 列出所有卷。
- **`docker volume create <volume>`**: 创建一个新的卷。
- **`docker volume rm <volume>`**: 删除指定的卷。
- **`docker volume inspect <volume>`**: 显示卷的详细信息。

## Docker 资源汇总

### Docker 资源

- Docker 官方主页: [https://www.docker.com](https://www.docker.com/)
- Docker 官方博客: https://blog.docker.com/
- Docker 官方文档: https://docs.docker.com/
- Docker Store: [https://store.docker.com](https://store.docker.com/)
- Docker Cloud: [https://cloud.docker.com](https://cloud.docker.com/)
- Docker Hub: [https://hub.docker.com](https://hub.docker.com/)
- Docker 的源代码仓库: https://github.com/moby/moby
- Docker 发布版本历史: https://docs.docker.com/release-notes/
- Docker 常见问题: https://docs.docker.com/engine/faq/
- Docker 远端应用 API: https://docs.docker.com/develop/sdk/

### Docker 国内镜像

阿里云的加速器：https://help.aliyun.com/document_detail/60750.html

网易加速器：http://hub-mirror.c.163.com

官方中国加速器：https://registry.docker-cn.com

ustc 的镜像：https://docker.mirrors.ustc.edu.cn

daocloud：https://www.daocloud.io/mirror#accelerator-doc（注册后使用）