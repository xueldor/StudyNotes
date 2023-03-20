1. snap list 查看有哪些软件

2. sudo snap remove --purge firefox卸载软件。将上一步list展示的软件逐个卸载

3. sudo apt autoremove --purge snapd gnome-software-plugin-snap

4.  删除`~/snap`目录。

5. 为了防止apt把snap再装回来，再/etc/apt/preferences.d/目录下创建一个nosnap.pref文件：

   ```txt
   Package: snapd
   Pin: release a=*
   Pin-Priority: -10
   ```

6. sudo apt update

7. 安装flatpak。

   ```shell
   sudo apt install flatpak
   flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
   ```

8. 通过flatpak安装软件：

   ```shell
   flatpak install flathub org.mozilla.firefox #火狐
   flatpak install chrome #  谷歌浏览器
   flatpak install scrot #一个截图软件
   flatpak install motrix #一个下载软件类似迅雷
   ```

9. 如果flatpak install时报GPG问题，执行`flatpak remote-delete flathub`移除这个仓库，然后重新添加，注意下文本前后不要有不可见字符。我之前是因为直接从网页上面拷贝命令，结果末尾多了一个类似回车一样的东西。当然，flathub这个仓库不需要GPG，但是如果你使用其他仓库，可能是真的要添加GPG。