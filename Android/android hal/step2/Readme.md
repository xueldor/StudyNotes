在上一步的基础上，增加sysfs、procfs相关内容。这一步不是必须的，如果跳过这一步，你就需要手动/dev目录创建设备文件，就像上一步演示的那样。包括在proc目录创建freg、sysfs中创建属性文件这些都不是必须的。

涉及到的内核函数和结构：

* DEVICE_ATTR

  DEVICE_ATTR是一个宏定义： 

  ```c++
  #define DEVICE_ATTR(_name,_mode,_show,_store) struct device_attribute dev_attr_ ## _name = __ATTR(_name, _mode, _show, _store)
  ```

  `##`的意思是，把参数_name跟前面连起来，\##前后的空格是无所谓的，所以`DEVICE_ATTR(reg1, S_IRUGO | S_IWUSR, freg_reg1_show, freg_reg1_store)`扩展成：

  ```c++
  struct device_attribute dev_attr_reg1 = __ATTR(reg1, (00400|00040|00004) | 00200, freg_reg1_show, freg_reg1_store)
  ```

  `__ATTR`也是一个宏：

  ```c++
  #define __ATTR(_name, _mode, _show, _store) {				\
  	.attr = {.name = __stringify(_name),				\
  		 .mode = VERIFY_OCTAL_PERMISSIONS(_mode) },		\
  	.show	= _show,						\
  	.store	= _store,						\
  }
  ```

  __stringify仍然是一个define，作用是把传进来的参数用双引号包起来，变成字符串。比如这传进来的reg1, 那么相当于：

  ```c++
  struct device_attribute dev_attr_reg1 = {
  	.attr = {.name = "reg1",
  	....
  }
  ```

  扒到这里，`DEVICE_ATTR(reg1, S_IRUGO | S_IWUSR, freg_reg1_show, freg_reg1_store)`的意思很清楚了，就是定义了一个"struct device_attribute"类型的变量dev_attr_reg1，并给各属性赋值。

  如果不熟悉宏定义，可能会误以为reg1上前面定义的变量，然后尝试去找reg1在哪声明的。这是徒劳的，需周知。

  以上是从c语言的语法层面来说的。从功能角度，`DEVICE_ATTR`用途是在sys fs中添加“文件”，文件在/sys/devices/目录中对应的device下面。因此熟悉sysfs的应该能猜到，类似的还有DRIVER_ATTR，BUS_ATTR，CLASS_ATTR。相关文档见`<kernel_source>Documentation/driver-model/`目录下的txt。

* class_create(owner,name)

  调用后会在sysfs中创建一个设备类，表现为/sys/class/目录下会多一个name目录。

  owner, 拥有者,一般赋值为THIS_MODULE;

  name, 创建的逻辑类的名称

  例如：

  ```c++
  struct class *mem_class;
  mem_class = class_create(THIS_MODULE, "my_char_dev");
  ```

  返回一个指向class结构体的指针。

* device_create

  ```c++
  struct device *device_create(struct class *class, struct device *parent,
  			     dev_t devt, void *drvdata, const char *fmt, ...)
  ```

  在sysfs中创建设备，注册到指定的class。返回创建的设备（struct device*）。表现为，`/sys/class/<class_name>/`目录下再多一个目录：`/sys/class/<class_name>/<device_dir>`

* device_create_file

  ```c++
  int device_create_file(struct device *dev,
  		       const struct device_attribute *attr)
  ```

  第二个参数即前面DEVICE_ATTR声明的struct device_attribute对象。

  在`/sys/class/<class_name>/<device_dir>`目录下创建该设备的属性节点文件。即把设备的属性抽象成文件，然后可以通过VFS的读写方法来操作属性。所以你要实现好该属性的读写方法，设给device_attribute。

* 

分步调试代码

1. 加入class_create

```c++
//在/sys/class/目录下创建freg目录
    freg_class = class_create(THIS_MODULE, FREG_DEVICE_CLASS_NAME);
    if(IS_ERR(freg_class)){//class_create这种函数，返回值并不是只判断是否为NULL这么简单，应该用IS_ERR。
        err = PTR_ERR(freg_class);
        printk(KERN_ALERT"failed to create freg device class.\n");
        goto destory_cdev;
    }
```

