. setenv.sh
./shutdown.sh

opengrokconfig="/home/work/opengrok_config"
sourceDir="source"
opengrokjar="/home/work/soft/opengrok-1.13.7/lib/opengrok.jar"

function deleteIndex(){
    for file in `ls "$opengrokconfig/$sourceDir"`
    do
       echo rm -rf "$opengrokconfig/data_$file"/*
       rm -rf "$opengrokconfig/data_$file"/*
    done
}

function configuration(){
	for file in `ls "$opengrokconfig/$sourceDir"`
	do
	    full=`readlink -f "$opengrokconfig/$sourceDir/$file"` #符号链接指向的目标
        echo $full	    
        if [ -d "$full" ] #如果是一个目录
	    then
		    mkdir "data_$file"
			java -Xmx2096m \
				-jar "$opengrokjar" \
				-W "$opengrokconfig/data_$file/configuration.xml" \
				-c "/usr/local/bin/ctags" \
				-P -S -v \
				-s "$full" \
				-d "$opengrokconfig/data_$file" \
			   --include "*.java"  \
			   --include "*.c"  \
			   --include "*.cpp"  \
			   --include "*.cc"  \
			   --include "*.h"  \
			   --include "*.hpp"  \
			   --include "*.mk"  \
			   --include "*.bp"  \
			   --include "*.aidl"  \
			   --include "*.hidl"  \
			   --include "*.xml"  \
			   --ignore "d:out"  \
			   --ignore "d:build"  \
			   --ignore "d:cts"  \
			   --ignore "d:dalvik"  \
			   --ignore "d:developers"  \
			   --ignore "d:development"  \
			   --ignore "d:device"  \
			   --ignore "d:pdk"  \
			   --ignore "d:platform_testing"  \
			   --ignore "d:sdk"  \
			   --ignore "d:test"  \
			   --ignore "d:testing"  \
			   --ignore "d:tests"  \
			   --ignore "d:toolchain"  \
			   --ignore "d:tools"  \
			   --ignore "d:.git"  \
			   --ignore "d:.repo"
	    else
	        echo skip \"$full\"
	    fi
	done
}
deleteIndex
configuration

./startup.sh

