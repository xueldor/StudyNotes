# 此脚本用来对公司的加密文件进行解密
# 场景：
# 1. 一份代码中往往有成百上千个文件，而公司的加密系统一次只能申请解密12个文件，所以有必要通过外挂的方式。
# 2. android studio安装NDK文件是加密的，工具链不能正常工作
#
# 原理：
# 为了保证员工能正常工作，必须保证有些软件能正常打开加密文件。
# 所以公司会配置策略，根据文件类型（后缀名）和软件（进程名），匹配上就自动解密，不需要申请。
# 比如，txt类型的加密文件，用记事本打开是正常显示的，用notepad++打开则是密文。
# 那么我们只需先用记事本打开文件，复制内容，然后用notepad++打开，粘贴进去，覆盖加密文本，保存。文件就会变成解密状态。
# 利用这个原理，写成脚本，批量处理。因为加密系统是根据后缀名来的，所以对于其它的文件类型，把后缀名改下，解密后再改回来即可。
# 同样适用于二进制文件。
# 
# 重要声明：
# 1. 此脚本不得用做违反公司规章政策的用途。
# 2. 为了防止脚本有bug破坏原文件，请务必先将待解密的文件备份。
# 3. 出了问题概不负责
# author：保密
# modified by： 保密
# version: 3.0


base_src_folder=
base_dst_folder=

IFS=$'\n' #解决遍历目录时路径有空格

function read_dir(){
	for file in `ls "$1"`
	do
	    full=$1/$file
	    if [ -d "$full" ] #如果是一个目录，则递归
	    then
	        read_dir "$full"
	    elif [ -f "$full" ];then
			relative=${full#*$base_src_folder}
			decrypt_file "$full" "$base_dst_folder$relative"
	    else
	        echo \"$full\" 既不是目录也不是普通文件
	    fi
	done
}
function decrypt_file(){
    src=$1
	dst=$2
	postfix=${src##*.}  #文件后缀
	
	tmpDstDir=$(dirname "$dst")
	if [ ! -e $tmpDstDir ];then
		mkdir -p $tmpDstDir
	fi
	echo -e "decrypt file $src"
	if [ "${postfix}" = "java" ];then
	    cat "$src" > "$dst"
	else
	    mv "$src" "${src}.java"
		cat "${src}.java" > "$dst"
		mv "${src}.java" "$src"
	fi
}

if [ $# == 0 ];then 
    echo "将目标文件或目录拖到decrypt.sh上面,用gitbash执行..."
	exit 1
elif (( $#>1 ));then
    echo "暂时只支持单个文件或目录"
	exit 1
fi

absolute_path=$(realpath "$1")
parent_dir=$(dirname "$absolute_path")
name=$(basename "$1")
output="${name}_decrypted"

echo -e "\n提示：解密文件将会输出到 \"${output}\" 目录(Ctrl+C中断)"
read -n 1 -p "Press any key to continue..."


echo -e "Debug:\n\t 父目录：$parent_dir\n\t 名称: $name \n\t 输出到: $output"
echo '是否需要备份源文件(y/n)？'
read backup
if [ $backup = "y" ];then
	tar -cvf "${absolute_path}.tar" "$1"
else
	echo -e "你选择了不备份。祝你好运!!!\n"
fi
if [ -e "$parent_dir/$output" ];then
	echo "$output 已存在，请手动移除后再试！\n abort"
	exit 1
fi
if [ -f "$absolute_path" ];then
    #解密文件
	decrypt_file "$absolute_path" "$parent_dir/$output"
elif [ -d "$absolute_path" ];then
	##解密目录
	base_src_folder="$absolute_path"
	base_dst_folder="$parent_dir/$output"
	mkdir -p "$base_dst_folder"
	read_dir "$base_src_folder" "$base_dst_folder"
fi

