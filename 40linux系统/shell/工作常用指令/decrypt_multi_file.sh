# 此脚本用来对公司的新版加密文件进行解密
# 场景：
#    一份代码中往往有成百上千个文件，领导一次一次审核过于麻烦，所以有必要通过外挂的方式。
#
# 方法：
# 公司更新加密软件后，方法发生了变化，
# 1. 对于非java、cpp、c、h等格式的文件，只需要先把文件后缀改成java（或h、c等），再用git bash改回去，文件就会变成未加密。（要点是：要用git bash）
# 2. 对于java、cpp、c、h等格式的文件，反其道而行之，先用git bash把格式改成其它（如txt）即可解密。 注意如果用git bash把后缀改回来，会再次加密，因此需要用Windows PowerShell或explorer资源管理器把后缀改回来。
# 3. 目前我只找了java、cpp、c、h几种后缀，如果使用时发现其它后缀，加到第26行arr数组里即可。
# 4. 和公司IT配置的策略有关，所以随时可能失效，不要奇怪。
# 5. 对于U盘里的文件，应该也没用，只能解位于本地硬盘的文件。没试验，理论如此。
#
# 
# 重要声明：
# 1. 仅用于学习，不得违反公司规章政策。有需求请通过正式途径。
# 2. 为了以防万一脚本有bug破坏原文件，请在执行脚本前，先做好备份。
# 3. 出了问题概不负责
# author：佚名
# time: 2024/7/1

IFS=$'\n' #解决遍历目录时路径有空格

# fix bug: 假如刚好目录存在a.java和a.java.txt， 脚本在重命名时会导致一个文件丢失.通过加一段随机文本解决
random="7Qx2Fy&9Bkl"
arr=("java" "c" "cpp" "h")

# 递归遍历解密目录
function read_dir(){
	for file in `ls "$1"`
	do
	    full=$1/$file
	    if [ -d "$full" ] #如果是一个目录，则递归
	    then
	        read_dir "$full"
	    elif [ -f "$full" ];then
			decrypt_file "$full"
	    else
	        echo \"$full\" 既不是目录也不是普通文件
	    fi
	done
}
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
	# else # 如需解密非代码文件，把else放开，速度会非常慢
	    # mv "$src" "${src}.${random}.h"
		# sleep 5
		# mv "${src}.${random}.h" "$src"
	fi
}

if [ $# == 0 ];then 
    echo "将目标文件或目录拖到decrypt.sh上面,用gitbash执行..."
	exit 1
elif (( $#>1 ));then
    echo "错误命令行参数"
	exit 1
fi

current_dir=$(pwd)
absolute_path=$(realpath "$1")
relative_path=${absolute_path#$current_dir/}
parent_dir=$(dirname "$absolute_path")
name=$(basename "$1")
echo 路径:$absolute_path 

if [ -f "$absolute_path" ];then
    #解密文件
	decrypt_file "$relative_path"
elif [ -d "$absolute_path" ];then
	##解密目录
	read_dir "$relative_path"
fi

read -n 1 -p "Press any key to continue..."