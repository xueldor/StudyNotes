**需求**：AndroidStudio项目中引用framework.jar

* 如果是我们自己在framework里新增的class，需要在代码里使用，只需要添加：

```
dependencies {
    compileOnly files('../sdk/framework.jar')
}
```

用compileOnly方式，避免打包到apk里面。无论哪个版本都有效。



* 如果是安卓原来就有的类，需调用里面的hide方法(或自己的新增函数)，仅仅上面的配置不够，因为默认会优先引用android.jar里面的。

  那么还需以下配置：

  -----------------------------更新：2025.12 AS升级到2025----------------

  所有方法都彻底失效，生成iml的开关也没了。

  可以找到你需要的那个类，从framework.jar里提出来，压缩到android.jar里。会触发转化android-mockable-jar失败。可以先构建，然后修改android.jar，之后尽量不要clean和修改gradle。没好办法，将就将就。
  
  
  
  -----------------------------更新：2025.10 针对新版AS，gradle 8.11----------------
  
  1. preBuild里的代码依然适用，做一些优化调整如下：
  
     ```
     preBuild {
         doLast {
             def imlFile = file(project.name + ".iml")
             if(!imlFile.exists()){
                 imlFile = file("../.idea/modules/app/" + project.parent.name + "." + project.name + ".main.iml")
             }
             if(imlFile.exists()) {
                 println 'Change ' + imlFile.name + ' order'
                 try {
                     def parsedXml = (new XmlParser()).parse(imlFile)
                     def jdkNode = parsedXml.component[1].orderEntry.find { it.'@type' == 'jdk' }
                     parsedXml.component[1].remove(jdkNode)
                     parsedXml.component[1].append(jdkNode)
                     groovy.xml.XmlUtil.serialize(parsedXml, new FileOutputStream(imlFile))
                 } catch (FileNotFoundException e) { // nop, iml not found
                 }
             }else {
                 println imlFile.name + " not found"
             }
         }
     }
     ```

     另外，记得到设置-Build,Execution,Deployment-Build Tools-Gradle里勾选："Generate *.iml files for modules imported from gradle"。不太稳定，有时需要重启AS才能生成iml。

     （或许以后AS会完全不依赖.idea/*.iml文件，到时这些方法都会失效）
  
  2. 基于新版AS，gradle8.11，options.compilerArgs和options.bootstrapClasspath都没用。这步没找到正确方法。
  
     所以目前只能配置，让AS不报红，gradle build还是不会通过。
  
  3. 假如第二步有用，原来的代码依然要做一些调整。
  
     ```
     1. 外面的build.gradle里已经没有allprojects{}，去掉
     2. 因为alias(libs.plugins.android.application) apply false
     false表示不立即应用，在子模块中应用，所以，对于tasks.withType(JavaCompile){}语句，如果放在外面的build.gradle里,因为JavaCompile是由android.application插件提供的，所以闭包里的语句不会执行（因为没有JavaCompile这个Task）。
     因此要移到app/build.gradle里
     3. 同时，语句语法有一点变动。
     tasks.withType(JavaCompile).configureEach { task ->
         // 1. 安全获取framework.jar路径（基于app模块的相对路径）
         def frameworkJar = project.findProject(':app')?.file('sdk/framework.jar')
         if (!frameworkJar?.exists()) {
             logger.warn("未找到 framework.jar，路径：${frameworkJar?.absolutePath ?: '未知路径'}")
             return
         }
     
         // 2. 合并原有启动类路径与framework.jar
         def newBootstrapClasspath = [frameworkJar]
         if (task.options.bootstrapClasspath != null) {
             newBootstrapClasspath.addAll(task.options.bootstrapClasspath.files)
         }
     
         // 3. 设置最终的启动类路径
         task.options.bootstrapClasspath = files(newBootstrapClasspath)
     
         // 打印调试信息
         logger.lifecycle("已为任务 ${task.name} 配置启动类路径，包含 framework.jar")
     }
     ```
  
  4. gradle.properties配置，不一定有帮助，可以试一下
     
        ```properties
        android.buildFeatures.precompileDependencies=false
        android.debug.obsoleteApi=true
        ```

---------------------old----------------------------

会优先引用Android SDK里的类，这样那些hide的函数就引用不到了。编译报错。

在旧版本的AndroidStudio中，在外面的build.gradle里添加：

```groovy
    allprojects {
        gradle.projectsEvaluated {
            tasks.withType(JavaCompile) {
                //../framework.jar 为相对位置，需要参照着修改
                options.compilerArgs.add('-Xbootclasspath/p:sdk/framework.jar')
            }
        }
    }
```

新版本已失效，应改为：

```groovy
gradle.projectsEvaluated {
    tasks.withType(JavaCompile) {
        Set<File> fileSet = options.bootstrapClasspath.getFiles()
        List<File> newFileList =  new ArrayList<>();
        //"../framework.jar" 为相对位置，需要参照着修改
        newFileList.add(new File("sdk/framework.jar"))
        newFileList.addAll(fileSet)
        options.bootstrapClasspath = files(
                newFileList.toArray()
        )
    }
}
```

依然放在allprojects里面。

这样可以编译通过，但是AS界面上依然报红，无法智能提示。原因是，编译是用的gradle命令，而IDE界面是根据gradle生成的idea配置文件：`.idea/*.iml`。

在`.idea/modules/app/AppName.app.main.iml`文件，拖到下面“orderEntry”标签位置，把`<orderEntry type="library" name="Gradle: D./MyCode/ASWorkspace/AndroidApps/AppPeace/sdk/framework.jar" level="project" />` 这一行移到`<orderEntry type="jdk" jdkName="Android API 29 Platform" jdkType="Android SDK" />`这行的前面就行了。

可以在app的build.gradle里面添加一段代码自动完成这个过程，示例：

```
    preBuild {
        doLast {
            //或者根据实际情况写死路径：
            def imlpath = "../.idea/modules/app/" + rootProject.name + "." + project.name + ".main.iml"
            def imlFile = file(imlpath)
            try {
                def parsedXml = (new XmlParser()).parse(imlFile)
                def jdkNode = parsedXml.component[1].orderEntry.find { it.'@type' == 'jdk' }
                parsedXml.component[1].remove(jdkNode)
                def sdkString = "Android API " + android.compileSdkVersion.substring("android-".length()) + " Platform"
                new Node(parsedXml.component[1], 'orderEntry', ['type': 'jdk', 'jdkName': sdkString, 'jdkType': 'Android SDK'])
                groovy.xml.XmlUtil.serialize(parsedXml, new FileOutputStream(imlFile))
            } catch (FileNotFoundException e) {
                // nop, iml not found
                println "no iml found"
            }
        }
    }
```

实测下，在我的版本上，doLast不执行，把doLast删掉即可。