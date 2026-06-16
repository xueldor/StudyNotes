如果支持ssh，可以用sshfs。

如果支持webdav，工具多了。

通用的、支持多种协议、各种网盘的，有：

* rclone
* openlist
* Cryptomator

# rclone

先安装**WinFsp 驱动**

rclone.conf配置文件：

```
C:\Users\name_xxx\AppData\Roaming\rclone\rclone.conf
[OSS]
type = s3
provider = Alibaba
access_key_id = LTAI5tKiVLm*********
secret_access_key = JMzmFb8p93O********
endpoint = http://oss-cn-shenzhen.aliyuncs.com
acl = private

[252-ssh]
type = sftp
host = 192.168.66.252
user = zhangsan
key_file = ~/.ssh/id_rsa
port = 22
shell_type = unix
md5sum_command = md5sum
sha1sum_command = sha1sum
```

当然，这个文件是rclone命令里输入，然后自动生成的。不过如果已经生成了，也可以直接拷贝过去，或手动创建文件，一样的。

挂载命令：

```
# windows bat，将OSS挂到O盘，252服务器挂到P盘
set PATH=%PATH%;D:\soft\rclone-v1.73.2-windows-amd64
rclone mount OSS:/hangshengbuket/yangzhou/zhangsan/ O: --cache-dir D:\disk-cache --vfs-cache-mode writes
rclone mount 252-ssh:/home/zhangsan/ P: --cache-dir D:\disk-cache --links --vfs-cache-mode writes
#如果是linux，还支持--daemon参数
#避免这行命令阻塞 start /b rclone mount xxxx

#特别地 将本地E盘重新映射到J盘符。通过J:\访问文件，避免被公司加密系统加密
rclone mount E:\ J: --file-perms 0777 --dir-perms 0777 --vfs-cache-mode writes
```

windows有一些三方工具，可以把bat脚本包装成服务，避免一个cmd窗口一直开着。

## GUI

* windows上有RcloneBrowser、Rclone UI等工具，避免记忆复杂的指令。
* rclone自带gui: `rclone rcd --rc-web-gui --rc-user admin --rc-pass 123456`,首次执行会下载webgui压缩包，5M左右。浏览器打开：`http://127.0.0.1:5572/` 即可。
* 安卓端，F-Droid上面有个app叫："Round Sync",是对rclone的封装。直接搜“rclone”就会出来。当然，不止这一个应用。其它几个，有些是多年未更新的，不要下载。还有些应用是提供其它增强功能的，不了解不必下载。

# openlist

和rclone相比，openlist的界面更友好，操作简单，自行摸索摸索就会了。

* 不要下载alist。

  openlist是原alist被卖给黑心企业后，社区维护的开源版本。alist被卖后，开始在后台偷偷采集用户数据、盗取信息。

* 安卓端对应的应用是alistlite,虽然名字还叫alist，但和黑心公司无关，方向使用。下面给出一些使用上的经验

  > 1、123网盘支持webdav和开发者平台，故有三两种添加方式：
  >
  > ​     1）alistlite添加时驱动选”123云盘“。
  >
  > ​     2）123网盘app进入 “第三方挂载”，开启webdav，然后alistlite添加时选webdav。
  >
  > ​     3）注册开发者，alistlite添加时选“123云盘开放平台”
  >
  > 性能上，实测下来第一种比webdav方式好一点。第一种的缺点在于，非官方支持，可能会被查封。
  >
  > 2、手机设置允许alistlite高耗电，否则当alistlite在后台时其它应用用不了(和具体手机有关，国产手机对后台应用管控较严)。
  >
  > 3、用admin登录，后台管理界面才能看到各种功能。默认密码admin 123456  
  >
  > 4、添加存储时，webdav策略选本机代理。302重定向不能预览、不能缩略图，且很多app不支持。
  > url是http://你的手机IP:5244/dav，端口写5244，user和密码就是admin 123456。这样就通过alistlite分享了webdav服务，像百度网盘这种官方不支持webdav的，通过alistlite中转，让三方应用能够使用。
  >
  > 5、加密功能:
  > 先添加一个存储，挂载到比如abc,
  > 再添加存储，驱动选Crypt,序号要比abc大一点，保证先挂载abc。文件名加密选标准。文件夹加密也打开。加密文件存储路径写abc/子路径。
  > 假如alistlite某天连不上了（比如被网盘封禁了），只要网盘账号自身没封，可以下载到本地，添加存储，驱动选本机存储，同样方式再添加Crypt。只要密码和盐一样，就能解密。

# Cryptomator

支持webdav和S3协议。用来实现网盘加密存储。即：先用Cryptomator连网盘，然后通过Cryptomator上传文件。Cryptomator会自动对文件进行加密。网盘app里看到的是密文，只有通过Cryptomator查看文件，才是明文。

相比使用网盘自身提供的“加密文件夹”，好处是，网盘厂家不知道你的密钥，无法解密。Cryptomator本身是开源的，同行审查，不可能隐藏后门。

Cryptomator的加密等级，比openlist更高。而且，openlist虽然可以加密文件内容、目录名、文件名，但是不打乱目录文件结构。而Cryptomator打乱目录层次，属于最高级的安全，绝对无法攻破。

Cryptomator也支持本地存储。就是加密文件存本地，手机文件管理里看到是密文，Cryptomator才看到明文。和alistlite一样，如果某天连不上了，只要网盘账号自身没封，就可以下载到本地，添加本地存储，然后就能看到并且导出明文了。

**tips:** 

* 密码一定不能忘记，否则神仙也不能帮你解密。 

* Cryptomator没有分享webdav功能，流媒体在线播放，还是用alistlite。
* 电脑版可以映射本地驱动器。安卓就别想了。
* 安卓版需要申请licence。由于代码是开源的，可以自己破解。

## cryptomator-android去除licence

1、 下载源码，导入android studio之前，先修改maven仓库，否则国内同步失败：

```
//build.gradle
repositories {
-		mavenCentral()
-		maven {
-			url "https://maven.google.com"
-		}
-		google()
+		maven { url 'https://maven.aliyun.com/repository/google' }
+		maven { url 'https://maven.aliyun.com/repository/public' }
+		maven { url 'https://maven.aliyun.com/repository/gradle-plugin' }
+//		mavenCentral()
+//		maven {
+//			url "https://maven.google.com"
+//		}
+//		google()
 	}
```

2、 如果没有通过git下载源码，getVersionCode会返回-1, 是不允许的，改成1即可：

```
//build.gradle
def getVersionCode = { ->
	try {
	.....
	}
	catch (ignored) {
		return 1  // <--------这里
	}
}
```

3、 默认debug版用谷歌默认签名，而release版没有签名，我们自己添加，可以还用谷歌签名，也可以使用自己的。

```
//presentation/build.gradle
+		mycustomkey {
+			keyAlias 'platform'
+			keyPassword 'android'
+			storeFile file('my.keystore') // <-----你的自定义签名，没有就还用debug.keystore
+			storePassword 'android'
+		}

 	buildTypes {
 		release {
+			signingConfig signingConfigs.mycustomkey
 			crunchPngs false
```

4、去掉checkLicense

```
// presentation/src/main/java/org/cryptomator/presentation/presenter/VaultListPresenter.kt
-		checkLicense()
+//		checkLicense()
```

完整patch文件见附件。