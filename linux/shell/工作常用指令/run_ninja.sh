mydir=`pwd`
echo ${mydir}

cd ${ANDROID_BUILD_TOP}
${ANDROID_BUILD_TOP}/prebuilts/build-tools/linux-x86/bin/ninja -v  -f ${mydir}/build_cpp_armv8.ninja -w dupbuild=err

cd ${mydir}

