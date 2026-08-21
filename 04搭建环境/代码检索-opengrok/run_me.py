#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import sys
import shutil
import subprocess
import threading
import json
from datetime import datetime

# 配置参数
opengrok_workspace = "/workspace/xuexiangyu/opengrok_workspace"
sourceDir = "source"
dataDir = "data"

opengrokjar = "/workspace/xuexiangyu/softs/opengrok-1.14.15/lib/opengrok.jar"
sourcewar = "/workspace/xuexiangyu/softs/opengrok-1.14.15/lib/source.war"
tomcatHome = "/opt/apache-tomcat-10.1.57"
deployPath = tomcatHome + "/webapps"
startTomcat = tomcatHome + "/bin/startup.sh"
stopTomcat = tomcatHome + "/bin/shutdown.sh"

configFile = "config.json"
indexHtmlFromPath = "index.html"
indexHtmlToPath = deployPath + "/ROOT/index.html"
htmlInfoFile = ".html_info.json"  # HTML信息临时文件

# HTML信息变量
html_project_links = ""      # 项目链接列表
html_update_info = ""       # 更新时间信息
html_include_files = "all"     # 包含文件列表
html_ignore_files = ""      # 忽略文件列表

# 认证Token配置
auth_token = "123456"        # 默认认证token

def save_html_info():
    """保存HTML信息到临时文件"""
    html_data = {
        "html_project_links": html_project_links,
        "html_update_info": html_update_info,
        "html_include_files": html_include_files,
        "html_ignore_files": html_ignore_files
    }
    with open(htmlInfoFile, 'w', encoding='utf-8') as file:
        json.dump(html_data, file, ensure_ascii=False, indent=2)
    print(f"HTML info saved to {htmlInfoFile}")

def add_auth_token_to_config(config_xml_path):
    """在configuration.xml的</object>之前添加认证token配置"""
    global auth_token
    
    if not os.path.exists(config_xml_path):
        print(f"Warning: config file not found: {config_xml_path}")
        return
    
    with open(config_xml_path, 'r') as file:
        content = file.read()
    
    token_config = f"""  <void property="allowInsecureTokens">
    <boolean>true</boolean>
  </void>
  <void property="authenticationTokens">
      <void method="add">
        <string>{auth_token}</string>
      </void>
  </void>
  <void property="indexerAuthenticationToken">
   <string>{auth_token}</string>
  </void>
"""
    
    content = content.replace(' </object>\n</java>', token_config + ' </object>\n</java>')
    
    with open(config_xml_path, 'w') as file:
        file.write(content)
    
    print(f"Added auth token to {config_xml_path}")

def load_html_info():
    """从临时文件加载HTML信息"""
    global html_project_links, html_update_info, html_include_files, html_ignore_files
    
    if not os.path.exists(htmlInfoFile):
        print(f"HTML info file not found: {htmlInfoFile}, using empty values")
        return
    
    try:
        with open(htmlInfoFile, 'r', encoding='utf-8') as file:
            html_data = json.load(file)
            html_project_links = html_data.get("html_project_links", "")
            html_update_info = html_data.get("html_update_info", "")
            html_include_files = html_data.get("html_include_files", "")
            html_ignore_files = html_data.get("html_ignore_files", "")
        print(f"HTML info loaded from {htmlInfoFile}")
    except Exception as e:
        print(f"Error loading HTML info: {e}")

def check_java():
    """检查Java是否安装"""
    try:
        java_path = subprocess.check_output(['which', 'java']).decode().strip()
        if java_path and os.path.isfile(java_path):
            print(f"Java is located at: {java_path}")
            return True
        else:
            return False
    except subprocess.CalledProcessError:
        print("Java is not found in the system PATH.")
        return False
    except Exception as e:
        print(f"An error occurred: {e}")
    return False

def runShell(script_path):
    """运行shell脚本"""
    try:
        result = subprocess.run(['bash', script_path], check=True, text=True, capture_output=True)
        if result.returncode == 0:
            print("Output:", result.stdout)
            return True
        else:
            print("ErrorMsg:", result.stderr)
            return False
    except subprocess.CalledProcessError as e:
        print("Error:", e.stderr)
        return False

def runCommand(commandStr):
    """运行命令"""
    try:
        result = subprocess.run(commandStr, shell=True, text=True, capture_output=True)
        if result.returncode == 0:
            print("Output:", result.stdout)
            return True
        else:
            print("ErrorMsg:", result.stderr)
            return False
    except subprocess.CalledProcessError as e:
        print("Error:", e.stderr)
        return False

