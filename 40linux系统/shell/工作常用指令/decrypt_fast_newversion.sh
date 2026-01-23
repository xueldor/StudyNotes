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
	# 文件较多时，为了提高速度，可以把下面一行echo注释掉
	echo -e "decrypt file $src"
	
	found=false
	for item in "${arr[@]}"; do
		if [[ "$item" == "$postfix" ]]; then
			found=true
			break
		fi
	done
	if $found;then
	    mv "$src" "${src}.${random}.java"
		# 打开一个powershell。这一步导致解密速度慢。
		powershell mv \""${src}.${random}.java"\" \""${src}"\"
	else
	    mv "$src" "${src}.${random}.java"
		# 有时会解密失败，失败的文件手动执行这个命令即可
		mv "${src}.${random}.java" "$src"
	fi
}

if [ $# == 0 ];then 
    echo "将目标文件或目录拖到decrypt.sh上面,用gitbash执行..."
	exit 1
elif (( $#>1 ));then
    echo "暂时只支持单个文件或目录"
	exit 1
fi

echo "请务必先备份好原文件，再执行这个脚本"
echo "需要把此脚本放到待解密文件的同目录"
read -n 1 -p "Press any key to continue..."

current_dir=$(pwd)
absolute_path=$(realpath "$1")
relative_path=${absolute_path#$current_dir/}
parent_dir=$(dirname "$absolute_path")
name=$(basename "$1")
echo 路径:$absolute_path 

echo '是否需要我帮你备份一下源文件(y/n)？'
read backup
if [ "$backup" = "y" ];then
	tar -cvf "${absolute_path}.tar" "$1"
else
	echo -e "即将开始，祝你好运!!!\n"
fi

# 由于可能会调用powershell，路径的格式和linux shell不一样，比如shell是/h/aaaaaa,而powershell是H:\aaaaaa
# 为了避免处理路径格式问题，规定需要把脚本放到待解密的文件同目录，并采用相对路径
if [ -f "$absolute_path" ];then
    #解密文件
	decrypt_file "$relative_path"
elif [ -d "$absolute_path" ];then
	##解密目录
	read_dir "$relative_path"
fi

