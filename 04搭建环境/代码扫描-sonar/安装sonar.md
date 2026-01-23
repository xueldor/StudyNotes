Sonar是一个用于代码质量管理的开源平台，可以集成不同的测试工具，代码分析工具，以及持续集成工具，比如FindBugs 、PMD  、CheckStyle 、Jenkins。

Sonar本身不进行代码分析，需要安装插件。插件可以从应用市场安装，也可以自己开发插件。PMD  、CheckStyle 等均以插件的形式集成到sonar中。基于集成工具的结果数据按照一定的质量模型，如iso-9126，对软件的质量进行评估。



## Ubuntu上面的sonar配置

1. 安装和配置jdk, sonarqube7.9.6要求至少jdk版本11。

2. 安装和配置数据库，新建数据库"sonar"(数据库名称也可自己定义)。SonarQube 7.9版本不再支持MySQL。我使用默认自带的H2数据库，所以不需要额外安装配置。

3. 从sonarQube官网下载sonarQube压缩包，解压。

4. 我下载的sonarQube版本是7.9.6。修改配置文件"sonarqube-7.9.6/conf/sonar.properties"，其实就是配置数据库，使用我们刚才创建的数据库。以下属性需要配置：

   ```properties
   sonar.jdbc.username=xue //数据库用户名
   sonar.jdbc.password=123456 //数据库用户密码
   sonar.jdbc.url=jdbc:mysql://127.0.0.1:3306/sonar?useUnicode=true&characterEncoding=utf8&rewriteBatchedStatements=true&useConfigs=maxPerformance&useSSL=false //这是mysql的连接字符串，高版本已经不支持mysql，改成你刚安装的数据库
   sonar.jdbc.driverClassName:com.mysql.jdbc.Driver
   ```

   我使用自带的H2数据库，不需要上面这些配置，但是正式的商用环境应该使用更强大稳定的数据库。

5. 进入sonarqube-7.9.6/bin/linux-x86-64目录，执行`./sonar.sh start`。

   ```shell
   Starting SonarQube...
   Failed to start SonarQube.
   ```

   启动失败，进入log目录查看日志：

   ```shell
   --> Wrapper Started as Daemon
   Launching a JVM...
   Unable to start JVM: No such file or directory (2)
   JVM exited while loading the application.
   JVM Restarts disabled.  Shutting down.
   <-- Wrapper Stopped
   ```

   打开conf/wrapper.conf文件,开头wrapper.java.command=java，把后面的值改成你的系统的实际路径，比如：`/home/xue/android-studio/jre/bin/java`。这里我偷懒使用了android studio自带的jdk。注意不是到bin这一级，而是要指定到java可执行程序，以及使用绝对路径，不要用~代替home目录。

   接着先`./sonar.sh stop`再`./sonar.sh start`，启动OK：

   ```shell
   xue@xue-virtual-machine:~/sonarqube-7.9.6/bin/linux-x86-64$ ./sonar.sh start
   Starting SonarQube...
   Started SonarQube.
   
   ```

   还是有错误日志：

   “SonarQube requires Java 11 to run”。修改wrapper.java.command指向一个高版本的java即可。

   然后提示系统允许打开的文件数量过低，以及*max_map_count*太小。

   *max_map_count*修改比较容易，打开`/etc/sysctl.conf`,末尾添加一行vm.max_map_count=262144。

   前一个问题百度的答案全是没有用的。只有一个方法：

   修改/etc/security/limits.conf ，添加

   ```txt
   *              hard    nofile          65537
   *              soft    nofile          65535
   ```

   星号表示所有用户。接着`/etc/systemd/user.conf`和`/etc/systemd/system.conf`,两个文件都在末尾加一行DefaultLimitNOFILE=65537。然后OK。

   假如你使用的是MySQL ，你会发现再次重启，还是不行。sonar.log看不到有用信息，web.log显示： End of Life of MySQL Support : SonarQube 7.9 and future versions do not support MySQL.

   我用的是H2，没这些问题，启动后，打开浏览器，“http://localhost:9000/”，进入界面。下面有个提示：

   Embedded database should be used for evaluation purposes only

   The embedded database will not scale, it will not support upgrading to newer versions of SonarQube, and there is no support for migrating your data out of it into a different database engine.

   提醒我H2数据库功能比较弱，也不够稳定，应仅用于实验。

6. 管理员默认账号和密码为admin/admin

7. 界面语言是英文。显示中文需要安装插件，Administration-> Marketplace，插件搜索框输入chinese搜索 点击插件右侧install 开始安装。然后重启sonar即可。

## 几种代码检测工具

| 工具       | 目的                                                         | 检查项                                                       | 特点                                                         |
| ---------- | ------------------------------------------------------------ | ------------------------------------------------------------ | ------------------------------------------------------------ |
| FindBugs   | 检查.class 基于Bug Patterns概念，查找javabytecode（.class文件）中的潜在bug | 主要检查bytecode中的bug patterns，如NullPoint空指针检查、没有合理关闭资源、字符串相同判断错（==，而不是equals）等 | FindBugs 大多数提示有用,值得改 配置无查找功能，不过缩写能让我们很快找到某个规则 提供图形界面的独立程序，对jar进行检测，有报告生成，非常方便 很多功能插件没有实现，可独立使用FindBugs，但没法同时修改源码 |
| PMD        | 检查源文件 检查Java源文件中的潜在问题                        | 主要包括： 空try/catch/finally/switch语句块 未使用的局部变量、参数和private方法 空if/while语句 过于复杂的表达式，如不必要的if语句等 复杂类 | 比较严格 独立的程序是命令行形式操作 插件可以配置规则，有独立显示问题的视图，也很方便 一般来说，需要自定义规则才通过检验 |
| CheckStyle | 检查源文件 主要关注格式 检查Java源文件是否与代码规范相符     | 主要包括： Javadoc注释 命名规范 多余没用的Imports Size度量，如过长的方法 缺少必要的空格Whitespace 重复代码 | 过于严格 按照Sun的规范太严格了，需要自定义规则 插件自定义规则没有查找功能，查找规则麻烦 只能做检查，不能修改代码，可配合Jalopy使用修改代码 |



要编写规则配置文件 必须先了解规则。

checkstyle

[详解CheckStyle的检查规则（共138条规则）](http://blog.csdn.net/zzq900503/article/details/42007193)

[checkstyle官网:http://checkstyle.sourceforge.net/](http://checkstyle.sourceforge.net/)

[基于华为java编程规范的checkstyle.xml以及格式化模版,注释模版](http://blog.csdn.net/zzq900503/article/details/49617939)



findbugs

[Findbug官网地址：http://findbugs.sourceforge.net/](http://findbugs.sourceforge.net/)
[FindBugs的详细bug描述清单见：http://findbugs.sourceforge.net/bugDescriptions.html](http://findbugs.sourceforge.net/bugDescriptions.html)

