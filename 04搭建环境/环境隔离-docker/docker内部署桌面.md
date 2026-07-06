# 背景：

* 远程服务器是ubuntu server，无桌面环境。
* 开发人员不可以自主改造服务器，容易搞挂，原则上不允许。

# 目标：

* 在docker里运行桌面，通过容器端口转发，在外面以远程桌面方式连接。
* 轻量级。服务器没有显卡，靠CPU软渲染。因此不需要炫酷的视觉。
* 低网络带宽

# 对比：

| 远程方案             | 图像渲染位置           | 本地负载      | 网络带宽要求      | 适配性                |
| ---------------- | ---------------- | --------- | ----------- | ------------------ |
| Docker+VNC       | 服务器 Docker 容器内   | 极低（仅解码显示） | 中（压缩后传输）    | ✅ 最佳               |
| X11 转发（ssh -Y）   | 本地 Windows/macOS | 高（本地渲染）   | 高（传输原始绘图指令） | ❌ 外网卡顿、Windows 适配差 |
| RDP（xrdp+Docker） | 服务器 Docker 容器内   | 极低        | 中（RDP 编码）   | ⚠️ 容器内配置复杂、依赖重     |
| NoMachine+Docker | 服务器 Docker 容器内   | 极低        | 低（自研高速编码）   | ⚠️ 免费版有限制、容器化重     |

 X11 转发看似只传轻量的「绘图指令」，但实际带宽占用远高于 VNC/RDP。

* X11 协议是**几十年前的设计**，**原生不支持指令压缩、也不支持增量传输**
* 这些指令需要 **「客户端 - 服务端」频繁双向交互 **（指令下发 + 确认回执），哪怕单条指令很小，**高频次的网络握手、延迟累积**，会让实际带宽占用大幅上升
* 对于**现代软件**，其界面包含**大量矢量图形、字体渲染、多层窗口、动态特效**，对应的**X11 绘图指令总量会呈指数级增长**，甚至会**超过 VNC 压缩后的像素流大小**。**动态高频刷新**的：代码高亮、鼠标悬浮提示、窗口滚动、侧边栏折叠 / 展开、项目索引树刷新…… 每一个微小操作，都会触发**成百上千条细碎的 X11 绘图指令**

| 指标          | X11 转发（ssh -Y）    | VNC（Tight 编码）   |
| ----------- | ----------------- | --------------- |
| 单操作指令 / 帧大小 | 单滚动→几十 KB 的细碎指令   | 单滚动→几 KB 的增量像素流 |
| 网络交互次数      | 单操作→上百次双向交互       | 单操作→1 次单向推送     |
| 实际带宽占用      | 20-50Mbps         | 1-5Mbps         |
| 操作延迟        | 200-500ms（卡顿明显）   | 50-100ms（无感知）   |
| 外网适配性       | 完全无法使用（丢包 = 卡死后台） | 流畅使用（压缩抗丢包）     |

# 方案选择：

通过对比，选择的方案是: VNC（TigerVNC）+ Xfce4 / LXDE

* 适合docker容器内部
* xfce4是轻量桌面
* TigerVNC提供**远程连接**，分为服务端和客户端
* 稳、轻、快
* 可以纯 CPU 软渲染，Ubuntu Server 无独显也能跑

# 环境部署

1、 容器创建

```
docker run --name=xue_p32s --hostname=qcom_8155 --user=root:root --volume /home/xue:/home/xue --env=PYTHONIOENCODING=utf-8 --network=bridge --restart=always --runtime=runc --detach=true -p 5901:5901 -p 2222:22 -t 192.168.64.40:8090/nv626/nv626:1.0 /bin/bash
```

* 如果没有ssh需求，-p 2222:22不需要
* -p 5901:5901，容器内的端口不一定是5901，取决于容器内vnc server监听哪个端口,等于5900 + displayId。

2、容器内安装vnc

```
apt install --no-install-recommends -y xfce4 xinit xfce4-goodies xfce4-terminal dbus-x11 xterm
apt install --no-install-recommends -y tigervnc-standalone-server tigervnc-common
apt install -y xfonts-base xfonts-75dpi xfonts-100dpi libxfont2 libxtst6 libxrender1 libxi6 libgtk-3-0
```

3、配置文件

```
mkdir ~/.vnc
vi ~/.vnc/xstartup
#!/bin/bash
unset SESSION_MANAGER
unset DBUS_SESSION_BUS_ADDRESS

# 启动dbus服务
eval $(dbus-launch --sh-syntax)

# 启动XFCE桌面
startxfce4 &
#或，适合Docker ENTRYPOINT，不要加&
exec startxfce4 

#给这个脚本添加执行权限(重要，否则VNC服务将无法正常启动)
chmod +x ~/.vnc/xstartup
```

4、设置密码

```
vncpasswd
```

5、启动

