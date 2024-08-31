公司限制本地的PC拉取代码，只能在云主机上面下载。

本地Ubuntu ping云主机是通的，ssh可用。

想了几种办法：

1. 通过NFS，把本地ubuntu的目录挂在云桌面上，在云上执行repo init&sync。这样在云桌面上下载，下载文件在本地。

   ----------实测可行，但是速度极慢，毕竟网络IO延迟远远大于本地磁盘IO。我下载了整整一天才把代码拉下来。

2. 在云桌面上搭建一个VPN服务器（比如clash、v2ray等）。本地配置指定IP走VPN。

3. 先在云桌面下载完整repo仓库，把它做完repo 镜像，然后本地Ubuntu从云桌面下载。

下面演示第三种方法。

1. 把本地的公钥加到云桌面的~/.ssh/authorized_keys文件。这步的目的是执行git指令时不要每次输密码。

2. 云桌面拉代码时，添加`--mirror`参数：

   ```
   // 在~/ft24mm/mirror目录下载代码
   repo init  -u ssh://xuexiangyu@192.168.64.47:29418/Android/8025/platform/manifest -b Toyota_24MM_dev --repo-url=ssh://xuexiangyu@192.168.64.47:29418/git-repo --mirror
   repo sync
   ```

   这种方式与正常拉取的目录结构不太一样，并且sync后不会检出，只有仓库。

   然后进入.repo/manifests目录，`git branch -avv`,我们发现本地只有一个default分支。所以后面的步骤，`-b Toyota_24MM_dev`都要改成`-b default`,否则会报Toyota_24MM_dev分支不存在。

   但是这样感觉比较奇怪，于是init后，我就在manifests里手动新建Toyota_24MM_dev分支，并关联到远程的Toyota_24MM_dev分支上。

   ```
   git checkout -b Toyota_24MM_dev
   git branch --set-upstream-to=origin/Toyota_24MM_dev Toyota_24MM_dev
   
   # 或者
   git checkout origin/Toyota_24MM_dev -b Toyota_24MM_dev
   git checkout remotes/m/Toyota_24MM_dev -b Toyota_24MM_dev //same
   ```

   ChatGPT给了我一个脚本，可以直接给所有远程分支在本地创建同名的分支：

   ```
   git branch -r | grep -v '\->' | while read remote; do git branch --track "${remote#origin/}" "$remote"; done
   ```

   

   有没有可能存在一些隐患？比如git仓库manifests有更新后，mirror同步后只有default指向最新提交，而Toyota_24MM_dev不动？

   Toyota_24MM_dev已经`set-upstream-to=origin/Toyota_24MM_dev`的话，应该不会出现这种情况。但不好肯定，从机制上分析，确实可能只更新了origin/branch, 本地branch落后几个提交。毕竟我没试过，假如万一会这样，我还是为此想了几个办法，

   1. 写个脚本，先到manifests目录，遍历每个分支，git pull origin <远程分支名>:<本地分支名>，然后再repo sync。
   2. 使用symbolic-ref，命令：

   ```
   git symbolic-ref refs/heads/Toyota_24MM_dev refs/heads/default
   ```

   实测当default移动后，Toyota_24MM_dev也跟着走，相当于Toyota_24MM_dev是default的别名。

   然而这样搞的话，如果后面重新repo init -b branch_new, 换到另一个分支，会有小麻烦，所以我不推荐这样搞。也许把Toyota_24MM_dev引用到`refs/remotes/origin/Toyota_24MM_dev`是个好主意,不过这不像常规方法，无法预料有什么隐患。实际行不行还要试过才知道。

3. 本地Ubuntu拉代码，需要把地址改成云桌面。

   ```
   repo init -u ssh://xue@192.168.60.154:22/home/xue/ft24/mirror/.repo/manifests.git -b Toyota_24MM_dev --repo-url=ssh://xue@192.168.60.154:22/home/xue/ft24/mirror/git-repo
   repo sync
   ```

4. 云桌面端也可以借助mirror检出：

   ```
   // 新建一个工作目录，注意mirror目录不是给你做工作目录的
   repo init  -u ssh://xuexiangyu@192.168.64.47:29418/Android/8025/platform/manifest -b Toyota_24MM_dev --repo-url=ssh://xuexiangyu@192.168.64.47:29418/git-repo --reference=/home/xue/ft24mm/mirror
   repo sync
   ```

   工作目录下的.repo目录，将会只包含远程仓库和miror的差异部分，体积很小。当有多个人在这台服务器上下载代码时，这种方式可以减小从远程git仓库下载的数据，充分复用，无需每人都下载一份完整的.repo。节约磁盘空间。修改提交的话，也只是提交在自己的.repo，不会影响mirror和其它人。

5. 写个定时脚本，每天夜里更新mirror。

6. 提交代码会比较麻烦，因为本地Ubuntu commit&push后，是push到mirror里。我们还需要到mirror再提交一次。

   ```
   //mirror里面
   cd mirror/atc/android/platform/frameworks/base.git
   git push hsaeyz HEAD:refs/for/Toyota_24MM_dev
   ```

   提示error，缺少change-id，这不是我的步骤问题，而是如果公司用gerrit管理的话，提交到gerrit上而不是直接入库，通过gerrit本来就需要change id。云桌面上直接repo下载，第一次提交时一样有这个错。原因是通过repo，git clone时没有下载hooks/commit-msg。根据报错提示自行解决。

   或者我们可以先把改动的代码，用ssh上传到云桌面上`--reference=/home/xue/ft24mm/mirror`下载的那份目录。然后在云桌面上面提交。如此，保持mirror纯净，永远只包含远程仓库的东西。缺点是，手动ssh上传过程中，可能存在人为失误，因此为确保正确，还得在云桌面上编译验证一次。

附录：

假如云桌面上同步时，不加--mirror，那么会有一系列问题：

1. 这时，本地Ubuntu直接从云桌面拉代码，会报一个错误：

   ```
   fatal: branch 'stable' has not been signed
   ```

   这是因为repo默认是找stable分支，`.repo/repo`目录处于default分支。需要先切到stable分支。

   除此之外，这一步还有其它问题。

2. 即使解决了repo init的所有问题，到下一步repo sync 时，还是会报错，而这以我目前能力无法手动解决。

3. 总之--mirror是谷歌提供的镜像repo的正规方式，安装正规方式做便是，不要自己去搞。