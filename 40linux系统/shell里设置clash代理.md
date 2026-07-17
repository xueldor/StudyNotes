首先，Clash Verge设置里打开局域网连接，clash才能接受非本机的连接。然后，下面脚本IP改成实际IP。即安装了clash的机器的IP。

// set-proxy.sh

```shell
#!/bin/bash

# 代理服务器配置
PROXY_HOST="172.17.192.1"
PROXY_PORT="7897"

# 设置代理环境变量
export http_proxy="http://${PROXY_HOST}:${PROXY_PORT}"
export https_proxy="http://${PROXY_HOST}:${PROXY_PORT}"
export all_proxy="socks5://${PROXY_HOST}:${PROXY_PORT}"
export ftp_proxy="http://${PROXY_HOST}:${PROXY_PORT}"
export socks_proxy="socks5://${PROXY_HOST}:${PROXY_PORT}"

# 同时设置大写版本（部分应用使用大写）
export HTTP_PROXY="http://${PROXY_HOST}:${PROXY_PORT}"
export HTTPS_PROXY="http://${PROXY_HOST}:${PROXY_PORT}"
export ALL_PROXY="socks5://${PROXY_HOST}:${PROXY_PORT}"
export FTP_PROXY="http://${PROXY_HOST}:${PROXY_PORT}"
export SOCKS_PROXY="socks5://${PROXY_HOST}:${PROXY_PORT}"

echo " 代理已设置："
echo "   http_proxy  = $http_proxy"
echo "   https_proxy = $https_proxy"
echo "   all_proxy   = $all_proxy"

echo " 验证方法：curl -I https://www.google.com"
```

source set-proxy.sh即可,当前shell有效。

撤销

```shell
cat unset-proxy.sh
#!/bin/bash

unset http_proxy
unset https_proxy
unset all_proxy
unset ftp_proxy
unset socks_proxy
unset HTTP_PROXY
unset HTTPS_PROXY
unset ALL_PROXY
unset FTP_PROXY
unset SOCKS_PROXY
```