加入这段代码后，出现`/sys/class/freg`目录。它是一个软链接，指向“../../devices/virtual/freg/freg”目录。

2. 加入device_create

   ```c++
   //创建freg设备。/sys/class/freg/目录下会创建一系列文件。/dev/目录会出现freg设备文件。
       struct device* temp = NULL;
       temp = device_create(freg_class, NULL, dev, NULL, "%s", FREG_DEVICE_FILE_NAME);
       if(IS_ERR(freg_class)){//返回值不能简单判断是否为NULL
           err = PTR_ERR(freg_class);
           printk(KERN_ALERT"failed to create freg device.\n");
           goto destory_class;
       }
   ```

   然后刷入设备，重启看到/sys/class/freg/目录下出现了许多文件：

   ```shell
   mek_8q:/sys/devices/virtual/freg/freg # ls -l
   total 0
   -r--r--r-- 1 root root 4096 2022-02-10 15:57 dev
   drwxr-xr-x 2 root root    0 1970-01-01 08:00 power
   lrwxrwxrwx 1 root root    0 2022-02-10 15:57 subsystem -> ../../../../class/freg
   -rw-r--r-- 1 root root 4096 1970-01-01 08:00 uevent
   
   #进入power目录
   mek_8q:/sys/devices/virtual/freg/freg/power # ls -l
   total 0
   -rw-r--r-- 1 root root 4096 2022-02-10 16:20 autosuspend_delay_ms
   -rw-r--r-- 1 root root 4096 2022-02-10 16:20 control
   -r--r--r-- 1 root root 4096 2022-02-10 16:20 runtime_active_time
   -r--r--r-- 1 root root 4096 2022-02-10 16:20 runtime_status
   -r--r--r-- 1 root root 4096 2022-02-10 16:20 runtime_suspended_time
   ```

   并且/dev目录出现freg文件，无需额外执行mknod命令。

   ```shell
   mek_8q:/dev # ls -l freg
   crw------- 1 root root 242,   0 1970-01-01 08:00 freg
   ```

   注意到权限是只有root才能读写，要改一下。串口下用vi打开ueventd.rc文件：

   ```shell
   mek_8q:/ $ busybox vi /ueventd.rc
   #新增一行
   /dev/freg                 0666   root       root
   
   #重启后
   130|mek_8q:/ $ ls -l /dev/freg
   crw-rw-rw- 1 root root 242,   0 1970-01-01 08:00 /dev/freg
   ```

   可以看到权限变成666了。

   如果想在安卓源码中改（而不是刷机后再改），ueventd.rc位于“system/core/rootdir/”目录。

   此时你要记住，`/sys/class/freg`里面的文件确实是device_create函数调用创建的，但是/dev/freg不是，而是由sysfs发送uevent，用户层守护进程ueventd收到消息后创建的（ueventd负责在/dev中创建设备节点，设置文件权限，设置selinux标签等）。

3. 加入DEVICE_ATTR等代码

   ```c++
   /* 设备属性操作方法*/
   static ssize_t freg_reg1_show(struct device* dev, struct device_attribute* attr, char* buf);
   static ssize_t freg_reg1_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t count);
   。。。省略freg_reg1_show及freg_reg1_store函数的实现部分
   //DEVICE_ATTR(_name, _mode, _show, _store)
   //扩展成struct device_attribute dev_attr_reg1 = __ATTR(reg1, (00400|00040|00004) | 00200, freg_reg1_show, freg_reg1_store)
   static DEVICE_ATTR(reg1, S_IRUGO | S_IWUSR, freg_reg1_show, freg_reg1_store);
   ```

   然后创建属性文件，把dev_attr_reg1传过去：

   ```c++
   //在/sys/class/freg/freg目录创建属性文件reg1
   err = device_create_file(temp, &dev_attr_reg1);
   if(err < 0){
       printk(KERN_ALERT"failed to create attribute reg1 of freg device.\n");
       goto destory_device;
   }
   dev_set_drvdata(temp, freg_dev);//对应freg_reg1_store和freg_reg1_show里的dev_get_drvdata
   ```

   编译烧录重启，然后我们看到`/sys/class/freg/freg`目录出现了一个reg1文件：

   ```shell
   mek_8q:/sys/class/freg/freg # ls -l
   total 0
   -r--r--r-- 1 root root 4096 2022-02-10 17:07 dev
   drwxr-xr-x 2 root root    0 1970-01-01 08:00 power
   -rw-r--r-- 1 root root 4096 2022-02-10 17:07 reg1
   lrwxrwxrwx 1 root root    0 2022-02-10 17:07 subsystem -> ../../../../class/freg
   -rw-r--r-- 1 root root 4096 1970-01-01 08:00 uevent
   ```

   可以通过往reg1写数据，来改变驱动里面reg1属性的值：

   ```shell
   mek_8q:/sys/devices/virtual/freg/freg # echo 97 > reg1                         
   mek_8q:/sys/devices/virtual/freg/freg # 
   mek_8q:/sys/devices/virtual/freg/freg # cat reg1                               
   97
   mek_8q:/sys/devices/virtual/freg/freg # cat /dev/freg                          
   a
   ---> 97对应的字符是a。
   ```

   

