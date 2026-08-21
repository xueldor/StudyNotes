## 需求：

搭建了一个opengrok。网页http://192.168.20.111:8080/搜索项目代码。

然后找了一个MCP：https://github.com/BarryHsia/opengrok-aosp-mcp , 能调用 opengrok api。

## 本地部署opengrok-aosp-mcp 

````
git clone https://github.com/BarryHsia/opengrok-aosp-mcp.git opengrok-aosp-mcp
cd opengrok-aosp-mcp
./install.sh
````

编辑 config.json：

```
opengrok-aosp-mcp# cat config.json 
{
  "opengrok": {
    "base_url": "http://127.0.0.1:8080:8080/24MM_T2_dev/",
    "token": "123456",
    "allowInsecure": true
  },
  "cache": {
    "enabled": true,
    "ttl_hours": 24,
    "directory": ".cache"
  },
  "limits": {
    "default_results": 10,
    "max_results": 50,
    "max_snippet_lines": 5
  },
  "token_optimization": {
    "abbreviate_paths": true,
    "path_prefixes": {
      "frameworks/base": "f/b",
      "frameworks/native": "f/n",
      "system/core": "s/c",
      "hardware/interfaces": "h/i",
      "packages/apps": "p/a"
    }
  }
}
```

可参考同目录下config.example.json。

### 修复一个bug

这个MCP在调用opengrok拼接参数时,有个小bug, 当project是空时，url会拼一个projects=。比如：

`http://localhost:8080/24MM_T2_dev/api/v1/search?full=ActivityManager&projects=&maxresults=10&start=0&path=&type=`

服务端会认为我传入一个project 名称为 "" 的工程，然后搜索结果为空。正确格式是，当没有project时应该去掉它，并增加一个searchall=true参数。正确格式如下：

```
http://192.168.20.111:8080/24MM_T2_dev/api/v1/search?full=ActivityManager&maxresults=10&start=0&path=&type=&searchall=true
```

代码修改：

```
diff --git a/core/opengrok_client.py b/core/opengrok_client.py
index f434b27..358f203 100644
--- a/core/opengrok_client.py
+++ b/core/opengrok_client.py
@@ -51,12 +51,15 @@ class OpenGrokClient:
         """
         params = {
             search_type: query,
-            "projects": project,
             "maxresults": max_results,
             "start": start,
             "path": path or "",
             "type": file_type or "",
         }
+        if project:
+            params["projects"] = project
+        else:
+            params["searchall"] = "true"
         
         with httpx.Client(timeout=self.timeout) as client:
             resp = client.get(
```

### 启动

```
uv run .venv/bin/python3 server.py
uv --directory /path/to/opengrok-aosp-mcp run .venv/bin/python3 server.py
```

## 添加到openwebui

openwebui不支持直接调用uv run(也就是不支持stdio类型),但openwebui官方提供了一个工具mcpo，可以把转成遵循 MCP (模型上下文协议) 的工具，自动转换成标准的 OpenAPI 服务。

> OpenAPI 规范（OAS）是一种用于描述 RESTful API 的标准格式。它是由 SmartBear Software 捐赠给 OpenAPI Initiative，在2015年从 Swagger 规范重命名为 OpenAPI 规范。OpenAPI 规范使得人类和计算机都能在“不接触任何程序源代码和文档、不监控网络通信”的情况下理解一个服务的作用。如果您在定义您的 API 时做的很好，那么使用 API 的人就能非常轻松地理解您提供的 API 并与之交互了。
>
> 如果您遵循 OpenAPI 规范来定义您的 API，那么您就可以用文档生成工具来展示您的 API，用代码生成工具来自动生成各种编程语言的服务器端和客户端的代码，用自动测试工具进行测试等等。

根据https://github.com/open-webui/mcpo 官网文档，mcpo支持三种客户端接入方式：

* stdio：本地子进程管道（默认）。示例：

  ```
  mcpo --port 8000 --api-key "top-secret" -- your_mcp_server_command
  ```

* sse：远端 SSE MCP 客户端（主动连外部 SSE）。示例：

  ```
  mcpo --port 8000 --api-key "top-secret" --server-type "sse" -- http://127.0.0.1:8001/sse
  mcpo --port 8000 --api-key "top-secret" --server-type "sse" --header '{"Authorization": "Bearer token", "X-Custom-Header": "value"}' -- http://127.0.0.1:8001/sse
  ```

* streamable-http：远端流式 HTTP MCP 客户端

  ```
  mcpo --port 8000 --api-key "top-secret" --server-type "streamable-http" -- http://127.0.0.1:8002/mcp
  ```

我们这个是stdio type，故命令：

```
uvx mcpo --host 0.0.0.0 --port 9000 -- uv run .venv/bin/python3 server.py
```

然后openwebui里添加：

* 点 头像- 设置-扩展功能
* 管理工具服务器，点+号，添加连接
* 随便起个名字，URL填 http://localhost:9000 ，因为和 opengrok在同一个PC。点刷新图标验证一下连接。
* 认证方式无。启用功能。保存

聊天对话框-扩展功能，选中这个工具。一个扳手图标

验证一下：

```
//对话框
试一下list_project能不能调用，给我结果
// 观察输出。

搜索ActivityManager
```

## opengrok授权

上面调用失败，提示认证拒绝。是因为opengrok默认不允许外部调用它的api。

tomcat部署的项目，cat WEB-INF/web.xml ，会看到

