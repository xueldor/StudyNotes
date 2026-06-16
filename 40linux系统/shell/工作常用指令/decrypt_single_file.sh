#!/usr/bin/sh
# 思路：
# 为了能够正常工作，IT必然要配置一些特权软件，这些软件能正常读取加密文件，创建的文件是不加密的。
#
# 重要声明：
# 1. 安全起见，限制仅解密单个文件，大量文件请走正规渠道
# 2. 仅用于学习，出了问题概不负责

IFS=$'\n' #解决遍历目录时路径有空格
set +o histexpand # 解决echo -e "不支持目录!!!\n" 不支持双引号里面感叹号，防止! 被解析成 “查找历史命令”

# fix bug: 假如刚好目录存在a.java和a.java.txt， 脚本在重命名时会导致一个文件丢失.通过加一段随机文本解决
random="7Qx2Fy(9Bkl"
arr=("java" "c" "cpp" "h")

function decrypt_file(){
    src=$1
	postfix=${src##*.}  #文件后缀
	#echo -e "decrypt file $src"

	found=false
	for item in "${arr[@]}"; do
		if [[ "$item" == "$postfix" ]]; then
			found=true
			break
		fi
	done
	if $found;then # 对于java c cpp .h .py这类文件
	    mv "$src" "${src}.${random}.xls"
		mv2 "${src}.${random}.xls" "${src}"  #cp mv mv2
	else # 其它后缀
	    mv "$src" "${src}.${random}.h"
		sleep 5
		mv "${src}.${random}.h" "$src"
	fi
}

if [ $# == 0 ];then 
    echo "将待解密文件拖到脚本上,用gitbash执行..."
	echo "或命令行： ./decrypt_single_file.sh filename"
	exit 1
elif (( $#>1 ));then
    echo "只支持单个文件"
	exit 1
fi

current_dir=$(pwd)
param1=$(cygpath -u "$1") #Windows路径转为Git Bash识别的Linux格式路径
echo 路径:$current_dir $param1 

absolute_path=$(realpath "$param1") #文件完整路径
parent_dir=$(dirname "$absolute_path") #文件所属目录
name=$(basename "$param1") #文件名
echo 路径:$absolute_path 


if [ -f "$absolute_path" ];then
    #解密文件
	decrypt_file "$absolute_path"
elif [ -d "$absolute_path" ];then
	##解密目录
	echo -e "不支持目录!!!\n"
else 
	echo -e '无效路径!!!\n'
fi

read -n 1 -p "Press any key to continue..."