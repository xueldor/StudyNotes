## **为不同 Git 仓库配置不同的 ssh key**

为优化下载效率，最近把 Linux Lab 迁移到了码云，配置了不同的 ssh 私钥/公钥。为了避免在命令行每次都要额外指定不同的参数，可以添加一个配置文件。

例如，给码云的私钥文件命名为 `gitee.id_rsa`，把它放到 `~/.ssh` 目录下并修改权限。

```text
$ chmod 600 ~/.ssh/gitee.id_rsa
$ chmod 700 ~/.ssh
```

之后，新增一个 `~/.ssh/config`，加入如下配置：

```text
$ cat ~/.ssh/config
Host gitee
    HostName gitee.com
    IdentityFile ~/.ssh/gitee.id_rsa
    User git
```

这样就可以直接类似下面下载和上传，而无需每次输入密码或指定密钥了，同时省掉了 `git@`。

```text
$ git clone gitee:aaaa/yyyy.git

$ cd cloud-lab
$ touch xxxx
$ git add xxxx
$ git commit -s -m "add xxxx"

$ git push gitee:aaaa/yyyy.git master
```