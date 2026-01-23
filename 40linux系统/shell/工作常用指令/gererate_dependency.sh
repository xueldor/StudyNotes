protofiles=(
                default/proto/user_key.proto
                default/proto/user_face.proto
		   )
halFiles=(
	ISerial.hal
	ISerialCallback.hal
	IHsaeFace.hal
	IHsaeFaceCallback.hal
)


cd ../
mydir=`pwd`

echo protobuf源文件: ${protofiles[@]}
echo hal文件: ${halFiles[@]}

while ! test -d frameworks -a -d bionic -a -d bootable -a -d system -a -d vendor
do
   cd ../
done
ANDROID_BUILD_TOP=`pwd`

cd $mydir
echo -e "cd ${mydir}\n"

echo "----------首先保证之前整编过一次------------"
#定义变量和文件路径
#protobuf相关
aprotoc=${ANDROID_BUILD_TOP}/out/soong/host/linux-x86/bin/aprotoc 
proto_cpp_out=${ANDROID_BUILD_TOP}/out/soong/.intermediates/vendor/hsae/hardware/interfaces/early_autocore/serial/1.0/default/vendor.hsae.hardware.early_autocore.serial@1.0-service/android_vendor.30_arm64_armv8-a/gen/proto

#hidl
hidlGen=${ANDROID_BUILD_TOP}/out/soong/host/linux-x86/bin/hidl-gen
fqName=vendor.hsae.hardware.early_autocore.serial@1.0
genCppDir=${ANDROID_BUILD_TOP}/out/soong/.intermediates/vendor/hsae/hardware/interfaces/early_autocore/serial/1.0/vendor.hsae.hardware.early_autocore.serial@1.0_genc++/gen
genHeadDir=${ANDROID_BUILD_TOP}/out/soong/.intermediates/vendor/hsae/hardware/interfaces/early_autocore/serial/1.0/vendor.hsae.hardware.early_autocore.serial@1.0_genc++_headers/gen
roots="-rvendor.hsae.hardware.early_autocore:${ANDROID_BUILD_TOP}/vendor/hsae/hardware/interfaces/early_autocore -randroid.hidl:${ANDROID_BUILD_TOP}/system/libhidl/transport"
depCppfile=${ANDROID_BUILD_TOP}/out/soong/.intermediates/vendor/hsae/hardware/interfaces/early_autocore/serial/1.0/vendor.hsae.hardware.early_autocore.serial@1.0_genc++/gen/vendor/hsae/hardware/early_autocore/serial/1.0/SerialAll.cpp.d
depHeadfile=${ANDROID_BUILD_TOP}/out/soong/.intermediates/vendor/hsae/hardware/interfaces/early_autocore/serial/1.0/vendor.hsae.hardware.early_autocore.serial@1.0_genc++_headers/gen/vendor/hsae/hardware/early_autocore/serial/1.0/ISerial.h.d

#clang编译器
clang_path=${ANDROID_BUILD_TOP}/prebuilts/clang/host/linux-x86/clang-r383902b1/bin/clang++


echo "处理protobuf文件..."
result=`${aprotoc} --cpp_out=:${proto_cpp_out} -I . ${protofiles[*]} 2>&1`
echo -e "\033[31m ERROR: ${result} \033[0m"
tree ${proto_cpp_out}

echo "处理hal文件..."
rm -rf ${genHeadDir}
${hidlGen} -R -p . -d ${depHeadfile} -o ${genHeadDir} -L c++-headers ${roots} ${fqName}
rm -rf ${genCppDir}
${hidlGen} -R -p . -d ${depCppfile} -o ${genCppDir} -L c++-sources ${roots} ${fqName}
tree ${genHeadDir}
tree ${genCppDir}
