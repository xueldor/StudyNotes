通常，从一台已经配好的机器上，把.ssh目录拷贝到新环境，设置.ssh目录和里面文件的权限。然后就不需要重新在新机器里生成密钥、git网站添加public key了。

对于docker，我们经常和宿主环境共享home目录，因此docker里面是可以直接用git的，不需要配置密钥。

但是遇到一种情况：

```shell
xuexiangyu@test:~/Public$ git clone "ssh://xuexiangyu@192.168.64.47:29418/Android/QCOM_SA8155_6155/platform/hardware/ublox" && scp -p -P 29418 xuexiangyu@192.168.64.47:hooks/commit-msg "ublox/.git/hooks/"
Cloning into 'ublox'...
xuexiangyu@192.168.64.47: Permission denied (publickey).
fatal: Could not read from remote repository.

Please make sure you have the correct access rights
and the repository exists.
```

问题是宿主和其它的docker都是可以的。.ssh目录都是共享的一个。

我们单独执行后半段scp指令，加上-v参数（或-vvvv）：

```
xuexiangyu@test:~/Public$ scp -v -p -P 29418 xuexiangyu@192.168.64.47:hooks/commit-msg "ublox/.git/hooks/"
Executing: program /usr/bin/ssh host 192.168.64.47, user xuexiangyu, command scp -v -p -f hooks/commit-msg
OpenSSH_8.9p1 Ubuntu-3ubuntu0.10, OpenSSL 3.0.2 15 Mar 2022
debug1: Reading configuration data /etc/ssh/ssh_config
...略去
debug1: Authentications that can continue: publickey
debug1: Next authentication method: publickey
debug1: Offering public key: /home/xuexiangyu/.ssh/id_rsa RSA SHA256:mFeiUEpnNbTqo3TIouX1bNvixKBA7i+IUeVynRbeNxQ
debug1: send_pubkey_test: no mutual signature algorithm   <----------------------------here
debug1: Trying private key: /home/xuexiangyu/.ssh/id_ecdsa
debug1: Trying private key: /home/xuexiangyu/.ssh/id_ecdsa_sk
debug1: Trying private key: /home/xuexiangyu/.ssh/id_ed25519
debug1: Trying private key: /home/xuexiangyu/.ssh/id_ed25519_sk
debug1: Trying private key: /home/xuexiangyu/.ssh/id_xmss
debug1: Trying private key: /home/xuexiangyu/.ssh/id_dsa
debug1: No more authentication methods to try.
xuexiangyu@192.168.64.47: Permission denied (publickey).
```

看到这行：debug1: send_pubkey_test: no mutual signature algorithm。

正常执行的机器上，前面日志一样，到这里开始区别。百度搜索这行日志，参考这个网页：https://blog.csdn.net/qq_41765918/article/details/126837789。

原因是OpenSSH 8.8和更新的版本，默认使用 SHA-1 哈希算法禁用 RSA 签名。

解决方法：

1. 使用ed25519的算法，这是最佳的处理办法

   ssh-keygen -t ed25519 -C "email@example.com"

2. ssh命令里添加：`-o PubkeyAcceptedKeyTypes=+ssh-rsa`

3. 把PubkeyAcceptedKeyTypes=+ssh-rsa 添加到文件

   ```
   # vim /etc/ssh/ssh_config
   # 添加以下内容
   PubkeyAcceptedKeyTypes +ssh-rsa
   ```

4. 或者~/.ssh/config文件

   ```
   xue@myweb:~/.ssh$ cat config
   Host *
     HostkeyAlgorithms +ssh-rsa
     PubkeyAcceptedKeyTypes +ssh-rsa
   ```

   



后半段scp命令如果报： scp: Connection closed

 -O 参数可以让 scp 命令使用 SCP 协议而不是 SFTP 协议进行传输。

scp -p -O -P 29418 xuexiangyu@192.168.64.47:hooks/commit-msg "ublox/.git/hooks/"

即可。-O是大写的字母o, 不是数字零。

或者在bashrc里： `alias scp='scp -O'`