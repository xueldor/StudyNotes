## 现象：卡顿

![image-20250619180438099](./imgs/%E9%AB%98IO%E7%9A%84%E5%88%86%E6%9E%90%E5%92%8C%E5%AE%9A%E4%BD%8D/image-20250619180438099.png)

常见原因：

![image-20250619184312300](./imgs/%E9%AB%98IO%E7%9A%84%E5%88%86%E6%9E%90%E5%92%8C%E5%AE%9A%E4%BD%8D/image-20250619184312300.png)

大体来说，通常伴随着CPU 100%、内存不足、IO过高。

## 确定原因

1、 通过指令查看资源占用（比如top、iotop）

![image-20250619184552877](./imgs/%E9%AB%98IO%E7%9A%84%E5%88%86%E6%9E%90%E5%92%8C%E5%AE%9A%E4%BD%8D/image-20250619184552877.png)

   1） com.android.providers.media.module的CPU比较高

   2） sys 94%, iow 30%, 表明有大量的系统调用，很有可能和IO有关。

```
iotop -m 5 -P -s io
iotop可能有点问题看不到各个应用的iow。因此后面通过抓trace。
```



下一步判断是否在持续的读设备。

2、确定是否providers.media这个进程,可以看到插入U盘后IO这列变高。

(top -Hp 2551 -o PID,TID,IO,USER,PR,NI,VIRT,RES,SHR,S,%CPU,%MEM,TIME+,CMDLINE -s 3)

插入前：

![image-20250620162816968](./imgs/%E9%AB%98IO%E7%9A%84%E5%88%86%E6%9E%90%E5%92%8C%E5%AE%9A%E4%BD%8D/image-20250620162816968.png)

插入后：

![image-20250620162653065](./imgs/%E9%AB%98IO%E7%9A%84%E5%88%86%E6%9E%90%E5%92%8C%E5%AE%9A%E4%BD%8D/image-20250620162653065.png)

![image-20250620162701937](./imgs/%E9%AB%98IO%E7%9A%84%E5%88%86%E6%9E%90%E5%92%8C%E5%AE%9A%E4%BD%8D/image-20250620162701937.png)

3. 抓取systrace:

   ![image-20250620163005984](./imgs/%E9%AB%98IO%E7%9A%84%E5%88%86%E6%9E%90%E5%92%8C%E5%AE%9A%E4%BD%8D/image-20250620163005984.png)

   一个线程上存在大量的“Uninterruptible Sleep”（橙色）

   ![image-20250620163358847](./imgs/%E9%AB%98IO%E7%9A%84%E5%88%86%E6%9E%90%E5%92%8C%E5%AE%9A%E4%BD%8D/image-20250620163358847.png)

   执行scanItem，可能是U盘性能不够，scanItem方法有82%的时间处于等待IO响应的状态。

4. U盘盘符是FAD7-D9D8，可以到/proc/目录下查看当前在访问U盘的哪个文件：

   ```shell
   ls /storage/                                       
   FAD7-D9D8  emulated  self
   cd /proc/2588/fd
   ls -l | grep FAD7-D9D8
   
   ```

   ![image-20250620164100150](./imgs/%E9%AB%98IO%E7%9A%84%E5%88%86%E6%9E%90%E5%92%8C%E5%AE%9A%E4%BD%8D/image-20250620164100150.png)

扫描U盘音乐文件。
