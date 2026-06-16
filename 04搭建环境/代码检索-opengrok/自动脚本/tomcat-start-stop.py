
startTomcat="/usr/local/tomcat/bin/startup.sh"
stopTomcat="/usr/local/tomcat/bin/shutdown.sh"


def eachProject(func):
    projects = content["projects"]
    for proj in projects :
        func(proj)

#遍历命令行参数指定了的proj
def eachTargetProject(func, projsFilter):
    if len(projsFilter) == 0:
        eachProject(func)
    else :
        projects = content["projects"]
        for proj in projects :
            if proj["name"] in projsFilter:
                func(proj)


def printProjInfo(proj) :
    print(proj["name"], proj["description"])

def deployProj(proj) :
    if not proj:
        print("NULL")
        return
    print("deploy: " + proj["branch"])
    name,branch=proj["name"],proj["branch"]
    print(name, branch)
    tomcatWebXmlFile = deployPath+ "/" + branch + "/WEB-INF/web.xml"
    if os.path.exists(tomcatWebXmlFile):
        print("OK")
    else:
        # 解压source.war,来自opengrok/lib/
        runCommand("unzip " + deployPath + "/aospxref.war -d " + deployPath+ "/" + branch)
    # 修改web.xml CONFIGURATION的值
    changed = False
    with open(tomcatWebXmlFile, 'r') as file:
        lines = file.readlines()
        for index, li in enumerate(lines):
            pos = li.find("<param-value>/var/opengrok/etc/configuration.xml</param-value>")
            if pos != -1:
                tmp = "<param-value>" + opengrok_workspace + "/" + dataDir + "/" + branch + "/configuration.xml</param-value>"
                lines[index] = li.replace("<param-value>/var/opengrok/etc/configuration.xml</param-value>", tmp)
                changed = True
                print(lines[index])
    if changed:
        with open(tomcatWebXmlFile, 'w') as file:
            file.writelines(lines)

def syncProj(proj) :
    branch,repo_init,repo_sync,description=proj["branch"],proj["repo_init"],proj["repo_sync"],proj["description"]
    codeRootDir = opengrok_workspace + "/" + sourceDir + "/" + branch
    os.makedirs(codeRootDir, exist_ok=True)
    runCommandInDir(repo_init, codeRootDir)
    runCommandInDir(repo_sync, codeRootDir)
    global htmlInfo2
    htmlInfo2 += '<li><b>' + datetime.now().strftime('%Y-%m-%d') + '</b> - Update ' + description + ' - ' + branch + '</li>'

def generateIndex(proj) :
    branch=proj["branch"]
    cared_dir, description=proj["cared_dir"],proj["description"]
    theindexPath = opengrok_workspace + '/' + dataDir + "/" + branch
    print("rm: " + theindexPath)
    if os.path.exists(theindexPath):
        # 删除目录及其所有内容
        shutil.rmtree(theindexPath)

    command="java -Xmx2096m "
    # 拼参数
    command += ' -jar "' + opengrokjar + '"' # 路径外面加个双引号，防止路径里有空格
    command += ' -W "' + opengrok_workspace + '/' + dataDir + '/' + branch + '/configuration.xml"'
    command += ' -c "/usr/bin/ctags"'
    command += ' -P -S -v'
    command += ' -s "' + opengrok_workspace + '/' + sourceDir + "/" + branch + '/' + cared_dir + '"'
    command += ' -d "' + theindexPath + '"'
    command += ' ' + commandExpand
    print(command)
    runCommand(command)

    
    
def updateHtmlFile(proj) :
    branch=proj["branch"]
    description=proj["description"]
    current_date = datetime.now()
    date_string = current_date.strftime('%Y-%m-%d')
    global htmlInfo1,htmlInfo2
    htmlInfo1 += '<li><a href="/' + branch + '/">' + branch + '</a></li>'
    with open(indexHtmlFromPath, 'r', encoding='utf-8') as file:
        lines = file.read().format(htmlInfo1, htmlInfo2,htmlInfo3, htmlInfo4)
        # print(lines)
    with open(indexHtmlToPath, 'w') as file:
        file.writelines(lines)


if __name__ == '__main__':
   targetProj = sys.argv[1:]
   print("传入的参数：",targetProj)
   check_java()
   runShell(stopTomcat) # 先停止tomcat服务器
   with open(configFile, 'r') as file:
    try:
        content = json.load(file)
        eachProject(deployProj) # 部署source.war
        # 同步代码
        eachTargetProject(syncProj, targetProj)

        #生成opengrok 索引
        ## 先拼接一部分命令行
        newline=0
        commandExpand=""
        globalConfig=content["globalConfig"]
        if 'includeFile' in globalConfig :
            includeFile = globalConfig['includeFile']
            for ii in includeFile :
                commandExpand += ' --include "' + ii + '"'
                htmlInfo3 += ii + ','
                if newline > 10: 
                    htmlInfo3 += '<br/>'
                    newline=0
        newline=0
        if 'ignoreFile' in globalConfig :
            ignoreFile = globalConfig['ignoreFile']
            for ig in ignoreFile :
                commandExpand += ' --ignore "' + ig + '"'
                htmlInfo4 += ig + ','
                if newline > 8: 
                    htmlInfo3 += '<br/>'
                    newline=0

        eachTargetProject(generateIndex, targetProj)

        eachProject(updateHtmlFile)
        print(htmlInfo1,htmlInfo2)

    except json.JSONDecodeError as e:
        print("JSON 格式错误:", e)
    
    # 重新启动tomcat服务器
    runShell(startTomcat)