def read_stream(stream):
    """读取流输出"""
    for line in iter(stream.readline, b''):
        if line:
            print(line, end='', flush=True)
        else:
            return

def runCommandInDir(commandStr, directory_path):
    """在指定目录运行命令"""
    print("run command:", commandStr, "at", directory_path)
    try:
        process = subprocess.Popen(commandStr, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, cwd=directory_path)
        stdout_thread = threading.Thread(target=read_stream, args=(process.stdout,))
        stderr_thread = threading.Thread(target=read_stream, args=(process.stderr,))
        stdout_thread.start()
        stderr_thread.start()
        process.wait()

        stdout_thread.join()
        stderr_thread.join()
    except Exception as e:
        print(f"发生错误: {e}")
    finally:
        if process.stdout:
            process.stdout.close()
        if process.stderr:
            process.stderr.close()

def normalize_path(path):
    """规范化路径：去掉./开头，使用os.path.normpath"""
    # 去掉开头的./
    if path.startswith('./'):
        path = path[2:]
    # 使用os.path.normpath规范化
    return os.path.normpath(path)

def get_link_name(cared_dir_path):
    """从cared_dir路径获取链接名称（最后一级目录名）"""
    normalized = normalize_path(cared_dir_path)
    # 获取最后一级目录名
    link_name = os.path.basename(normalized)
    return link_name

def create_symlinks():
    """根据config.json创建source目录和软链接"""
    global html_project_links, html_update_info
    
    with open(configFile, 'r') as file:
        content = json.load(file)
    
    projects = content.get("projects", [])
    
    for proj in projects:
        name = proj.get("name")
        root_dir = proj.get("root_dir")
        cared_dirs = proj.get("cared_dir", [])
        
        if not name:
            continue
        
        # 创建/清空source/<name>目录
        source_proj_dir = os.path.join(opengrok_workspace, sourceDir, name)
        if os.path.exists(source_proj_dir):
            # 删除目录下所有内容
            for entry in os.listdir(source_proj_dir):
                entry_path = os.path.join(source_proj_dir, entry)
                if os.path.islink(entry_path) or os.path.isfile(entry_path):
                    os.remove(entry_path)
                elif os.path.isdir(entry_path):
                    shutil.rmtree(entry_path)
            print(f"Cleaned directory: {source_proj_dir}")
        else:
            os.makedirs(source_proj_dir, exist_ok=True)
            print(f"Created directory: {source_proj_dir}")
        
        # 根据cared_dir创建软链接
        for cared_dir in cared_dirs:
            # 规范化路径
            normalized_cared_dir = normalize_path(cared_dir)
            # 获取链接名称
            link_name = get_link_name(normalized_cared_dir)
            # 目标路径：root_dir + cared_dir
            target_path = os.path.join(root_dir, normalized_cared_dir)
            target_path = os.path.normpath(target_path)
            # 链接路径：source/<name>/link_name
            link_path = os.path.join(source_proj_dir, link_name)
            
            # 确保目标路径存在
            if not os.path.exists(target_path):
                print(f"Warning: target path does not exist: {target_path}")
                continue
            
            # 创建软链接
            os.symlink(target_path, link_path)
            print(f"Created symlink: {link_path} -> {target_path}")
    
    print("Symlinks creation completed.")