```
# 创建脚本/usr/local/bin/start.sh
#!/usr/bin/env bash
rm -f /tmp/.X1-lock /tmp/.X11-unix/X1
export USER=xuexiangyu
vncserver :1 -localhost no -geometry 1920x1080 -depth 24

#停止：脚本/usr/local/bin/stop.sh
vncserver  -kill :1


# docker run时指定ENTRYPOINT 脚本
# 加-fg参数，否则脚本结束立即推出。
vncserver -fg :1 -localhost no -geometry 1920x1080 -depth 24
还有一些方法比如脚本最后一行加tail -f /dev/null防止推出。理论可能没试过。

#然后创建容器指定ENTRYPOINT 脚本为start.sh
docker run -itd --user $(id -u):$(id -g) --name=vnc --hostname=vnc --network host vnc_image:V1 /usr/local/bin/start.sh
```

端口是，5900+display id。由于这里“vncserver :1”，displayId是1，所以上面转发端口是5900+1=5901。如果指令是`vncserver :0 -geometry 1920x1080 -depth 24`,那么在第一部创建容器时，转发端口就应该改成5900：

```
docker run -p 5901:5900 xxxxx
```

或者，网络采用用host模式

```shell
docker run -itd --user $(id -u):$(id -g) --name=vnc --hostname=vnc  --network host vnc_image:V1 /bin/bash
```



6、客户端

https://sourceforge.net/projects/tigervnc/files/stable/

下载tigervnc64-1.16.2.exe。

不要下tigervnc64-winvnc-1.16.2.exe，这个是server，不是client。

开始菜单运行“TigerVNC”，VNC服务器填“192.168.66.252:5901”。

7、剪切板共享支持

在vnc viewer窗口桌面里面，开一个终端，执行：

```
vncconfig -nowin &
```

如果不生效，可能是:

1、装的tightserver而不是tigervnc-standalone-server。卸载，改装tigervnc-standalone-server

2、执行vncconfig位置不对，在vnc viewer里面，而不是ssh shell。

3、或者目标display不对，执行

```
export DISPLAY=:1
vncconfig -display :1 -nowin &
```

4、或许可以装一下xclip：

```
apt install xclip
```

在标准的 XFCE、GNOME 等桌面环境中，`vncconfig` 通常可以直接工作，理论上是不需要装这个的。但是有些轻量级窗口管理器，可能缺少剪贴板管理组件，`xclip` 可提高兼容性。这一步可作为排查手段之一。

# 输入法

ubuntu20不支持fcitx5, 采用fcitx4+rime

```
#1
sudo apt install -y fcitx fcitx-rime fcitx-config-gtk
#2 对于docker,~/.profile或许无效
echo 'export GTK_IM_MODULE=fcitx' >> ~/.profile
echo 'export QT_IM_MODULE=fcitx' >> ~/.profile
echo 'export XMODIFIERS="@im=fcitx"' >> ~/.profile
#3
fctix然后tab，提交fctix开头的命令，我们选择fcitx-configtool，其它好几个其实也行，打开输入法配置界面GUI。
点加号，去掉"Only Show Current Language",选择rime，OK。
rime输入法默认繁体，点扳手图标，点“Fcitx Rime Config”,弹窗界面，“Schema”。把“明月拼音*简化字”移到上面。保存。
#4
发现实际保存不了。于是手动改配置文件：
cd ~/.config/fcitx/rime/
build目录有个luna_pinyin_simp，就是简体中文。把它设置成默认即可。
work@qcom_8155:~/.config/fcitx/rime$ cat default.custom.yaml 
patch:
  schema_list:
    - schema: luna_pinyin_simp   # 简体拼音（默认）
    - schema: luna_pinyin        # 繁体拼音（备用）
    - schema: bopomofo           # 注音

#5 开机自启(严格来说，是在每次我启动vncserver时，自动执行fcitx.desktop)，而不是docker start。
cp /usr/share/applications/fcitx.desktop /etc/xdg/autostart/
```

# 常见问题

> Warning: xxxx:1 is taken because of /tmp/.X1-lock

因为以root身份运行过，tmp路径下有/tmp/.X1-lock文件，chmod是root：root，拒绝个人身份。

解决： 切到root， 删除tmp下所有文件。重新执行。

注意删除用"rm -rf .*", 因为"rm -rf *"不会删除点开头的隐藏文件。

# 现成的镜像

可以在dockerhub搜索一些别人做好的，省去自己配置环境。

* https://hub.docker.com/r/infrastlabs/docker-headless
  
  基于ubuntu20, 支持XRDP/NOVNC。可以mstsc+xrdp+tigervnc多屏。支持音频（xrdp+pulseaudio/noVNC+broadcast）
  
  作者还提供了一个镜像是集成了搜狗输入法的。镜像大小360M。
  
  ```bash
  docker pull infrastlabs/docker-headless:sogou
  #docker pull infrastlabs/docker-headless:ubuntu-24.04
  ```

