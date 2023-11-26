# 这个脚本用于给安卓源码里的C++工程生成compile_commands.json
# 先设置：
# export SOONG_GEN_CMAKEFILES=1
# export SOONG_GEN_CMAKEFILES_DEBUG=1
# 然后整编代码或者编译需要导入工程的模块
# 假设待导入工程为: aosp_dir/**/projA,
# 假设使用VS Code作为IDE，
# 将本脚本放到: aosp_dir/**/projA/.vscode/
# 然后执行，会在aosp_dir/out/development/ide/clion/**/projA下面生成compile_commands.json文件
# 并创建符号链接：aosp_dir/**/projA/.vscode/compile_commands.json 指向真实的文件。
# 脚本会遍历projA下面的所有子构建项，并把子构建项的编译命令汇集在一个compile_commands.json里。

# 执行前，先安装jq: sudo apt install jq

WORK_DIR=$(cd $(dirname $0); pwd)
ANDROID_BUILD_TOP=/home/xue/toyota_24mm_dev/source/android

if [[ $WORK_DIR != $ANDROID_BUILD_TOP* ]]; then
    echo Do not support directory outside android root
    exit
fi
WORK_DIR_REAL=${WORK_DIR%/.vscode*} #去掉末尾/.vscode
echo workspacedir=$WORK_DIR_REAL
target_dir=$ANDROID_BUILD_TOP/out/development/ide/clion/${WORK_DIR_REAL#*$ANDROID_BUILD_TOP/}
echo target_dir=$target_dir

cd $target_dir
if [ $? -ne 0 ]; then
    echo "dir not exits"
    exit
fi
echo -e "\n\n"
for line in `find . -name CMakeLists.txt`
do
    if [[ "$line" == *"-arm64-android"* ]]
    then
        echo $line
        cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_CXX_COMPILER=clang -DCMAKE_C_COMPILER=clang $line
    fi
done
# merge compile_commands to one file
# sudo apt install jq, if jq is not installed
jq -s 'map(.[])' `find . -name compile_commands.json` > compile_commands.json

rm $WORK_DIR/compile_commands.json
ln -s $target_dir/compile_commands.json $WORK_DIR/compile_commands.json 