4. 在/proc/下面创建freg。

```c++
struct file_operations proc_fops = {
        .read = freg_proc_read,
        .write = freg_proc_write,
        .owner = THIS_MODULE
    };
/* 创建/proc/freg_dir/freg文件 */
static void freg_create_proc(void){
    entry_dir = proc_mkdir("freg_dir", NULL);//parent为NULL表示父目录为/proc
    ...
    entry = proc_create(FREG_DEVICE_PROC_NAME, 0, entry_dir, &proc_fops);
    ...

/* 删除/proc/freg_dir和里面的文件 */
static void freg_remove_proc(void){
    remove_proc_entry(FREG_DEVICE_PROC_NAME, entry_dir);
    proc_remove(entry_dir);
}
```

然后，可以在/proc目录看到"freg_dir"文件夹，里面可以看到freg文件。

```shell
mek_8q:/proc/freg_dir # ls -l
total 0
-r--r--r-- 1 root root 0 2022-02-11 15:26 freg
```

然后可以用echo 和 cat 命令测试读写：

```shell
mek_8q:/proc # echo a > freg_dir/freg                                          
mek_8q:/proc # 
mek_8q:/proc # cat freg_dir/freg                                               
a
```

OK!



## 问题记录

1. `ls /proc/freg_dir`报错，并且车机立即重启

[  860.142452] Internal error: Oops: 96000007 [#1] PREEMPT SMP

[  113.983062] CPU: 4 PID: 2566 Comm: ls Tainted: G        W       4.14.98-00074-g8509fe95d-dirty #30

原因： 传给proc_create的file_operations结构体是一个局部变量：

```c++
static void freg_create_proc(void){
    struct file_operations proc_fops = {xxx};
    entry = proc_create(FREG_DEVICE_PROC_NAME, 0, entry_dir, &proc_fops);
}
```

这属于低级错误，proc_fops放外面即可解决。

2. 将android_car_defconfig文件里CONFIG_FREG改成m，采用模块的方式加载freg.ko, 当rmmod freg后，`ls /proc/freg_dir`报错，并且车机立即重启

原因：在freg_remove_proc里面，为了拿到parent目录的proc_dir_entry对象，再次执行了proc_mkdir。应该采用全局变量，在freg_create_proc里执行proc_mkdir后，把指针记下来。

3. `echo w > /proc/freg_dir/freg`继续报错重启

   [  310.201433] Internal error: Oops: 96000004 [#1] PREEMPT SMP
   [  310.249078] CPU: 1 PID: 7272 Comm: sh Not tainted 4.14.98-00074-g8509fe95d-dirty #30

echo是写，对应freg_proc_write方法。通过内核日志（dmesg或cat /dev/kmsg）,知道是空指针导致的。原来`filp->private_data`是空的。之前写入/dev/freg时，我们在驱动的open函数里给private_data赋了值，所以echo /dev/freg时，private_data不是空的。但是/proc/freg_dir/freg不是设备文件，不是走原来的open函数。

那么可以直接把freg_open传给proc_fops.open吗？也不能。freg_open是提供给字符设备的，里面“inode->i_cdev”这些代码不能用在/proc/freg_dir/freg上面。

解决很简单，用全局变量fake_reg_dev* freg_dev即可。