* https://hub.docker.com/r/dorowu/ubuntu-desktop-lxde-vnc/
  
  这个镜像好多年不更新了。focal表示ubuntu 20.04
  
  ```bash
  docker pull dorowu/ubuntu-desktop-lxde-vnc:focal
  ```

* https://hub.docker.com/r/dewgenenny/docker-headless-vnc-container
  
  Debian 11 with `Xfce4` UI session,VNC和noVnc。还集成了火狐和Chromium浏览器。虽然ubuntu20是基于Debian 11，但是稳妥起见还是尽量直接用基于ubuntu构建的。

# 信息

```
apt install tigervnc-standalone-server tigervnc-common tigervnc-tools
sudo apt-get install tigervnc-scraping-server
```

tigervnc-standalone-server和tigervnc-scraping-server的区别是：

- **Standalone Server (独立服务器)**：创建一个全新的、虚拟的、独立的桌面环境，不依赖于物理显示器是否开启或登录
- **Scraping Server (抓取服务器)**：它不创建新桌面，而是像摄像头一样“抓取”当前物理屏幕上已经显示的内容，并将其共享出去
- **tigervnc-common**里面包含`vncconfig` ，vncconfig是用于配置剪贴板同步等功能的工具
- ubuntu22开始，额外安装tigervnc-tools，包含vncpasswd。之前的版本不要。
- ubuntu22搞不起来，要不连不上，要不连上黑屏。ubuntu20按前面手顺即可。

显然，我们远程连接的是ubuntu server，1、未必有物理显示器；2、多人连接，我们不应该影响别人；3、在docker内部。

所以显然我们应该选tigervnc-standalone-server。

其它相似的概念有：

* **X11 转发**：在本地机器上运行 Docker，并希望显示图形界面。也就是带GUI的程序安装在docker里，然后希望主机上显示出来。
* **Xvfb (虚拟帧缓冲区)**：不需要直接显示图形界面，可以使用 Xvfb 来模拟图形环境。也就是，我不需要界面，但程序必须在这个图形环境下才能跑，用它虚拟模拟图形环境但不真正显示。就像有些主机不接显示器不能开机接一个显卡欺骗器那样。

**TightVNC和TigerVNC**：

| **VNC 实现** | TightVNC                                   | TigerVNC                                                                                                              |
| ---------- | ------------------------------------------ | --------------------------------------------------------------------------------------------------------------------- |
| 对比         | (较老的实现，默认无加密，会话为虚拟桌面)                      | (更新的实现，性能更好，支持 TLS 加密)                                                                                                |
| 安装指令       | apt install  tightvncserver                | apt install tigervnc-standalone-server                                                                                |
| 启动         | vncserver :0 -geometry 1920x1080 -depth 24 | vncserver :0 -localhost no -geometry 1920x1080 -depth 24<br />TigerVNC 为了安全，默认只允许本机连接，启动时添加 `-localhost no` 参数，才能正常远程 |

**XFCE** 和**GNOME** ：对于远程桌面，强烈推荐 XFCE。

| 对比维度              | **XFCE (轻量级首选)**                                                                                   | **GNOME (功能型选手)**                                                                                                                                   |
|:----------------- |:-------------------------------------------------------------------------------------------------- |:--------------------------------------------------------------------------------------------------------------------------------------------------- |
| **设计理念**          | 轻量、快速、低资源消耗，追求极致效率                                                                                 | 美观、现代、交互体验丰富，功能完整                                                                                                                                   |
| **内存占用 (空闲)**     | **约 200 - 400 MB**，对云服务器非常友好                                                                       | **约 600 MB - 1.2 GB**，在 2GB 内存的机器上会比较吃力                                                                                                             |
| **远程流畅度**         | **响应迅速，操作几乎没有延迟**，体验接近本地                                                                           | 图形渲染开销大，在远程连接中**容易感觉卡顿、不跟手**                                                                                                                        |
| **配置难度**          | **配置简单，开箱即用**，与 VNC 兼容性极佳                                                                          | **配置复杂**，尤其在无显卡的 headless 服务器上，常需要模拟显示器 (EDID) 才能启动                                                                                                 |
| **适用场景**          | **低配服务器、开发板、注重流畅度的日常远程开发**                                                                         | 高配机器、本地使用、或特别需要 GNOME 特有生态的场合                                                                                                                       |
| ~/.vnc/xstartup内容 | `#!/bin/sh`<br/>`unset SESSION_MANAGER`<br/>`unset DBUS_SESSION_BUS_ADDRESS`<br/>`exec startxfce4` | `#!/bin/sh`<br/>`export GNOME_SHELL_SESSION_MODE=ubuntu`<br/>`export XDG_CURRENT_DESKTOP=ubuntu:GNOME`<br/>`exec /etc/X11/Xsession ubuntu-xsession` |

除了Xfce，LXQt、lXde也是很轻量的桌面，比较适合docker。LXDE 比 Xfce 更轻量。LXQt是 LXDE 的继任者，因为它基于 Qt 库，意味着资源占用相对 LXDE 稍高，接近 Xfce。