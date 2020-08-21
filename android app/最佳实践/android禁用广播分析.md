**各位Android开发的同事：**

​			请注意！ 在很早之前，我就强调过慎用Broadcast广播作为跨进程通信的方式，目前在CCS项目上应该有过深刻的体会。

​			不过，我发现越来越多的第三方应用喜欢用Broadcast广播作为对外接口通信方式，例如讯飞新版就从aidl接口改为broadcast。

  但如若对broadcast广播的特点不清楚的话，在膨胀的广播通信中将带来灾难性后果。

  

  		广播通信的优点我不列举了，对于我们来说，他的最大缺点是其安全性： 广播在系统中是以单线程串行方式 发送端发出，接收端接收处理，

 		 这样就会有阻塞的情况出现。这不光是会影响效率的问题，在CCS5.0 POC联调过程中，发现了一个极端的情况，共享给大家：

  问题：在联调语音唤起导航的时候，发现一直无响应（语音发送AUTONAVI_STANDARD_BROADCAST_RECV+param的广播协议）

  分析过程：

1. 通过 dumpsys | grep BroadcastRecord 查看广播，发现广播记录一直卡在最后一条

​    BroadcastRecord{cfc645d u0 AUTONAVI_STANDARD_BROADCAST_SEND} to user 0

​    BroadcastRecord{af941d2 u0 AUTONAVI_STANDARD_BROADCAST_SEND} to user 0

​    BroadcastRecord{89b32a3 u0 AUTONAVI_STANDARD_BROADCAST_SEND} to user 0

​    BroadcastRecord{9469a0 u0 AUTONAVI_STANDARD_BROADCAST_SEND} to user 0

​     \- BroadcastRecord{f7eaa9a u0 com.iflytek.autofly.handMessage}

 

2.  对比分析：通过模拟广播消息看看导航有无反应：

​    通过：am broadcast -a AUTONAVI_STANDARD_BROADCAST_RECV --ei KEY_TYPE 10011 --es EXTRA_ADDRESS "大润发" --es SOURCE_APP "Third App" 

​    结果：Broadcasting: Intent { act=AUTONAVI_STANDARD_BROADCAST_RECV flg=0x400000 (has extras) }

​      （这里无返回，正确的话是Broadcast completed: result=0）

 

3.  分析com.iflytek.autofly.handMessage的广播，是voiceadapter接收讯飞的广播消息处理platform动作的

![image005](img/image005.jpg)

4. 进而判断出是voiceadapter出问题了。通过差异分析，是由于voiceadapter中循环绑定讯飞服务绑定不上，接收到广播消息就进程崩溃，然后重启、崩溃（这是另外一个问题）

 

​    结论：接收端如若有问题了，不仅影响广播通信的效率，极端情况会造成所有广播消息都收不到。

​    过程中我们或许不能强制规定第三方不使用广播，但接收端的健壮性和高效率要得以保证；以及上述经验以供参考。谢谢！