def sync_projects():
    """同步包含repo_init的项目 - 在root_dir目录下下载代码"""
    global html_update_info
    
    with open(configFile, 'r') as file:
        content = json.load(file)
    
    projects = content.get("projects", [])
    
    for proj in projects:
        name = proj.get("name")
        repo_init = proj.get("repo_init")
        repo_sync = proj.get("repo_sync")
        root_dir = proj.get("root_dir")
        description = proj.get("description")
        cared_dirs = proj.get("cared_dir", [])
        
        # 只同步包含repo_init的项目
        if not repo_init:
            print(f"Skipping {name}: no repo_init configured")
            continue
        
        # 检查root_dir是否配置
        if not root_dir:
            print(f"Skipping {name}: no root_dir configured")
            continue
        
        # 创建root_dir目录
        os.makedirs(root_dir, exist_ok=True)
        
        print(f"Syncing project: {name}")
        print(f"Downloading to: {root_dir}")
        
        # 执行repo_init - 在root_dir目录下执行
        if repo_init:
            print(f"Running repo_init: {repo_init}")
            runCommandInDir(repo_init, root_dir)
        
        # 执行repo_sync - 在root_dir目录下执行
        if repo_sync:
            print(f"Running repo_sync: {repo_sync}")
            runCommandInDir(repo_sync, root_dir)
        
        # 创建source/<name>目录
        source_proj_dir = os.path.join(opengrok_workspace, sourceDir, name)
        os.makedirs(source_proj_dir, exist_ok=True)
        
        # 根据cared_dir创建软链接
        for cared_dir in cared_dirs:
            # 规范化路径
            normalized_cared_dir = normalize_path(cared_dir)
            # 获取链接名称
            link_name = get_link_name(normalized_cared_dir)
            # 目标路径：root_dir + cared_dir
            target_path = os.path.join(root_dir, normalized_cared_dir)
            target_path = os.path.normpath(target_path)
            # 链接路径：source/<name>/link_name
            link_path = os.path.join(source_proj_dir, link_name)
            
            # 确保目标路径存在
            if not os.path.exists(target_path):
                print(f"Warning: target path does not exist: {target_path}")
                continue
            
            # 如果链接已存在，先删除
            if os.path.exists(link_path) or os.path.islink(link_path):
                os.remove(link_path)
            
            # 创建软链接
            os.symlink(target_path, link_path)
            print(f"Created symlink: {link_path} -> {target_path}")
        
        # 更新HTML信息（同步时间）
        html_update_info += '<li><b>' + datetime.now().strftime('%Y-%m-%d') + '</b> - Update ' + description + ' - ' + name + '</li>'
    
    # 保存HTML信息到文件
    save_html_info()
    print("Project sync completed.")

def generate_index():
    """生成opengrok索引"""
    global html_project_links, html_update_info, html_include_files, html_ignore_files
    
    with open(configFile, 'r') as file:
        content = json.load(file)
    
    projects = content.get("projects", [])
    
    # 构建命令扩展参数
    commandExpand = ""
    newline = 0
    
    for proj in projects:
        name = proj.get("name")
        description = proj.get("description")
        cared_dirs = proj.get("cared_dir", [])
        proj_ignore_files = proj.get("ignoreFile", [])
        proj_include_files = proj.get("ignoreFile", [])

        # 构建项目特有的ignore参数
        html_ignore_files += "<li><b>" + name + '</b>: <br/>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;'
        proj_command_expand = commandExpand
        for ig in proj_ignore_files:
            proj_command_expand += ' --ignore "' + ig + '"'
            html_ignore_files += ig + ','
            newline += 1
            if newline > 8:
                html_ignore_files += '<br/>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;'
                newline = 0
        html_ignore_files += '</li>'

        # 索引路径
        theindexPath = os.path.join(opengrok_workspace, dataDir, name)
        print("rm: " + theindexPath)
        if os.path.exists(theindexPath):
            shutil.rmtree(theindexPath)
        
        # 构建命令
        command = "java -Xmx2096m "
        command += ' -jar "' + opengrokjar + '"'
        command += ' -W "' + os.path.join(opengrok_workspace, dataDir, name, 'configuration.xml') + '"'
        command += ' -c "/usr/bin/ctags"'
        command += ' -P -S -v'
        command += ' -s "' + os.path.join(opengrok_workspace, sourceDir, name) + '"'
        command += ' -d "' + theindexPath + '"'
        command += ' ' + proj_command_expand
        
        print(command)
        runCommand(command)
        
        config_xml_path = os.path.join(opengrok_workspace, dataDir, name, 'configuration.xml')
        add_auth_token_to_config(config_xml_path)
        
        # 更新HTML信息（项目链接）
        html_project_links += '<li><a href="/' + name + '/">' + name + '</a></li>'
    
    # 保存HTML信息到文件
    save_html_info()
    print("Index generation completed.")

