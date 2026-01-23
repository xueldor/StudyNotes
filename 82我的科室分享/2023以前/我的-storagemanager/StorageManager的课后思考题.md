（仅代表个人理解。如有异议欢迎指正）

基于Android 11，回答以下问题：
1. 对照ppt第11页，解释访问/sdcard，最终到/data/media/0的过程。

   ----> 1) sdcard通过符号链接链接到/storage/emulated/0;

   2) /storage bind mount到/mnt/user/0

   3) 所以/storage/emulated/0即/mnt/user/0/emulated/0

   4) /dev/fuse挂载到/mnt/user/0/emulated

   5) 同时/storage/emulated和/data/media作为upper_path和lower_path传给fuse守护进程。

   6) fuse守护进程作为中转，转发到/data/media/0。

  其中/mnt/user/0、mount fuse等动作由vold在触发动作时创建执行；storage绑定到/mnt/user/0是app在fork时根据当前用户的user_id执行（ppt19页）。

2. 应用A在sdcard下创建文件，文件所有者和所属组是谁？A:应用A   B:media provider？  C: media_rw  D: shell

   ----》media provider

3. 开机后，/sdcard目录是什么时候开始可以访问？

   ----》unlock，即屏幕解锁后。

4. 出于性能考虑，想直接访问/data/media/0读取文件(绕过fuse)。是否可行？

   ----》不能。从需求角度，安卓显然不会允许这样做。从技术实现角度，目录“/data/media/0”的用户和用户组是media_rw（id=1023,定义在android_filesystem_config.h里），权限是rwxrwx---，所以除了root和media_rw自身，other用户不能访问此目录下的文件。

   里面的文件通常所有者是media provider，而other没有rwx权限，所以media_rw自身实际上也不能随意访问里面的文件。只有root可以无视权限。

5. Android 11之后，WRITE_EXTERNAL_STORAGE、READ_EXTERNAL_STORAGE、MANAGE_EXTERNAL_STORAGE 的作用

   ---》 MANAGE_EXTERNAL_STORAGE 可以管理存储卡上除了Android目录之外的所有文件，类似旧版本里同时申请READ_EXTERNAL_STORAGE和WRITE_EXTERNAL_STORAGE。

   READ_EXTERNAL_STORAGE可以查看其它用户创建的共享文件。

   WRITE_EXTERNAL_STORAGE已经废弃，11保留作为历史兼容，13完全没用。

6. uevent是通过哪种机制发送：socket、管道、共享内存、消息队列、binder？

   ----》 netlink socket

7. 基于sdcardfs, 介绍/mnt/runtime目录下default  full  read  write四个目录的作用。

   ----》sdcardfs通过将底层文件系统的目录挂载多次来解决文件权限的动态授权问题，而不需要重启进程。不同的bind mount的挂载点传入不同的挂载参数，这样通过不同的目录访问存储卡，就具有不同的读写权限。

   目标进程如果只有读权限，则将storage目录 bind到/mnt/runtime/read，只有写权限则bind到write，所有权限则bind到full。

   这种方式，权限控制的颗粒度比不上android 11使用的fuse，但是不需要在用户态和内核态之间来回切换，效率非常高。而且可以避免动态授权读写权限发生变更会导致应用重启的问题。

8. 判断对错：微信双开，两个账号互相感知不到对方存在（即数据是彼此不可见的）？

   ----> 是

9. 两个包名不同的应用，如何才能共享私有数据和keystore？

   ----》 在AndroidManifest.xml里声明同样的android:sharedUserId，并使用同样的签名。

10. 安卓为什么放弃fuse使用sdcardfs？到了安卓11，为什么又从sdcardfs回到fuse？

    ----》fuse有性能比较差，而sdcardfs是三星为了解决fuse的性能问题开发的技术，可以提供媲美直接访问底层文件系统的性能，也支持动态权限，所以谷歌引了进来。

    后来，为了支持分区存储，进行更好的权限控制，谷歌放弃了sdcardfs，重新使用fuse，并对性能做了优化。

11. 以ppt里演示的hellofs为例，对照ppt49页的fuse架构图，讲述命令“cat hello”,系统的执行过程。cat命令位于架构图哪个位置?

    ----》 1） hello文件的fd作为函数参数传到VFS

    ​          2） VFS识别出文件是普通文件，文件系统类型是fuse的，将调用交给内核的fuse驱动处理

    ​          3）userspace守护进程读取/dev/fuse,解析命令，进行文件操作

    ​          4） 守护进程将数据再次通过/dev/fuse/传递给内核

    ​          5）内核将数据返回给cat

       此时cat的角色相当于ppt49页左上角黄色Application。

12. fuse daemon什么时候destroy？

    ----》存储设备拔出后，执行umount，并销毁其对应的fuse daemon

13. 磁盘分区表格式分为GPT和MBR。请自行了解他们的区别和优势。

    ----》 略。。。

14. （开放题）思考前两天的科室例会上分享的问题。拔U盘时，vold会遍历/proc/pid/fd,找出占用U盘的进程，然后发送signal将进程杀死。如何合理的设计避免进程被杀？

    ----》略。。。