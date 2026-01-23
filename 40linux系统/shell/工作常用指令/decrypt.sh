# 此脚本用来对公司的加密文件进行解密
# 场景：
# 1. 一份代码中往往有成百上千个文件，而公司的加密系统一次只能申请解密12个文件，所以有必要通过外挂的方式。
# 2. android studio安装NDK文件是加密的，工具链不能正常工作
#
# 原理：
# 为了保证员工能正常工作，必须保证有些软件能正常打开和编辑加密文件，不可能每次都先申请解密，再用软件打开
# 所以公司会配置策略，根据文件类型（后缀名）和软件（进程名），匹配上就自动解密，不需要申请
# 比如，txt类型的加密文件，用记事本打开是正常显示的，用notepad++打开则是加密的字符。
# 那么我们只需先用记事本打开文件，复制内容，然后用notepad++打开，粘贴进去，覆盖加密文本，保存。文件就会变成解密状态。
# 利用这个原理，写成脚本，批量处理。因为加密系统是根据后缀名来的，所以对于其它的文件类型，把后缀名改下，解密后再改回来即可。
# 对于二进制文件，理论上，也适用，但是可靠性不敢保证，解密后需自行检查。
# 此脚本仅用来批量解密代码文件。不得用做其它用途。
# 作者：朱标
# modified by xue
# version: 2.0

echo -e "\n输出到 xxx_decrypted目录(xxx代表文件名)，解密前尽量先自行确保不存在'xxx_decrypted'同名目录(Ctrl+C中断)"

read -n 1 -p "Press any key to continue..."

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
else
    echo "即将解密：$1"
	absolute_path=$(realpath "$1")
    parent_dir=$(dirname "$absolute_path")
	name=$(basename "$1")
	
	output_folder="${name}_decrypted"
	echo "Debug: $parent_dir, $name ,output: $output_folder"
	echo -e '\n'
	if [ -e "$parent_dir/$output_folder" ];then
	    #备份已经存在的ouput目录,避免被覆盖
	    mv "$parent_dir/$output_folder" "$parent_dir/$output_folder`date -d today +"%Y_%m_%d_%H_%M_%S"`"
	fi
	#创建输出目录
	mkdir -p "$parent_dir/$output_folder/origin"
	cp -rf "$1" "$parent_dir/$output_folder/origin/"
	echo -e "cp -rf $1 $parent_dir/$output_folder/origin/ \n"
	mkdir -p "$parent_dir/$output_folder/decrypt"
	
	base_src_folder="$parent_dir/$output_folder/origin"
	base_dst_folder="$parent_dir/$output_folder/decrypt"
	read_dir "$base_src_folder" "$base_dst_folder"
fi