```
    <context-param>
        <description>Full path to the configuration file where OpenGrok can read its configuration</description>
        <param-name>CONFIGURATION</param-name>
        <param-value>/workspace/xuexiangyu/opengrok_workspace/data/24MM_T2_dev/configuration.xml</param-value>
    </context-param>
```

这个configuration.xml，是opengrok索引生成的。打开它，添加：

```
//------添加
<void property="allowInsecureTokens">
    <boolean>true</boolean>
  </void>
  <void property="authenticationTokens">
      <void method="add">
        <string>123456</string>
      </void>
  </void>
  <void property="indexerAuthenticationToken">
   <string>123456</string>
  </void>
//------添加结束
 </object>
</java>

```

重启tomcat即可。这个配置对应前面MCP里的的：

```
opengrok-aosp-mcp# cat config.json 
{
  "opengrok": {
    "base_url": "http://127.0.0.1:8080:8080/24MM_T2_dev/",
    "token": "123456"
  },
  ....
```

验证opengrok权限是否已放开：

```
curl -H "Authorization: Bearer 123456" "http://192.168.20.111:8080/24MM_T2_dev/api/v1/projects"
//返回:
["system","frameworks","tz_8155","build","bionic","vendor","packages","bootable","hardware"]
```



## 问题

* 缓存导致奇怪问题。`rm -rf /home/opengrok-aosp-mcp/.cache/`清空。还有一个`/root/.cache/uv`目录，但要注意不要清空整个.cache,因为还有其它应用的缓存。比如首次启动openwebui,下载的大量缓存都在这里，清空的话启动又要等很久，而且要挂VPN下载。

  ```
  uv cache clean
  uv cache dir
  ```

* 存在socks代理的情况下，需`uv pip install httpx[socks] httpcore[socks]`

* 设置代理后，某些请求固定报502，可能是梯子的bug也有可能clash没配置好。建议尽量不要开代理。至少调用这个MCP的时候不要设置代理，如果openwebui需要，等启动openwebui时再设。

* 不加`--host 0.0.0.0`，则监听的是本地回环地址，外部机无法访问。

* 有时版本不适配需要指定版本`uvx --with "mcp==1.9.0"  xxxx`

## 添加到IDE

本地opengrok-aosp-mcp是stdio类型，openwebui支持OpenAPI ，而vscode插件(Cline、ROO Code等)以及其它一些主流IDE,基本不支持OpenAPI。它们支持的是stdio、sse、Streamable HTTP。

1、vscode和mcp server布署在一起，则可以以stdio方式

```
// 示例:
{
  "mcpServers": {
    "example-server": {
      "command": "npx",
      "args": [
        "-y",
        "mcp-server-example"
      ]
    }
  }
}

```

2、否则就需要转成sse、Streamable HTTP类型的server,就像为了给openwebui使用转成OpenAPI一样。用另外一个工具mcp-proxy。

```
uvx mcp-proxy --host 0.0.0.0 --port 9050 --transport streamablehttp -- uv run python3 server.py
uvx mcp-proxy --host 0.0.0.0 --port 9050 --transport sse -- uv run python3 server.py
```

streamablehttp比sse更新，这个命令--transport streamablehttp同时兼容了--transport sse,所以我们只用第一个即可。



## 快捷指令

```
//1. 让AI修改server.py, 支持传入mcp-name和config文件。

//2. mcpo配置多个mcpServers
root@ai:/home/opengrok-aosp-mcp# cat mcpo.config.json
{
  "mcpServers": {
    "24MM_T2": {
      "command": "uv",
      "args": ["run", ".venv/bin/python3", "server.py", "--mcp-name=opengrok_24mm_t2", "--config=config_24mm_t2.json" ]
    },
    "p32s": {
      "command": "uv",
      "args": ["run", ".venv/bin/python3", "server.py", "--mcp-name=opengrok_p32s", "--config=config_p32s.json" ]
    }
  }
}
// 3.启动
uvx mcpo --host 0.0.0.0 --port 9010 --config mcpo.config.json


// 启动脚本
root@ai:/home# cat startMCP.sh 
cd /home/opengrok-aosp-mcp
# OpenAPI REST HTTP格式, 给openwebui用
nohup uvx --with "mcp==1.9.0" mcpo --host 0.0.0.0 --port 9000 --config mcpo.config.json > logs/mcpo_opengrok.log 2>&1 &
# streamablehttp兼容sse，给当前主流IDE使用
nohup uvx --with "mcp==1.9.0" mcp-proxy --host 0.0.0.0 --port 9050 --transport streamablehttp -- uv run python3 server.py --mcp-name=opengrok_24mm_t2 --config=config_24mm_t2.json > logs/mcp_24mm_t2.log 2>&1 &
nohup uvx --with "mcp==1.9.0" mcp-proxy --host 0.0.0.0 --port 9051 --transport streamablehttp -- uv run python3 server.py --mcp-name=opengrok_p32s --config=config_p32s.json > logs/mcp_p32s.log 2>&1 &


root@ai:/home# cat runMCPAndOpenWebUI.sh 
./startMCP.sh

cd /home/openwebui
source venv/bin/activate
export OFFLINE_MODE=true
export HF_HUB_OFFLINE=1
nohup open-webui serve --port 3000 > openwebui.log 2>&1 &


# open-webui里添加的URL:
http://192.168.20.111:9000/24MM_T2
http://192.168.20.111:9000/p32s
```

