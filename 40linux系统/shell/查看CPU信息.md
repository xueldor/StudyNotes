CPU相关信息都从/proc/cpuinfo 获取。

总核数 = 物理CPU个数 X 每颗物理CPU的核数
总逻辑CPU数 = 物理CPU个数 X 每颗物理CPU的核数 X 超线程数

查看物理CPU个数

```bash
cat /proc/cpuinfo| grep "physical id"| sort| uniq| wc -l
1
```

查看每个物理CPU中core的个数(即核数)

```bash
cat /proc/cpuinfo| grep "cpu cores"| uniq
1
```

查看逻辑CPU的个数

```bash
cat /proc/cpuinfo| grep "processor"| wc -l
1
```

查看线程数

```bash
grep 'processor' /proc/cpuinfo | sort -u | wc -l    
1
```

注意，此处查看的线程数是总的线程数，可以理解为逻辑cpu的数量



查看实时频率

```
watch -d -n 1 grep \'cpu MHz\' /proc/cpuinfo
```



不包含温度信息。因为温度本质是从传感器获取的。



lscpu命令： CPU 的综合信息， CPU 数量和核心数，线程数，架构，操作模式（ 32-bit, 64-bit），缓存等

```shell
Architecture:        x86_64
CPU op-mode(s):      32-bit, 64-bit
Byte Order:          Little Endian
CPU(s):              16
On-line CPU(s) list: 0-15
Thread(s) per core:  1
Core(s) per socket:  8
Socket(s):           2
NUMA node(s):        1
Vendor ID:           GenuineIntel
CPU family:          6
Model:               85
Model name:          Intel(R) Xeon(R) Gold 5118 CPU @ 2.30GHz
Stepping:            4
CPU MHz:             2294.611
BogoMIPS:            4589.22
Hypervisor vendor:   VMware
Virtualization type: full
L1d cache:           32K
L1i cache:           32K
L2 cache:            1024K
L3 cache:            16896K
NUMA node0 CPU(s):   0-15
Flags:               fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush mmx fxsr sse sse2 ss ht syscall nx pdpe1gb rdtscp lm constant_tsc arch_perfmon nopl xtopology tsc_reliable nonstop_tsc cpuid pni pclmulqdq ssse3 fma cx16 pcid sse4_1 sse4_2 x2apic movbe popcnt tsc_deadline_timer aes xsave avx f16c rdrand hypervisor lahf_lm abm 3dnowprefetch cpuid_fault invpcid_single pti ssbd ibrs ibpb stibp fsgsbase tsc_adjust bmi1 hle avx2 smep bmi2 invpcid rtm mpx avx512f avx512dq rdseed adx smap clflushopt clwb avx512cd avx512bw avx512vl xsaveopt xsavec xsaves arat pku ospke md_clear flush_l1d arch_capabilities
```



lshw命令的输出中也包含一些cpu的信息：

