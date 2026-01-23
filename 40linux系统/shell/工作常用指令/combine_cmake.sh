# use add_subdirectory to import all cmakelists into one file
# 举例：命令 ./combine_cmake.sh system/core/init, 会在system/core/init目录下生成cmakelists文件

function generageCMakeLists() {
    if [ $# == 0 ];then
        echo "未传参，需要传入相对于android根目录的路径"
        exit
    else
        if [ -z "$ANDROID_BUILD_TOP" ];then
            echo "先执行source build/envsetup.sh && lunch xxx"
            exit
        else
            projPath="$ANDROID_BUILD_TOP/$1"
            echo "project path=$projPath"
            if [ ! -e $projPath ];then
                echo "not exist: $projPath"
                exit
            fi

            clionDir=$ANDROID_BUILD_TOP/out/development/ide/clion
            cmakesDir="$ANDROID_BUILD_TOP/out/development/ide/clion/$1"
            if [ ! -e $cmakesDir ];then
                echo "not found: $cmakesDir"
                exit
            fi

            # 1. 
            cmakeFile="$projPath/CMakeLists.txt"
            echo "cmake_minimum_required(VERSION 3.5)" > $cmakeFile
            projName=$(basename "$projPath")
            echo -e "project($projName)\n" >> $cmakeFile

            # 2. 
            echo "set(ANDROID_ROOT $ANDROID_BUILD_TOP)" >> $cmakeFile
            echo -e "set(ANDROID_CMAKE_DIR \${ANDROID_ROOT}/out/development/ide/clion)\n" >> $cmakeFile
            echo -e "set(PROJ_CMAKE_DIR \${ANDROID_CMAKE_DIR}/$1)\n" >> $cmakeFile

            # 3.
            for line in `find $cmakesDir -name CMakeLists.txt`
            do
                if [[ "$line" == *"-arm64-android"* ]]
                then
                    echo $line
                    path=$(dirname $line)
                    path=${path/$cmakesDir/\${PROJ_CMAKE_DIR\}}
                    echo -e "add_subdirectory(${path} ${path}/build)" >> $cmakeFile
                fi
            done
        fi
    fi
}

generageCMakeLists "$@"
