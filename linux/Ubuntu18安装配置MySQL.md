

```shell
sudo apt update
sudo apt install mysql-server
sudo apt install emma #一个MySQL的可视化工具
```

打开emma，新建连接，hostname是localhost；port是3306；用户和密码就写你的ubuntu登录账号。

然后连接，这时肯定会提示你权限问题。我们用命令行进入mysql命令：

```shell
> mysql
mysql> grant all privileges on *.* to xue@localhost identified by '123456' with grant option;
Query OK, 0 rows affected, 1 warning (0.00 sec)
mysql> flush privileges;
Query OK, 0 rows affected (0.00 sec)
```

用户相关信息是在mysql这个数据库的user表中：

```shell
mysql> use mysql#选择使用mysql数据库
Reading table information for completion of table and column names
You can turn off this feature to get a quicker startup with -A

Database changed
mysql> 
mysql> select User from user;
+------------------+
| User             |
+------------------+
| debian-sys-maint |
| mysql.session    |
| mysql.sys        |
| root             |
| xue              |
+------------------+
5 rows in set (0.00 sec)

mysql> 
```

看到表里有xue用户。

然后emma可以连接成功了。