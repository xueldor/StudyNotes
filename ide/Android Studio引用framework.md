AndroidStudio项目中引用framework.jar，首先添加：

```
dependencies {
    compileOnly files('../sdk/framework.jar')
}
```

用compileOnly方式，避免打包到apk里面。

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