def deploy_projects():
    """部署项目到Tomcat并更新HTML界面"""
    global html_project_links, html_update_info, html_include_files, html_ignore_files
    
    # 加载HTML信息
    load_html_info()
    
    with open(configFile, 'r') as file:
        content = json.load(file)
    
    projects = content.get("projects", [])
    
    # 根据.repo_fetchtimes.json文件修改时间更新html_update_info
    html_update_info = ""
    for proj in projects:
        name = proj.get("name")
        root_dir = proj.get("root_dir")
        description = proj.get("description", name)
        
        if not name:
            continue
        
        # 检查.repo_fetchtimes.json文件
        repo_fetchtimes_file = os.path.join(root_dir, ".repo", ".repo_fetchtimes.json")
        if os.path.exists(repo_fetchtimes_file):
            # 获取文件修改时间
            try:
                mtime = os.path.getmtime(repo_fetchtimes_file)
                update_time = datetime.fromtimestamp(mtime).strftime('%Y-%m-%d')
                html_update_info += '<li><b>' + update_time + '</b> - Update ' + description + ' - ' + name + '</li>'
                print(f"Found .repo_fetchtimes.json for {name}, modified at {update_time}")
            except Exception as e:
                print(f"Error getting modification time for {repo_fetchtimes_file}: {e}")
                # 如果获取时间失败，保持原有值
                pass
    
    for proj in projects:
        name = proj.get("name")
        
        if not name:
            continue
        
        tomcatWebXmlFile = os.path.join(deployPath, name, "WEB-INF", "web.xml")
        
        if os.path.exists(tomcatWebXmlFile):
            print(f"WEB-INF/web.xml exists for {name}")
        else:
            # 解压war包
            runCommand("unzip " + sourcewar + " -d " + os.path.join(deployPath, name))
        
        # 修改web.xml CONFIGURATION的值
        changed = False
        with open(tomcatWebXmlFile, 'r') as file:
            lines = file.readlines()
            for index, li in enumerate(lines):
                pos = li.find("<param-value>/var/opengrok/etc/configuration.xml</param-value>")
                if pos != -1:
                    tmp = "<param-value>" + os.path.join(opengrok_workspace, dataDir, name, "configuration.xml") + "</param-value>"
                    lines[index] = li.replace("<param-value>/var/opengrok/etc/configuration.xml</param-value>", tmp)
                    changed = True
                    print(lines[index])
        
        if changed:
            with open(tomcatWebXmlFile, 'w') as file:
                file.writelines(lines)
    
    print("Deployment completed.")
    
    # 更新HTML界面
    with open(indexHtmlFromPath, 'r', encoding='utf-8') as file:
        content = file.read()
    
    # 使用format替换占位符
    updated_content = content.format(html_project_links, html_update_info, html_include_files, html_ignore_files)
    
    with open(indexHtmlToPath, 'w', encoding='utf-8') as file:
        file.write(updated_content)
    
    print("HTML file updated.")

def restart_tomcat():
    """重启Tomcat服务器"""
    print("Stopping Tomcat...")
    runShell(stopTomcat)
    
    print("Starting Tomcat...")
    runShell(startTomcat)
    
    print("Tomcat restart completed.")

def print_usage():
    """打印使用说明"""
    print("""Usage: python run_new.py <command> [project_name]
    
Commands:
  init        创建source目录结构和软链接
  sync        同步代码（只同步包含repo_init的项目）
  index       生成索引并更新HTML界面
  deploy      部署项目到Tomcat
  restart     重启Tomcat服务器
  all         执行全部操作（init -> sync -> index -> deploy -> restart）
  
Options:
  [project_name]  可选，指定要操作的项目名称（仅对sync和index命令有效）
""")

def main():
    if len(sys.argv) < 2:
        print_usage()
        sys.exit(1)
    
    command = sys.argv[1]
    target_project = sys.argv[2] if len(sys.argv) > 2 else None
    
    # 检查Java
    check_java()
    
    if command == "init":
        create_symlinks()
    elif command == "sync":
        print("=== 因为有些项目依赖低版本python和repo工具,需要在docker容器里sync代码,所以屏蔽此命令选项，请手动同步 ===")
        # sync_projects()
    elif command == "index":
        generate_index()
    elif command == "deploy":
        deploy_projects()
    elif command == "restart":
        restart_tomcat()
    elif command == "all":
        print("=== Creating symlinks ===")
        create_symlinks()
        
        print("\n=== Syncing projects ===")
        print("=== 因为有些项目依赖低版本python和repo工具,需要在docker容器里sync代码,所以屏蔽此命令选项，请手动同步 ===")
        # sync_projects()
        
        print("\n=== Generating index ===")
        generate_index()
        
        print("\n=== Deploying projects ===")
        deploy_projects()
        
        print("\n=== Restarting Tomcat ===")
        restart_tomcat()
        
        print("\n=== All operations completed ===")
    else:
        print(f"Unknown command: {command}")
        print_usage()
        sys.exit(1)

if __name__ == '__main__':
    main()