```shel
xuexiangyu@S111-CCS2plus:~/ft24mm$ sudo lshw
s111-ccs2plus               
    description: Computer
    product: VMware Virtual Platform
    vendor: VMware, Inc.
    version: None
    serial: VMware-56 4d af 54 5a 25 eb 9e-5a db 35 91 ce 5d e4 6b
    width: 64 bits
    capabilities: smbios-2.7 dmi-2.7 smp vsyscall32
    configuration: administrator_password=enabled boot=normal frontpanel_password=unknown keyboard_password=unknown power-on_password=disabled uuid=564DAF54-5A25-EB9E-5ADB-3591CE5DE46B
  *-core
       description: Motherboard
       product: 440BX Desktop Reference Platform
       vendor: Intel Corporation
       physical id: 0
       version: None
       serial: None
     *-firmware
          description: BIOS
          vendor: Phoenix Technologies LTD
          physical id: 0
          version: 6.00
          date: 07/29/2019
          size: 86KiB
          capabilities: isa pci pcmcia pnp apm upgrade shadowing escd cdboot bootselect edd int5printscreen int9keyboard int14serial int17printer int10video acpi smartbattery biosbootspecification netboot
     *-cpu:0
          description: CPU
          product: Intel(R) Xeon(R) Gold 5118 CPU @ 2.30GHz
          vendor: Intel Corp.
          physical id: 1
          bus info: cpu@0
          version: Intel(R) Xeon(R) Gold 5118 CPU @ 2.30GHz
          slot: CPU #000
          size: 2300MHz
          capacity: 4230MHz
          width: 64 bits
          capabilities: x86-64 fpu fpu_exception wp vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush mmx fxsr sse sse2 ss ht syscall nx pdpe1gb rdtscp constant_tsc arch_perfmon nopl xtopology tsc_reliable nonstop_tsc cpuid pni pclmulqdq ssse3 fma cx16 pcid sse4_1 sse4_2 x2apic movbe popcnt tsc_deadline_timer aes xsave avx f16c rdrand hypervisor lahf_lm abm 3dnowprefetch cpuid_fault invpcid_single pti ssbd ibrs ibpb stibp fsgsbase tsc_adjust bmi1 hle avx2 smep bmi2 invpcid rtm mpx avx512f avx512dq rdseed adx smap clflushopt clwb avx512cd avx512bw avx512vl xsaveopt xsavec xsaves arat pku ospke md_clear flush_l1d arch_capabilities
          configuration: cores=8 enabledcores=8
        *-cache:0
             description: L1 cache
             physical id: 0
             slot: L1
             size: 16KiB
             capacity: 16KiB
             capabilities: asynchronous internal write-back
             configuration: level=1
        *-cache:1
             description: L1 cache
             physical id: 1
             slot: L1
             size: 16KiB
             capacity: 16KiB
             capabilities: asynchronous internal write-back
             configuration: level=1
     *-cpu:1
          description: CPU
          product: Intel(R) Xeon(R) Gold 5118 CPU @ 2.30GHz
          vendor: Intel Corp.
          physical id: 2
          bus info: cpu@1
          version: Intel(R) Xeon(R) Gold 5118 CPU @ 2.30GHz
          slot: CPU #001
          size: 2300MHz
.......
```



mpstat  1以及top查看CPU使用率

```shell
xuexiangyu@S111-CCS2plus:~/ft24mm$ mpstat  1
Linux 4.15.0-74-generic (S111-CCS2plus)         09/03/24        _x86_64_        (16 CPU)

15:22:44     CPU    %usr   %nice    %sys %iowait    %irq   %soft  %steal  %guest  %gnice   %idle
15:22:45     all   91.50    0.00    8.00    0.06    0.00    0.44    0.00    0.00    0.00    0.00
15:22:46     all   94.31    0.00    5.56    0.00    0.00    0.12    0.00    0.00    0.00    0.00
15:22:47     all   96.08    0.00    3.92    0.00    0.00    0.00    0.00    0.00    0.00    0.00
```

CPU 使用率实际上可以根据/proc/stat计算获取：

```shell
xuexiangyu@S111-CCS2plus:~/ft24mm$ cat /proc/stat 
cpu  34792736 12382 6879079 19806711158 2413972 0 419436 0 0 0
cpu0 2192352 712 352321 1238211482 41269 0 23857 0 0 0
cpu1 2160411 198 330320 1237338290 41454 0 32114 0 0 0
cpu2 2167799 92 323278 1238356672 38901 0 11167 0 0 0
cpu3 2121897 528 473967 1237099506 40988 0 12430 0 0 0
cpu4 2149353 339 374978 1238202604 36401 0 15854 0 0 0
....

读这个文件，然后可以通过算法计算出总的占用率：
    public static float getcpuUsage() {
        try {
            RandomAccessFile reader = new RandomAccessFile("/proc/stat", "r");
            String load = reader.readLine();
            String[] toks = load.split(" +");// 根据空格分割字符串
            long idle1 = Long.parseLong(toks[4]);
            long cpu1 =Long.parseLong(toks[1])+ Long.parseLong(toks[2])+ Long.parseLong(toks[3])
                    + Long.parseLong(toks[5])+ Long.parseLong(toks[6])+ Long.parseLong(toks[7]);
            try {
                Thread.sleep(360);
            } catch(Exception e){}
            reader.seek(0);
            load = reader.readLine();
            reader.close();
            toks = load.split(" +");
            long idle2 = Long.parseLong(toks[4]);
            long cpu2 = Long.parseLong(toks[1]) + Long.parseLong(toks[2]) + Long.parseLong(toks[3])
                    + Long.parseLong(toks[5]) + Long.parseLong(toks[6])+ Long.parseLong(toks[7]);
            return (float)(cpu2-cpu1)/((cpu2 +idle2)-(cpu1 + idle1));
        } catch(IOException e) {
            e.printStackTrace();
        }
        return 0;
    }

```

