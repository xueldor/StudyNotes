基于 SOONG_GEN_CMAKEFILES 生成的 CMakeLists.txt，目的是为了让IDE能：

* 识别源码结构
* 跳转符号
* 做代码补全

只能用于“辅助开发/IDE”，原则上不能可靠地产出可执行文件。因为：

* 没有完整 toolchain
* 没有正确 sysroot
* 没有 linker
* 依赖路径可能是 Soong 内部路径（out/...）
* flags 不完整（特别是隐式依赖）

从安卓源码里将这些依赖库路径“挖”出来，指定给cmake即可。下面几个示例覆盖：

* 通过脚本android_cmake_env.sh设置，先执行source ../android_cmake_env.sh, 再执行cmake
  
  这是比较传统的方法，编译没有问题，但是如果用IDE打开项目，如何让IDE先执行脚本再解析cmake文件，是个问题，有些IDE可以配。

* 创建工具链文件“toolchain.cmake”的方法。可下载NDK，用NDK里提供的cmake。

* 直接在CMakeLists.txt里添加。缺点是，有些变量的设置时间太晚，可能不生效。须甄别出这些变量，放到合适的位置。

* 创建CMakePresets.json文件(要求cmake版本比较高)

鉴于有些特性依赖比较高的cmake版本，ubuntu默认软件源不一定提供高版本，故此提供升级方法：

```
apt purge --auto-remove cmake #卸载原来的低版本cmake
#下载并安装 Kitware 的 GPG 密钥
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | sudo apt-key add -
cd /etc/apt/sources.list.d
新建文件，文件名可以就叫cmake.list，或kitware.list，添加
deb https://apt.kitware.com/ubuntu/ bionic main   # bionic对应的是 Ubuntu 18.04 LTS
deb https://apt.kitware.com/ubuntu/ focal main   #对应的是 Ubuntu 20.04 LTS
deb https://apt.kitware.com/ubuntu/ jammy main   #对应的是 Ubuntu 22.04 LTS
deb https://apt.kitware.com/ubuntu/ noble main   # 对应的是 Ubuntu 24.04 LTS
deb https://apt.kitware.com/ubuntu/ resolute main   # 对应 Ubuntu 26.04 LTS

sudo apt update
sudo apt install cmake
```

如果方式失效，可下载cmake代码，编译安装：

```
# 以3.14为例，高版本应该也是一样的
wget https://cmake.org/files/v3.14/cmake-3.14.5.tar.gz

tar xzvf cmake-3.14.5.tar.gz
apt install gcc g++
cd make-3.14.5
./bootstrap
make
make install
```

几个示例均基于SOONG_GEN_CMAKEFILES生成的 CMakeLists.txt，演示几种不同方法。

## 示例一

希望尽量最小化的修改CMakeLists.txt，构建最终的可执行文件。我们把配置尽量放shell脚本里。

这是第一个示例我解释具体详细一些。后面的示例二就一笔带过了。

基于securityta10的hal服务。

1. 把CMakeLists.txt复制到hal服务源码目录，打开，在末尾追加target_link_libraries。需要追加的库可以从Android.bp获知，并补充c、dl、c++这些C++程序的基础库。
   
   ```
   add_executable(vendor.iauto.hardware.securitychip-1.0-service ${SOURCE_FILES})
   set_target_properties(vendor.iauto.hardware.securitychip-1.0-service
       PROPERTIES
       OUTPUT_NAME "vendor.iauto.hardware.securitychip@1.0-service"
       PREFIX ""
   )
   # 后面追加link库信息，这样才能链接生成exe，否则只能.cpp->.o
   target_link_libraries(vendor.iauto.hardware.securitychip-1.0-service
       c dl c++ base hidlbase log cutils utils hidltransport hardware crypto QSEEComAPI ion :vendor.iauto.hardware.securitychip@1.0.so
   )
   ```
   
   * hal程序的文件名应该是vendor.iauto.hardware.securitychip@1.0-service，但是“@”在cmake里有特殊的含义，不支持名称里含有“@”符号。
   * Android.bp里依赖的libbase.so, CMakeLists.txt里应写成base,去掉前面的lib。
   * vendor.iauto.hardware.securitychip@1.0.so这个库，原本前面就没有lib，所以只能用指定名字的方式。
   * 额外添加libc libdl libc++以及crtbegin_dynamic、crtend_android静态库，必要时还要加上libm。原因是我会在LDFLAGS指定“-nostdlib”,故默认不加载，需要我手动添加链接。这种方式精确控制得当，可大幅减小可执行文件体积。如果不指定“-nostdlib”，不管模块实际是否用到，链接器都把这些基础库加进来。

2. 此时虽然指定了依赖库，但是链接器不知道到哪儿找，需要脚本里给出搜索路径，另外还需脚本指定LDFLAGS：
   
   ```
   //新建一个“android_cmake_env.sh”文件
   export aosp_root="/home/xuexiangyu/workspace/24mm_t2/apps/LINUX/android"
   export LDFLAGS="-target aarch64-linux-android32 \
       -fuse-ld=lld \
       -nostdlib \
       -pie \
       -Wl,-dynamic-linker,/system/bin/linker64 -v \
       ${aosp_root}/out/soong/.intermediates/bionic/libc/crtbegin_dynamic/android_vendor.32_arm64_armv8-a/crtbegin_dynamic.o \
       -L${aosp_root}/out/target/product/msmnile_au/system/apex/com.android.runtime/lib64/bionic \
       -L${aosp_root}/out/target/product/msmnile_au/system/lib64 \
       -L${aosp_root}/out/target/product/msmnile_au/vendor/lib64 "
   ```

3. 上面我只指定了4个路径，并且没有指定sysroot：
   
   > //不加-L,依赖的静态库
   > 
   > out/soong/.intermediates/bionic/libc/crtbegin_dynamic/android_vendor.32_arm64_armv8-a/crtbegin_dynamic.o
   > 
   > //加-L，到这些目录下找so
   > 
   > out/target/product/msmnile_au/system/apex/com.android.runtime/lib64/bionic
   > 
   > out/target/product/msmnile_au/system/lib64
   > 
   > out/target/product/msmnile_au/vendor/lib64
   
   product/msmnile_au/system/lib64里文件是编译快完成的最后阶段，拷贝过去的。真实mm编译的时候，引用的是out/soong/.intermediates路径里的so。一个一个的搜索这些路径比较麻烦，所以方便起见，整编过后，直接用target/product/msmnile_au/system/lib64就行了。
   
   另外有些库还可以到prebuilts/ndk/下找。有些平台把prebuilts/ndk阉割掉了，只能到out下面去找，总能找到的。

4. **（重要）**交叉编译安卓时，不能用系统默认的 GNU ld（ld.bfd），必须改用 LLVM 的 lld 链接器，所以“-fuse-ld=lld”必不可少，少则不能编译。“-Wl,-dynamic-linker,/system/bin/linker64”最好也加上，防止能构建，push进去不能运行。

5. cpp编译成.o文件的过程，我们不需要关心，自动生成的CMakeLists.txt已经包含了。如果你非要自己写，需要设置一堆交叉编译工具链。例如：
   
   ```
   #参考
   export clang_toolchain_dir="${aosp_root}/prebuilts/clang/host/linux-x86/clang-r416183b1"
   export PATH="${cmake_bin_path}:${PATH}"
   export CC="${clang_toolchain_dir}/bin/clang --target=aarch64-linux-android "
   export CXX="${clang_toolchain_dir}/bin/clang++ --target=aarch64-linux-android "
   export LD="${clang_toolchain_dir}/bin/ld.lld"
   export LDFLAGS="-fuse-ld=lld"
   export AR="${clang_toolchain_dir}/bin/llvm-ar"
   export AS="${clang_toolchain_dir}/bin/llvm-as"
   ```

6. 最后，基于androd_cmake_env.sh和CMakeLists.txt，构建目标
   
   ```
   mkdir build
   cd build
   source ../androd_cmake_env.sh
   cmake .. -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY
   make
   #-----》 vendor.iauto.hardware.securitychip-1.0-service文件就在build目录下，可以push到车机
   ```
   
   为什么要加“-DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY”呢，因为我们的配置是不完整的，比如，没有指定sysroot。CMake 在 `project()` 命令执行时会启用语言（如 C/C++），并立即开始对工具链进行一系列测试（如检测编译器 ABI 信息）。交叉编译不指定sysroot的话，测试肯定不能通过，一堆配置，没必要，不影响编译，所以“-DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY”跳过这一步。
   
   ```
   # 不加“-DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY"
   -- Check for working C compiler: /usr/bin/cc
   -- Check for working C compiler: /usr/bin/cc -- broken
   CMake Error at /usr/share/cmake-3.16/Modules/CMakeTestCCompiler.cmake:60 (message):
     The C compiler
   
       "/usr/bin/cc"
   
     is not able to compile a simple test program.
   
   同时注意到compiler仍然是/usr/bin/cc。有两个方法：
   1、把CMAKE_C_COMPILER/CMAKE_CXX_COMPILER放到project前面；
   2、脚本里指定CC、CXX变量
   指定了CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY后，其实是无所谓的。
   ```
   
   把CMAKE_TRY_COMPILE_TARGET_TYPE也一起写到androd_cmake_env.sh里也不可以，CC、CXX  、CFLAGS  这些环境变量会被 CMake 自动读取，而CMAKE_TRY_COMPILE_TARGET_TYPE是cmake变量，不是环境变量，也不会自动从环境变量读取赋给cmake变量。解决方法仍然是把set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)写到project()前面。
   
   > cmake3.10必须指定“-DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY”，版本比较高的话，已经不需要了。

最后附上完整的、执行通过的脚本

```shell
#!/bin/bash
get_aosp_root() {
    local current_path="$PWD"
    while [[ "$current_path" != "/" ]]; do
        if [[ -f "$current_path/build/envsetup.sh" ]]; then
            echo "$current_path"
            return 0
        fi
        current_path="$(dirname "$current_path")"
    done
    # 如果找不到，返回非零退出码
    return 1
}
export ANDROID_BUILD_TOP="$(get_aosp_root)"
check_android_build_top() {
  if [[ -z "$ANDROID_BUILD_TOP" ]]; then
      return 1
  fi
  if [[ ! -d "$ANDROID_BUILD_TOP" ]]; then
      return 1
  fi
  local current_path="$PWD"
  if [[ "$current_path" != "$ANDROID_BUILD_TOP"* ]]; then
      return 1
  fi
  return 0
}
check_android_build_top
if [[ $? -ne 0 ]]; then
    echo "找不到源码根路径，你可以手动修改脚本对ANDROID_BUILD_TOP赋值，或者到aosp根目录执行source build/envsetup.sh && lunch msmnile_au-userdebug"
    return 1
fi

if which cmake &> /dev/null; then
    echo "use cmake: $(which cmake)"
else
    echo "错误：找不到 cmake 命令"
    return 1
fi

export aosp_root="$ANDROID_BUILD_TOP"
export crtbegin_dynamic_o_path="${aosp_root}/out/soong/.intermediates/bionic/libc/crtbegin_dynamic/android_vendor.32_arm64_armv8-a/crtbegin_dynamic.o"
export CMAKE_MAKE_PROGRAM="${aosp_root}/prebuilts/build-tools/linux-x86/bin/ninja"
export PATH="${aosp_root}/prebuilts/build-tools/linux-x86/bin:${PATH}"
# 编译器
export CC="${aosp_root}/prebuilts/clang/host/linux-x86/clang-r416183b1/bin/clang --target=aarch64-linux-android "
export CXX="${aosp_root}/prebuilts/clang/host/linux-x86/clang-r416183b1/bin/clang++ --target=aarch64-linux-android "
# 链接器标志（关键参数）
# 链接器标志（关键参数）
LDFLAGS_ARGS=(
    "-target aarch64-linux-android32"
    "-fuse-ld=lld"
    "-nostdlib"
    "-pie"
    "-Wl,-dynamic-linker,/system/bin/linker64"
    "-v"
    "${aosp_root}/out/soong/.intermediates/bionic/libc/crtbegin_dynamic/android_vendor.32_arm64_armv8-a/crtbegin_dynamic.o"
    "--sysroot=${aosp_root}/out/target/product/msmnile_au/system/"
    "-L${aosp_root}/out/target/product/msmnile_au/system/apex/com.android.runtime/lib64/bionic"
    "-L${aosp_root}/out/target/product/msmnile_au/system/lib64"
    "-L${aosp_root}/out/target/product/msmnile_au/vendor/lib64"
)
export LDFLAGS="${LDFLAGS_ARGS[*]}"

echo "=========================================="
echo "Android CMake Environment Configuration"
echo "=========================================="
echo "ANDROID_BUILD_TOP: $ANDROID_BUILD_TOP"
echo "LDFLAGS: $LDFLAGS"
echo "=========================================="
echo ""
echo "To use this environment, run:"
echo "  cd build"
echo "  source ../android_cmake_env.sh"
echo "  cmake .. -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY -G Ninja"
echo "  ninja"
```

### 总结

1、通过LDFLAGS，指定：

    * 链接器 ldd
    * 头文件路径
    * 静态库路径
    * 动态库路径

2、CMakeLists.txt追加target_link_libraries，指定依赖库

3、编译时cmake命令加`-DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY`。否则你要给出完整的交叉编译环境配置，巨麻烦。

4、参考脚本

```
#################################
## set path to yourself
#################################
export aosp_root="/home/work/workspace01/TOYOTA_24MM/P42_R/apps/LINUX/android"
export cmake_bin_path="/home/work/workspace01/TOYOTA_24MM/cmake/cmake-4.2.1-linux-x86_64/bin"

export clang_toolchain_dir="${aosp_root}/prebuilts/clang/host/linux-x86/clang-r383902b1"
export PATH="${cmake_bin_path}:${PATH}"
export CC="${clang_toolchain_dir}/bin/clang --target=aarch64-linux-android "
export CXX="${clang_toolchain_dir}/bin/clang++ --target=aarch64-linux-android "
export LD="${clang_toolchain_dir}/bin/ld.lld"
export AR="${clang_toolchain_dir}/bin/llvm-ar"
export AS="${clang_toolchain_dir}/bin/llvm-as"

export BIONIC_LIBC_INC="${aosp_root}/bionic/libc/include"
export BIONIC_LIBCXX_INC="${aosp_root}/external/libcxx/include"

export CFLAGS="-Wall -Wextra -O2 -Wno-sign-compare -D__packed__ -DDFEATURE_LE_DIAG -DOE -I${BIONIC_LIBC_INC} -I{BIONIC_LIBCXX_INC}"
export CXXFLAGS="${CFLAGS}"

#################################
## android's libc
#################################
export gpte_vendor_lib_path="${aosp_root}/out/target/product/msmnile_au/vendor/lib64"
export crtbegin_dynamic_lib_dir=" -L${aosp_root}/out/soong/.intermediates/bionic/libc/crtbegin_dynamic/android_vendor.30_arm64_armv8-a "
export bionic_libdl_lib_dir=" -L${aosp_root}/out/soong/.intermediates/bionic/libdl "
export bionic_libm_lib_dir=" -L${aosp_root}/out/soong/.intermediates/bionic/libm "
export bionic_libc_llndk_lib_dir=" -L${aosp_root}/out/soong/.intermediates/bionic/libc/libc.llndk/android_vendor.30_arm64_armv8-a_shared "
export bionic_libdl_llndk_lib_dir=" -L${aosp_root}/out/soong/.intermediates/bionic/libdl/libdl.llndk/android_vendor.30_arm64_armv8-a_shared "
export bionic_libm_llndk_lib_dir=" -L${aosp_root}/out/soong/.intermediates/bionic/libm/libm.llndk/android_vendor.30_arm64_armv8-a_shared "
export gcc_libgcc_lib_dir=" -L${aosp_root}/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/lib/gcc/aarch64-linux-android/4.9.x "
export bionic_libcxx_lib_dir=" -L${aosp_root}/out/soong/.intermediates/external/libcxx/libc++/android_vendor.30_arm64_armv8-a_shared "

export crtbegin_dynamic_o_path="${aosp_root}/out/soong/.intermediates/bionic/libc/crtbegin_dynamic/android_vendor.30_arm64_armv8-a/crtbegin_dynamic.o"
export crtend_android_o_path="${aosp_root}/prebuilts/ndk/r21/platforms/android-29/arch-arm64/usr/lib/crtend_android.o"
export android_crtend_lib_dir=" -L${aosp_root}/prebuilts/ndk/r21/platforms/android-29/arch-arm64/usr/lib/"

export LDFLAGS=" -nostartfiles -fuse-ld=lld -v ${crtbegin_dynamic_o_path} ${bionic_libcxx_lib_dir} ${bionic_libc_llndk_lib_dir} ${bionic_libdl_llndk_lib_dir} ${bionic_libm_llndk_lib_dir} ${gcc_libgcc_lib_dir} ${crtbegin_dynamic_lib_dir} ${crtend_android_o_path} ${android_crtend_lib_dir} "

export crtbegin_lib_dir="${aosp_root}/prebuilts/ndk/r21/platforms/android-29/arch-arm64/usr/lib/"
export crtend_lib_dir="${aosp_root}/prebuilts/ndk/r21/platforms/android-29/arch-arm64/usr/lib/"

export LIBRARY_PATH="${crtbegin_lib_dir}:${crtend_lib_dir}:${gcc_libgcc_lib_dir}:${gpte_vendor_lib_path}"

export PATH="${aosp_root}/out/host/linux-x86/bin:${crtbegin_lib_dir}:${crtend_lib_dir}:${aosp_root}/prebuilts/ndk/r21/platforms/android-29/arch-arm64:${PATH}"
```

## 示例二

示例二不新建“android_cmake_env.sh”，直接在SOONG_GEN_CMAKEFILES生成的 CMakeLists.txt里修改

securitychip_pkcs11_ut是我的一个gtest单元测试程序，Android.bp如下：

```
cc_test {
    name: "securitychip_pkcs11_ut",
    vendor: true,
    cppflags: [
        //...省略
    ],
    srcs: [
        "test/pkcs11_test.cpp",
        //...省略
    ],
    header_libs: [
        "device_kernel_headers",
        "qti_kernel_headers",
        "vendor_common_inc",
    ],
    include_dirs: [
        "vendor/qcom/proprietary/securemsm/QSEEComAPI",
        "system/memory/libion/kernel-headers",
    ],
    local_include_dirs: [
        "include",
    ],
    shared_libs: [
        "libbase",
        "libion",
        "libQSEEComAPI_bp",
    ],
    static_libs: [
        "libgmock",
    ],
    compile_multilib: "64", 
    gtest: true,
}
```

同样，先把SOONG_GEN_CMAKEFILES生成的 CMakeLists.txt拷贝过来。

```
cp out/development/ide/clion/vendor/hsae/proprietary/hardware/securityta100/CA/libhsckteec/securitychip_pkcs11_ut-arm64-android/CMakeLists.txt vendor/hsae/proprietary/hardware/securityta100/CA/libhsckteec/
```

在CMakeLists.txt末尾追加：

```
add_executable(securitychip_pkcs11_ut ${SOURCE_FILES})
# 链接器参数
target_link_options(securitychip_pkcs11_ut PRIVATE
    -target aarch64-linux-android32
    -fuse-ld=lld
    -nostdlib
    -pie
    "LINKER:-dynamic-linker,/system/bin/linker64"
    -v
)
# 静态库
target_link_options(securitychip_pkcs11_ut PRIVATE
    ${ANDROID_ROOT}/out/soong/.intermediates/bionic/libc/crtbegin_dynamic/android_vendor.32_arm64_armv8-a/crtbegin_dynamic.o
    ${ANDROID_ROOT}/out/soong/.intermediates/external/googletest/googlemock/libgmock/android_vendor.32_arm64_armv8-a_static/libgmock.a
    ${ANDROID_ROOT}/out/soong/.intermediates/external/googletest/googletest/libgtest/android_arm64_armv8-a_static/libgtest.a
    ${ANDROID_ROOT}/out/soong/.intermediates/external/googletest/googletest/libgtest_main/android_arm64_armv8-a_static/libgtest_main.a
)

# 动态库搜索路径
target_link_directories(securitychip_pkcs11_ut PRIVATE
    ${ANDROID_ROOT}/out/target/product/msmnile_au/system/apex/com.android.runtime/lib64/bionic
    ${ANDROID_ROOT}/out/target/product/msmnile_au/system/lib64
    ${ANDROID_ROOT}/out/target/product/msmnile_au/vendor/lib64
)

target_link_libraries(securitychip_pkcs11_ut
    c m dl c++ base hidltransport hardware QSEEComAPI ion
)
```

## 示例三

可以用include直接引入“out/development/ide/clion“，放在项目路径。这样更方便，后续改了bp，也不要重新复制过来。缺点是对于library，自动生成的cmakelists依然是add_executable,而不是add_library，如何用include方式，不方面自己修改。

```
cmake_minimum_required(VERSION 3.10)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY) #必须在project前面。
project(vendor.iauto.hardware.securitychip@1.0-service)
if(DEFINED ENV{ANDROID_BUILD_TOP})
    set(ANDROID_ROOT $ENV{ANDROID_BUILD_TOP})
else()
    message(FATAL_ERROR "错误：请先指定源码路径，可通过export ANDROID_BUILD_TOP=/path/xxx, 或source build/envsetup.sh")
endif()
include("${ANDROID_ROOT}/out/development/ide/clion/vendor/hsae/proprietary/hardware/securityta100/CA/hal/default/vendor.iauto.hardware.securitychip@1.0-service-arm64-android/CMakeLists.txt")

# 安卓构建链
set(CMAKE_SYSROOT ${ANDROID_ROOT}/out/target/product/msmnile_au/system/)
set(CMAKE_FIND_ROOT_PATH
    ${ANDROID_ROOT}/out/target/product/msmnile_au/vendor
)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

target_link_options(vendor.iauto.hardware.securitychip-1.0-service PRIVATE
    -target aarch64-linux-android32
    -fuse-ld=lld
    -nostdlib
    -pie
    "LINKER:-dynamic-linker,/system/bin/linker64"
    -v
)
target_link_options(vendor.iauto.hardware.securitychip-1.0-service PRIVATE
    ${ANDROID_ROOT}/out/soong/.intermediates/bionic/libc/crtbegin_dynamic/android_vendor.32_arm64_armv8-a/crtbegin_dynamic.o
)
target_link_directories(vendor.iauto.hardware.securitychip-1.0-service PRIVATE
    ${ANDROID_ROOT}/out/target/product/msmnile_au/system/apex/com.android.runtime/lib64/bionic
    ${ANDROID_ROOT}/out/target/product/msmnile_au/system/lib64
    ${ANDROID_ROOT}/out/target/product/msmnile_au/vendor/lib64
)
set_target_properties(vendor.iauto.hardware.securitychip-1.0-service
    PROPERTIES
    OUTPUT_NAME "vendor.iauto.hardware.securitychip@1.0-service"
    PREFIX ""
)
target_link_libraries(vendor.iauto.hardware.securitychip-1.0-service
    c dl c++ base hidlbase log cutils utils 
    hidltransport hardware crypto QSEEComAPI ion
    :vendor.iauto.hardware.securitychip@1.0.so
    :vendor.hsae.hardware.ta100@1.0.so
)
```

cmake命令可省略-DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY

## 推荐方式(NDK)

通过Android SDK下载好最新的cmake和NDK。

设置环境变量ANDROID_NDK_HOME，以及cmake，确保cmake命令使用的是Android sdk里的。

下面提供完整示例，包含：

* 编译静态库
* 编译动态库
* 编译可执行程序

前面的示例只是针对可执行程序。编库有一些差异，而NDK提供的构建链能帮我们自动处理，节省配置。

### CMakePresets.json

创建CMakePresets.json文件（Android SDK内的cmake版本较高，支持CMakePresets）

```json
{
    "version": 6,
    "configurePresets": [
        {
            "name": "default",
            "generator": "Ninja",
            "binaryDir": "${sourceDir}/build/output",
            "cacheVariables": {
                "CMAKE_SYSTEM_NAME": "Android",
                "CMAKE_ANDROID_ARCH_ABI": "arm64-v8a",
                "CMAKE_SYSTEM_VERSION": "32",
                "CMAKE_ANDROID_NDK": "$env{ANDROID_NDK_HOME}"
            }
        },
        {
            "name": "default-release",
            "inherits": "default",
            "binaryDir": "${sourceDir}/build/release",
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Release"
            }
        },
        {
            "name": "traditional",
            "generator": "Ninja",
            "binaryDir": "${sourceDir}/build/ubuntu_output",
            "cacheVariables": {
                "CMAKE_TOOLCHAIN_FILE": "$env{ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake",
                "ANDROID_ABI": "arm64-v8a",
                "ANDROID_PLATFORM": "android-32",
                "ANDROID_NDK": "$env{ANDROID_NDK_HOME}"
            }
        },
        {
            "name": "build_onwindows_to_arm64",
            "displayName": "Windows2arm64 Debug",
            "description": "When use this preset, please download all dependencies to local machine,otherwise building will be very slow.Using copy_android_root_files.py to automatically download files",
            "generator": "Ninja",
            "binaryDir": "D:/workspace/build/securitychipca",
            "environment": {
                "ANDROID_NDK_HOME": "D:/Android/android-sdk/ndk/30.0.14904198",
                "CMAKE_PATH": "D:/Android/android-sdk/cmake/4.1.2",
                "ANDROID_ROOT": "D:/workspace/24mmt2_root"
            },
            "cacheVariables": {
                "CMAKE_SYSTEM_NAME": "Android",
                "CMAKE_ANDROID_ARCH_ABI": "arm64-v8a",
                "CMAKE_SYSTEM_VERSION": "32",
                "CMAKE_ANDROID_NDK": "$env{ANDROID_NDK_HOME}",
                "CMAKE_MAKE_PROGRAM": "$env{CMAKE_PATH}/bin/ninja",
                "CMAKE_BUILD_TYPE": "Debug"
            }
        },
        {
            "name": "no_ndk",
            "generator": "Ninja",
            "binaryDir": "${sourceDir}/build/nondk_output",
            "environment": {
                "ANDROID_ROOT": "${sourceDir}/../../../../../../.."
            },
            "cacheVariables": {
                "CMAKE_C_COMPILER": "$env{ANDROID_ROOT}/prebuilts/clang/host/linux-x86/clang-r416183b1/bin/clang",
                "CMAKE_CXX_COMPILER": "$env{ANDROID_ROOT}/prebuilts/clang/host/linux-x86/clang-r416183b1/bin/clang++",
                "CMAKE_LINKER": "$env{ANDROID_ROOT}/prebuilts/clang/host/linux-x86/clang-r416183b1/bin/ld.lld",
                "CMAKE_AR": "$env{ANDROID_ROOT}/prebuilts/clang/host/linux-x86/clang-r416183b1/bin/llvm-ar",
                "CMAKE_C_COMPILER_TARGET": "aarch64-linux-android32",
                "CMAKE_CXX_COMPILER_TARGET": "aarch64-linux-android32",
                "CMAKE_EXE_LINKER_FLAGS": "-fuse-ld=lld -nostdlib -pie $env{ANDROID_ROOT}/out/soong/.intermediates/bionic/libc/crtbegin_dynamic/android_vendor.32_arm64_armv8-a/crtbegin_dynamic.o $env{ANDROID_ROOT}/out/soong/.intermediates/bionic/libc/crtend_android/android_vendor.32_arm64_armv8-a/obj/bionic/libc/arch-common/bionic/crtend.o -lc -ldl -lm -Wl,--start-group $env{ANDROID_ROOT}/prebuilts/clang/host/linux-x86/clang-r416183b1/lib64/clang/12.0.7/lib/linux/libclang_rt.builtins-aarch64-android.a -Wl,--end-group  -Wl,-v,-dynamic-linker,/system/bin/linker64",
                "CMAKE_SHARED_LINKER_FLAGS": "-fuse-ld=lld -nostdlib $env{ANDROID_ROOT}/out/soong/.intermediates/bionic/libc/crtbegin_so/android_arm64_armv8-a/crtbegin_so.o $env{ANDROID_ROOT}/out/soong/.intermediates/bionic/libc/crtend_so/android_vendor.32_arm64_armv8-a/obj/bionic/libc/arch-common/bionic/crtend_so.o -lc -ldl -lm -Wl,--start-group $env{ANDROID_ROOT}/prebuilts/clang/host/linux-x86/clang-r416183b1/lib64/clang/12.0.7/lib/linux/libclang_rt.builtins-aarch64-android.a -Wl,--end-group ",
                "CMAKE_MODULE_LINKER_FLAGS": "-fuse-ld=lld",
                "CMAKE_CXX_STANDARD": "17",
                "CMAKE_CXX_STANDARD_REQUIRED": "ON",
                "CMAKE_CXX_EXTENSIONS": "OFF",
                "CMAKE_TRY_COMPILE_TARGET_TYPE": "STATIC_LIBRARY",
                "CMAKE_FIND_ROOT_PATH_MODE_PROGRAM": "NEVER",
                "CMAKE_FIND_ROOT_PATH_MODE_LIBRARY": "ONLY",
                "CMAKE_FIND_ROOT_PATH_MODE_INCLUDE": "ONLY",
                "CMAKE_FIND_ROOT_PATH_MODE_PACKAGE": "ONLY"
            }
        }
    ]
}
```

这里面提供了五种配置。命令：

```
mkdir build && cd build
cmake ..  --preset=default
cmake ..  --preset=default-release
cmake ..  --preset=build_onwindows_to_arm64
cmake ..  --preset=traditional
cmake ..  --preset=no_ndk
# 编译
ninja  #在build.ninja目录下执行
ninja -C D:/workspace/build/securitychipca  #指定build.ninja路径
```

default和traditional是在Ubuntu环境执行。traditional使用的传统手动指定工具链文件的方式。

build_onwindows_to_arm64在windows上执行。也是在windows上下载NDK和cmake。

最后一种在没有NDK的条件下编译项目。相当于前面几个示例的方案。你要自己搞定crtbegin_dynamic这些。麻烦不推荐。

下面对主要的参数做下说明：

```
{
    "version": 6,
    "configurePresets": [
        {
            "name": "default",
            "generator": "Ninja",
            "binaryDir": "${sourceDir}/build",
            "cacheVariables": {
                // 现代用法，这些是cmake里定义的变量，不要手动设置 CMAKE_TOOLCHAIN_FILE，新版 CMake根据CMAKE_SYSTEM_NAME自动识别工具链
                "CMAKE_SYSTEM_NAME": "Android", 
                "CMAKE_ANDROID_ARCH_ABI": "arm64-v8a",
                "CMAKE_SYSTEM_VERSION": "30",
                "CMAKE_ANDROID_NDK": "${ANDROID_NDK}",

                // 传统，手动指定TOOLCHAIN file, ANDROID_ABI等是NDK里定义的变量，不依赖cmake版本
                "CMAKE_TOOLCHAIN_FILE": "${ANDROID_NDK}/build/cmake/android.toolchain.cmake",
                "ANDROID_ABI": "arm64-v8a",
                "ANDROID_PLATFORM": "android-30",
                "ANDROID_NDK": "${ANDROID_NDK}",

                // 链接可执行文件的参数。CMAKE_SHARED_LINKER_FLAGS则用于动态库
                "CMAKE_EXE_LINKER_FLAGS": "-fuse-ld=lld"

                // 非必须
                "CMAKE_TRY_COMPILE_TARGET_TYPE": "STATIC_LIBRARY",
                "CMAKE_FIND_ROOT_PATH_MODE_PROGRAM": "NEVER",
                "CMAKE_FIND_ROOT_PATH_MODE_LIBRARY": "ONLY",
                "CMAKE_FIND_ROOT_PATH_MODE_INCLUDE": "ONLY",
                "CMAKE_FIND_ROOT_PATH_MODE_PACKAGE": "ONLY"
            }
        }
    ]
}
```

使用CMakePresets.json的好处是，现代IDE都支持，方便IDE解析项目。而前面的方法依赖先执行脚本或命令行传参，手动命令行编译没问题，但是IDE打开项目时,如何先执行脚本或者给cmake命令加参数，是个问题。IDE多多少少提供一些配置的方法，但是不同IDE不统一，学习成本高。配置一般也比较麻烦，而且每个人下代码后都要在自己电脑上配置一遍，浪费时间，不确定因素多。

### CMakeUserPresets.json

每个人的本地环境多多少少有点差异，因此下载到本地后，需要做一些调整。但是如果修改了CMakePresets.json，就会出现在git diff里,每次git commit要小心翼翼排除。可以再创建一个CMakeUserPresets.json，将这个文件添加到.gitignore里。内容：

```
{
    "version": 6,
    "configurePresets": [
        {
            "name": "local-configure",
            "inherits": "build_onwindows_to_arm64",
            "binaryDir": "${sourceDir}/build",
            "environment": {
                "ANDROID_NDK_HOME": "D:/Android/android-sdk/ndk/30.0.14904198",
                "CMAKE_PATH": "D:/Android/android-sdk/cmake/4.1.2",
                "ANDROID_ROOT": "D:/workspace/24mmt2_root"
            },
            "cacheVariables": {
                "CMAKE_EXPORT_COMPILE_COMMANDS": "ON"
            }
        }
    ]
}
```

"inherits": "build_onwindows_to_arm64"继承CMakePresets.json的build_onwindows_to_arm64配置，然后添加差异的部分。比如通常cmake路径、ndk路径不一样，写在这里覆盖build_onwindows_to_arm64里的。

### CMakeLists.txt

在目录创建CMakePresets.json相同目录创建CMakeLists.txt文件，把各个模块加进来（CMakePresets.json和CMakeLists.txt必须在一个目录）

```
cmake_minimum_required(VERSION 3.25)
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)
project(SecurityChipCA VERSION 1.0.0 LANGUAGES CXX C)

if(DEFINED ENV{ANDROID_ROOT} AND NOT "$ENV{ANDROID_ROOT}" STREQUAL "")
    set(ANDROID_ROOT "$ENV{ANDROID_ROOT}")
    # 规范cmake风格路径
    file(TO_CMAKE_PATH "${ANDROID_ROOT}" ANDROID_ROOT)
    get_filename_component(ANDROID_ROOT "${ANDROID_ROOT}" ABSOLUTE)
endif()
if(NOT ANDROID_ROOT)
    # 逐层向上定位aosp根目录
    set(SEARCH_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    while(TRUE)
        if(EXISTS "${SEARCH_DIR}/build/envsetup.sh")
            set(ANDROID_ROOT "${SEARCH_DIR}")
            break()
        endif()
        get_filename_component(PARENT_DIR "${SEARCH_DIR}" DIRECTORY)
        if(PARENT_DIR STREQUAL SEARCH_DIR)
            message(FATAL_ERROR "ANDROID_ROOT not set")
        endif()
        set(SEARCH_DIR "${PARENT_DIR}")
    endwhile()
endif()
# 或手动指定
#set(ANDROID_ROOT "/home/xxx/24mm_t2")
message(STATUS "ANDROID_ROOT_DIR = ${ANDROID_ROOT}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__ANDROID_VENDOR__")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DANDROID")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__ANDROID_VENDOR__")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DANDROID")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -nostdlibinc")

add_subdirectory(../qsee_api ${CMAKE_CURRENT_BINARY_DIR}/qsee_api)
add_subdirectory(../libhsckteec ${CMAKE_CURRENT_BINARY_DIR}/libhsckteec)
add_subdirectory(../hal/default ${CMAKE_CURRENT_BINARY_DIR}/hal_default)
```

add_subdirectory只搜索加进来的目录的CMakeLists.txt文件，不会遍历子目录。

下面是各个模块的CMakeLists文件：

#### qsee_api:

```
cmake_minimum_required(VERSION 3.25)
project(libhsaeQteeIpc)

list(APPEND
    SOURCE_FILES
    hsae_qtee_ipc.c
)
include_directories(SYSTEM 
    "${ANDROID_ROOT}/out/soong/.intermediates/bionic/libc/libc/android_vendor.32_arm64_armv8-a_shared/gen/include"
    "${ANDROID_ROOT}/bionic/libc/kernel/uapi"
    "${ANDROID_ROOT}/bionic/libc/kernel/android/scsi"
    "${ANDROID_ROOT}/bionic/libc/kernel/android/uapi"
    "${ANDROID_ROOT}/bionic/libc/kernel/uapi/asm-arm64"
)

include_directories( 
    "./include"
    "${ANDROID_ROOT}/vendor/qcom/proprietary/securemsm/QSEEComAPI"
    "${ANDROID_ROOT}/system/memory/libion/kernel-headers"
    "${ANDROID_ROOT}/out/soong/.intermediates/kernel/msm-5.4/qti_generate_kernel_headers_arm64/gen"
    "${ANDROID_ROOT}/vendor/qcom/proprietary/common/inc"
    "${ANDROID_ROOT}/external/libcxx/include"
    "${ANDROID_ROOT}/external/libcxxabi/include"
    "${ANDROID_ROOT}/system/logging/liblog/include_vndk"
    "${ANDROID_ROOT}/system/memory/libion/include"
    "${ANDROID_ROOT}/system/memory/libion/kernel-headers"
)

add_library(hsaeQteeIpc STATIC ${SOURCE_FILES})


target_link_directories(hsaeQteeIpc PRIVATE
    ${ANDROID_ROOT}/out/target/product/msmnile_au/system/apex/com.android.runtime/lib64/bionic
    ${ANDROID_ROOT}/out/target/product/msmnile_au/system/lib64
    ${ANDROID_ROOT}/out/target/product/msmnile_au/vendor/lib64
)
target_link_libraries(hsaeQteeIpc
    QSEEComAPI ion
)
```

#### libcryptoauth:

```
project(libcryptoauth VERSION 0.1.0 LANGUAGES C CXX)

link_directories(
    ${ANDROID_ROOT}/out/target/product/msmnile_au/system/apex/com.android.runtime/lib64/bionic
    ${ANDROID_ROOT}/out/target/product/msmnile_au/system/lib64
    ${ANDROID_ROOT}/out/target/product/msmnile_au/vendor/lib64
)

set (SRC
    src/pkcs11_api.c
    src/ck_debug.c
    src/ck_helpers.c
    src/ck_invoke_ta.c
    src/pkcs11_processing.c
    src/pkcs11_token.c
    src/serializer.c
    src/serialize_ck.c
)
# libcryptoauth.so
add_library (cryptoauth SHARED ${SRC})
target_compile_definitions(cryptoauth PRIVATE ANDROID_BUILD)

include_directories(SYSTEM
    "${ANDROID_ROOT}/out/soong/.intermediates/bionic/libc/libc/android_vendor.32_arm64_armv8-a_shared/gen/include"
    "${ANDROID_ROOT}/bionic/libc/kernel/uapi"
    "${ANDROID_ROOT}/bionic/libc/kernel/android/scsi"
    "${ANDROID_ROOT}/bionic/libc/kernel/android/uapi"
    "${ANDROID_ROOT}/bionic/libc/kernel/uapi/asm-arm64"
)
include_directories(
    "./include"
    "../qsee_api/include"
    "${ANDROID_ROOT}/external/googletest/googletest/include"
    "${ANDROID_ROOT}/external/googletest/googlemock/include"
    "${ANDROID_ROOT}/external/libcxxabi/include"
    "${ANDROID_ROOT}/system/logging/liblog/include_vndk"
    "${ANDROID_ROOT}/system/memory/libion/include"
    "${ANDROID_ROOT}/system/memory/libion/kernel-headers"
    "${ANDROID_ROOT}/system/libbase/include"
    "${ANDROID_ROOT}/external/fmtlib/include"
    "${ANDROID_ROOT}/external/libcxx/include"
)
target_link_libraries (cryptoauth PUBLIC
    log ion hsaeQteeIpc
)

# 单元测试程序
set(UT_SRC
    ${SRC}
    test/pkcs11_test.cpp
)
add_executable(securitychip_pkcs11_ut ${UT_SRC})
# gtest
target_link_options(securitychip_pkcs11_ut PRIVATE
    ${ANDROID_ROOT}/out/soong/.intermediates/external/googletest/googlemock/libgmock/android_vendor.32_arm64_armv8-a_static/libgmock.a
    ${ANDROID_ROOT}/out/soong/.intermediates/external/googletest/googletest/libgtest/android_arm64_armv8-a_static/libgtest.a
    ${ANDROID_ROOT}/out/soong/.intermediates/external/googletest/googletest/libgtest_main/android_arm64_armv8-a_static/libgtest_main.a
)
target_include_directories(securitychip_pkcs11_ut SYSTEM PRIVATE # 三方头文件尽量加上SYSTEM以抑制警告。
    "${ANDROID_ROOT}/external/googletest/googletest/include"
    "${ANDROID_ROOT}/external/googletest/googlemock/include"
)
target_link_libraries (securitychip_pkcs11_ut PRIVATE
    log ion hsaeQteeIpc c++
)

# test_integration测试程序
set(TEST_SRC
    test_integration/pkcs11_test.cpp
)
add_executable(pkcs11_client_test ${TEST_SRC})
target_include_directories(pkcs11_client_test PRIVATE
    "./test_integration"
    "${ANDROID_ROOT}/system/core/libcutils/include_outside_system"
    "${ANDROID_ROOT}/system/core/libprocessgroup/include"
    "${ANDROID_ROOT}/system/core/libcutils/include"
    "${ANDROID_ROOT}/system/core/libutils/include"
    "${ANDROID_ROOT}/system/unwinding/libbacktrace/include"
    "${ANDROID_ROOT}/system/core/libsystem/include"
    "${ANDROID_ROOT}/vendor/hsae/proprietary/hardware/libp11/third_party/openssl/include"
)

target_link_libraries (pkcs11_client_test PRIVATE
    log cutils base utils crypto_ttrs p11 c++
)
```

#### securitychip hal

```cmake
cmake_minimum_required(VERSION 3.16)
project(vendor.iauto.hardware.securitychip@1.0-service)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -nostdlibinc")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")

link_directories(
    ${ANDROID_ROOT}/out/target/product/msmnile_au/system/apex/com.android.runtime/lib64/bionic
    ${ANDROID_ROOT}/out/target/product/msmnile_au/system/lib64
    ${ANDROID_ROOT}/out/target/product/msmnile_au/vendor/lib64
)

list(APPEND COMMON_FILES
    HsmManager.cpp
    TA100Common.cpp
    verify.cpp
    optee_ipc.cpp
)
list(APPEND SOURCE_FILES
    ${COMMON_FILES}
    service.cpp
)
add_executable(vendor.iauto.hardware.securitychip-1.0-service ${SOURCE_FILES})
set_target_properties(vendor.iauto.hardware.securitychip-1.0-service
    PROPERTIES
    OUTPUT_NAME "vendor.iauto.hardware.securitychip@1.0-service"
    PREFIX ""
)
target_compile_options(vendor.iauto.hardware.securitychip-1.0-service PRIVATE
    -Wall
    -Wextra
    -Wreturn-type
    -Wno-unused-parameter
)
include_directories(SYSTEM
    "${ANDROID_ROOT}/out/soong/.intermediates/bionic/libc/libc/android_vendor.32_arm64_armv8-a_shared/gen/include"
    "${ANDROID_ROOT}/bionic/libc/kernel/uapi"
    "${ANDROID_ROOT}/bionic/libc/kernel/android/scsi"
    "${ANDROID_ROOT}/bionic/libc/kernel/android/uapi"
    "${ANDROID_ROOT}/bionic/libc/kernel/uapi/asm-arm64"
)
include_directories(
    "./include"
    "."
    "../../qsee_api/include"
    "${ANDROID_ROOT}/external/libcxxabi/include"
    "${ANDROID_ROOT}/system/libbase/include"
    "${ANDROID_ROOT}/external/fmtlib/include"
    "${ANDROID_ROOT}/system/libhidl/base/include"
    "${ANDROID_ROOT}/system/libhidl/transport/include"
    "${ANDROID_ROOT}/system/libfmq/base"
    "${ANDROID_ROOT}/system/libhwbinder/include"
    "${ANDROID_ROOT}/system/core/libutils/include"
    "${ANDROID_ROOT}/system/unwinding/libbacktrace/include"
    "${ANDROID_ROOT}/system/logging/liblog/include_vndk"
    "${ANDROID_ROOT}/system/core/libsystem/include"
    "${ANDROID_ROOT}/system/core/libcutils/include_outside_system"
    "${ANDROID_ROOT}/system/core/libprocessgroup/include"
    "${ANDROID_ROOT}/system/core/libcutils/include"
    "${ANDROID_ROOT}/out/soong/.intermediates/system/libhidl/transport/manager/1.0/android.hidl.manager@1.0_genc++_headers/gen"
    "${ANDROID_ROOT}/out/soong/.intermediates/system/libhidl/transport/manager/1.1/android.hidl.manager@1.1_genc++_headers/gen"
    "${ANDROID_ROOT}/out/soong/.intermediates/system/libhidl/transport/manager/1.2/android.hidl.manager@1.2_genc++_headers/gen"
    "${ANDROID_ROOT}/out/soong/.intermediates/system/libhidl/transport/base/1.0/android.hidl.base@1.0_genc++_headers/gen"
    "${ANDROID_ROOT}/hardware/libhardware/include"
    "${ANDROID_ROOT}/system/media/audio/include"
    "${ANDROID_ROOT}/system/bt/types"
    "${ANDROID_ROOT}/external/boringssl/src/include"
    "${ANDROID_ROOT}/system/memory/libion/include"
    "${ANDROID_ROOT}/system/memory/libion/kernel-headers"
    "${ANDROID_ROOT}/out/soong/.intermediates/vendor/iauto/hardware/interfaces/securitychip/1.0/vendor.iauto.hardware.securitychip@1.0_genc++_headers/gen"
    "${ANDROID_ROOT}/out/soong/.intermediates/vendor/hsae/proprietary/hardware/securityta100/CA/hal/interfaces/1.0/vendor.hsae.hardware.ta100@1.0_genc++_headers/gen"
    "${ANDROID_ROOT}/external/libcxx/include"
)
target_link_libraries(vendor.iauto.hardware.securitychip-1.0-service
    c++ base hidlbase log cutils utils 
    hidltransport hardware crypto QSEEComAPI ion hsaeQteeIpc
    :vendor.iauto.hardware.securitychip@1.0.so
    :vendor.hsae.hardware.ta100@1.0.so
)

# 单元测试程序
set(UT_SRC
    ${COMMON_FILES}
    ./test/HsmManager_test.cpp
    ./test/TA100Common_test.cpp
    ./test/TestConfig.cpp
)
add_executable(securitychip_hal_ut ${UT_SRC})
# 链接gtest
target_link_options(securitychip_hal_ut PRIVATE
    ${ANDROID_ROOT}/out/soong/.intermediates/external/googletest/googlemock/libgmock/android_vendor.32_arm64_armv8-a_static/libgmock.a
    ${ANDROID_ROOT}/out/soong/.intermediates/external/googletest/googletest/libgtest/android_arm64_armv8-a_static/libgtest.a
    ${ANDROID_ROOT}/out/soong/.intermediates/external/googletest/googletest/libgtest_main/android_arm64_armv8-a_static/libgtest_main.a
)
target_include_directories(securitychip_hal_ut PRIVATE
    "${ANDROID_ROOT}/external/googletest/googletest/include"
    "${ANDROID_ROOT}/external/googletest/googlemock/include"
)
target_link_libraries (securitychip_hal_ut PRIVATE
    c++ base hidlbase log cutils utils 
    hidltransport hardware crypto QSEEComAPI ion hsaeQteeIpc
    :vendor.iauto.hardware.securitychip@1.0.so
    :vendor.hsae.hardware.ta100@1.0.so
)
target_compile_options(securitychip_hal_ut PRIVATE
    -Wall
    -Wextra
    -Wreturn-type
    -Wno-unused-parameter
)

# test_integration测试程序
set(TEST_SRC
    test_integration/HsmManagerListener.cpp
    test_integration/test.cpp
)
add_executable(securitychip_hal_client_test ${TEST_SRC})
target_include_directories(securitychip_hal_client_test PRIVATE
    "./test_integration"
    "${ANDROID_ROOT}/system/core/libcutils/include_outside_system"
    "${ANDROID_ROOT}/system/core/libprocessgroup/include"
    "${ANDROID_ROOT}/system/core/libcutils/include"
    "${ANDROID_ROOT}/system/core/libutils/include"
    "${ANDROID_ROOT}/system/unwinding/libbacktrace/include"
    "${ANDROID_ROOT}/system/core/libsystem/include"
    "${ANDROID_ROOT}/vendor/hsae/proprietary/hardware/libp11/third_party/openssl/include"
)

target_link_libraries (securitychip_hal_client_test PRIVATE
    log cutils base utils c++ hidltransport hardware hidlbase
    :vendor.iauto.hardware.securitychip@1.0.so
    :vendor.hsae.hardware.ta100@1.0.so
)
```

tips:

> 1、关于SYSTEM 
> 
> target_include_directories(test PRIVATE
>     external/googletest/include
> )
> 
> 生成命令：-Iexternal/googletest/include  。（大写的i，不是L）
> 
> 而
> 
> target_include_directories(test SYSTEM PRIVATE
>     external/googletest/include
> )
> 
> 生成命令：-isystem external/googletest/include
> 
> `-I` → gtest 被当“自己代码” → warning 全开
> 
> `-isystem` → gtest 被当“外部库” → warning 被压制，include search priority 更低
> 
> 2、许多功能有多种实现方式，尽量用常规的。
> 
> 优先用target_compile_options而不是
> 
> set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -xxxx")
> 
> 优先target_link_directories而不是全局的link_directories

### 保持和mm一致

clang版本有些差异一般问题不大，主要是编译器参数可能和原生mm编译有区别, 正常不会有问题，差异点包括：是否允许某些C++高级特性(比如try-catch); 警告是否当错误报出。如果你追求严谨，那么有一些方法修改的和Android.bp编译一致。

思路1、 基于 SOONG_GEN_CMAKEFILES 生成的 CMakeLists.txt

```
export SOONG_GEN_CMAKEFILES=1
export SOONG_GEN_CMAKEFILES_DEBUG=1
// mm后会在out/development/ide/clion目录生成。
```

把这个文件里的`set(CMAKE_CXX_FLAGS xxx`全部拷贝到自己写的CMakeLists.txt，然后调试一下，根据信息做适量调整。

思路2、 将 out目录`verbose.log*` 文件全部删除，然后`find . | xargs touch`更新项目代码的时间戳，mm重新编译。然后会在out生成verbose.log.*.gz文件，里面有mm编译过程中执行的每一条指令。和我们自己的 CMakeLists.txt 生成的compile_commands.json对比，即可以知道全部差异。在我们自己的CMakeLists.txt里，把差异通过flag加上。

### IDE导入和调试

#### 导入

如果cmake命令编译能成功，那么IDE也不会有问题，因为IDE也是通过解析cmakelists文件配置项目。打开后，指定CMakeLists.txt文件，指定目标preset。如果提示选择编译器的话，选择cmake里我们自己配的，而不是插件扫描出的系统里的。当然选其他的一般也没事：

![image-20260615120636026](./imgs/基于cmake编译安卓native程序/image-20260615120636026.png)

以vscode为例，安装ms cmake插件、安装clangd插件、下载clangd二进制并确保vscode clangd插件能找到它（环境配置不细述）。禁用微软的那个C++插件。

然后要让VSCode能正确识别项目，还需要：

```
1、生成compile_commands.json文件
"cacheVariables": {
    "CMAKE_EXPORT_COMPILE_COMMANDS": "ON"
}
2、settings.json指定compile_commands.json
"clangd.arguments": [
    "--compile-commands-dir=doc/build"
]
```

这其实不是cmake插件的问题，而是clangd插件的需求。智能提示、代码分析、调转等功能是clangd提供的。主要是vscode开发C++依赖插件。

更完整的IDE 如clion，目测没这个问题，也不需要你去装什么插件。2025年CLion已经提供免费版。

CMake里的构建目标，IDE界面上都有，直接通过点点就能生成目标，不再需要命令行。

#### 调试

下面介绍如何用VSCode进行debug。由于gdb调试，以前已经写了不少这方面的笔记，而且谷歌正在放弃gdb拥抱llvm,所以这里就介绍用lldb。

1. 安装插件：CodeLLDB。另外一个插件“C/C++ Debug”应该也可以，不过我这里就用CodeLLDB，将“C/C++ Debug”禁用。

2. 为了能找到lldb工具，将“ndk\30.0.14904198\toolchains\llvm\prebuilt\windows-x86_64\bin”添加到系统PATH。另外，系统里如果其它软件装了乱七八糟的python，也可能造成一定污染，但是总的来说影响不大。

3. 打开cmd，执行lldb,看能不能进入lldb的控制台。如果不能，那说明环境没配好。vscode自然也不可能正确调试。

4. vscode, 在“.vscode”目录创建：“launch.json”。根据界面引导自动创建的，不能满足我们，所以自己创建，或者在已有的基础上改。内容如下：
   
   ```
   {
       "version": "0.2.0",
       "configurations": [
           {
               "name": "Debug securitychip_pkcs11_ut on device",
               "type": "lldb",
               "request": "launch",
               "program": "${workspaceFolder}/doc/build/libhsckteec/securitychip_pkcs11_ut",
               "preLaunchTask": "securitychip_pkcs11_ut_prepared", //debug前先执行这个task
               "initCommands": [
                   "platform select remote-android",
                   "platform connect connect://localhost:1234",
                   "platform settings -w /data/local/tmp" , // 必须指定工作目录，否则提示readonly权限不足
                   "target create /data/local/tmp/securitychip_pkcs11_ut",
                   "process launch"
               ],
               "args": [
                   "--gtest_filter=PKCS11NeedSessionTest.ObjectTest_a001_label"
               ],
               "stopOnEntry": false
           },
           {
               "name": "Debug vendor.iauto.hardware.securitychip@1.0-service on device",
               "type": "lldb",
               "request": "attach",
               "program": "${workspaceFolder}/doc/build/hal_default/vendor.iauto.hardware.securitychip@1.0-service",
               "preLaunchTask": "securitychip_hal_service_prepared",
               "initCommands": [
                   "platform select remote-android",
                   "platform connect connect://localhost:1234",
                   "platform settings -w /data/local/tmp",
                   "target create /vendor/bin/hw/vendor.iauto.hardware.securitychip@1.0-service"
               ],
               "stopOnEntry": false
           }
       ]
   }
   ```
   
   这里分别演示了launch模式和attach模式。
   
   * launch: 启动目标程序并调试，适用需要在程序入口(如main函数)打断点。
   * attach：attach到已经在运行的程序上。比较适合安卓hal服务。

5. 如果CMakeLists.txt里没有指定相关参数，那么默认cmake可能构建的是Release版本，里面不带debug信息。所以需要：
   
   ```
   "cacheVariables": {
       ...
       // 加上，构建Debug程序。Debug版包含调试符号、无优化。如果用Release,可能出现：
       // 断点错位、变量消失(显示 <optimized out>)、函数内联、代码重排、调用栈不完整...等问题
       // 但是注意即使这里指定里Debug，可能CMakeLists.txt里也指定了如-g3 -O0这类参数，导致程序不能调试。
       "CMAKE_BUILD_TYPE": "Debug"
   }
   ```
   
   这里，我们编译debug版本，程序文件本身已经包含里调试符号，所以launch.json里不需要配置符号表文件位置，直接“target create xxxxx”即可。至于调试符号分离的情况下，lldb命令怎么写，自行百度。

6. 
   > 其实launch模式本来就会自动将securitychip_pkcs11_ut推到/data/local/tmp/securitychip_pkcs11_ut。但是第一次执行时，target create /data/local/tmp/securitychip_pkcs11_ut这行，因为没这个文件会报错。那么我们不要依赖它，写个task自己去push就行了。
   
   在“.vscode”目录创建：“tasks.json”,内容：
   
   ```
   {
       "version": "2.0.0",
       "tasks": [
           {
               "label": "push-lldb-server-windows",
               "type": "shell",
               "command": "adb",  // 将lldb-server推送到设备上
               "args": [
                   "push",
                   "D:/Android/android-sdk/ndk/30.0.14904198/toolchains/llvm/prebuilt/windows-x86_64/lib/clang/21/lib/linux/aarch64/lldb-server",
                   "/data/local/tmp/lldb-server",
                   "&&",
                   "adb",
                   "shell",
                   "chmod",
                   "+x",
                   "/data/local/tmp/lldb-server"
               ],
               "options": {
                   "shell": {
                       "executable": "cmd.exe", // 默认powershell，指定使用cmd
                       "args": ["/d", "/c"]
                   }
               },
               "problemMatcher": []
           },
           {
               "label": "start-lldb-server-windows",
               "type": "shell",  // 启动lldb server，转发断开
               "command": "adb forward tcp:1234 tcp:1234 && adb root && adb shell \"/data/local/tmp/lldb-server platform --listen *:1234 --server > /dev/null 2>&1 &\"",
               "dependsOn": ["push-lldb-server-windows"],
               "options": {
                   "shell": {
                       "executable": "cmd.exe",
                       "args": ["/d", "/c"]
                   }
               }
           },
           {
               "label": "push-lldb-server-linux",
               "type": "shell",
               "command": "adb",
               "args": [
                   "push",
                   "/workspace/xuexiangyu/Android/Sdk/ndk/30.0.14904198/toolchains/llvm/prebuilt/linux-x86_64/lib/clang/21/lib/linux/aarch64/lldb-server",
                   "/data/local/tmp/lldb-server",
                   "&&",
                   "adb",
                   "shell",
                   "chmod",
                   "+x",
                   "/data/local/tmp/lldb-server"
               ],"options": {
                   "shell": {
                       "executable": "/bin/bash",
                        "args": ["-c"]
                   }
               },
               "problemMatcher": []
           },
           {
               "label": "start-lldb-server-linux",
               "type": "shell",  // 启动lldb server，转发断开
               "command": "adb forward tcp:1234 tcp:1234 && adb root && adb shell \"/data/local/tmp/lldb-server platform --listen *:1234 --server > /dev/null 2>&1 &\"",
               "dependsOn": ["push-lldb-server-linux"],
               "options": {
                   "shell": {
                       "executable": "/bin/bash",
                       "args": ["-c"]
                   }
               }
           },
           {
               "label": "push-securitychip_pkcs11_ut",
               "type": "cmake",               // 使用 CMake 任务类型
               "command": "build",            // 执行构建命令
               "targets": [                   // 指定要构建的目标，即 CMakeLists.txt 中add_custom_target定义的名字
                   "push-securitychip_pkcs11_ut"
               ]
           },
           {
               "label": "push-securitychip-hal-service",
               "type": "cmake",               // 使用 CMake 任务类型
               "command": "build",            // 执行构建命令
               "targets": [                   // 指定要构建的目标，即 CMakeLists.txt 中add_custom_target定义的名字
                   "push-securitychip-hal-service"
               ]
           },
           {
               "label": "securitychip_pkcs11_ut_prepared",
               "dependsOrder": "sequence", //顺序执行，默认是并行执行
               "dependsOn": ["push-lldb-server-linux", "start-lldb-server-linux", "push-securitychip_pkcs11_ut"],
           },
           {
               "label": "securitychip_hal_service_prepared",
               "dependsOrder": "sequence", //顺序执行，默认是并行执行
               "dependsOn": ["push-lldb-server-linux", "start-lldb-server-linux", "push-securitychip-hal-service"],
           }
       ]
   }
   ```
   
   你会发现tasks里写复杂的shell比较难，可以放到单独的sh文件里，task去调sh脚本文件，这样比较方便。或者用cmake自定义target也比vscode task方便。
   这个tasks做了几件事：
   
   1、将ndk里的lldb-server推到车机里。因为比较老的安卓版本用的是gdbserver，没有lldb-server,故需要手动准备。这个脚本不完善地方在于，每次都会执行一次push。实际只要push一次，保证文件存在就行。但无伤大雅。如果脚本里先去判断文件存在的话，逻辑太复杂，反而容易出问题。
   
   2、启动lldb-server监听端口1234。以及adb端口转发：adb forward tcp:1234 tcp:1234。同样只要执行一次。每次执行反正也没影响。
   
   3、将带调试的问题securitychip_pkcs11_ut，推送到车机。这里我没有选择在tasks里实现，而是在cmake里加了一个：
   
   ```
   // CMakeLists.txt
   # push securitychip_pkcs11_ut
   add_custom_target(push-securitychip_pkcs11_ut
       COMMAND adb push $<TARGET_FILE:securitychip_pkcs11_ut> /data/local/tmp/securitychip_pkcs11_ut
       COMMAND adb shell chmod 755 /data/local/tmp/securitychip_pkcs11_ut
       DEPENDS securitychip_pkcs11_ut
       COMMENT "Push securitychip_pkcs11_ut to Android device"
   )
   ```
   
   然后tasks.json里：
   
   ```
   {
       "label": "push-securitychip_pkcs11_ut",
       "type": "cmake",               // 使用 CMake 任务类型
       "command": "build",            // 执行构建命令
       "targets": [                   // 指定要构建的目标，即 CMakeLists.txt 中add_custom_target定义的名字
           "push-securitychip_pkcs11_ut"
       ]
   }
   ```
   
   只要装了cmake插件，就支持type cmake。对比一下我上面的推送lldb-server的代码，就知道cmake里实现COMMAND毕竟比json里去写更加方便。而且可以通过`DEPENDS securitychip_pkcs11_ut`，保证debug前先构建最新的二进制。

#### 遇到的问题

* AI让我在initCommands里在加一行"process attach -n vendor.iauto.hardware.securitychip@1.0-service"，实测不能加。

* 如果机器里待调试的文件，不是把本地编译出的，比如可能是在服务器上编译的。那么，符号表里的源文件路径，就会和本地源代码路径不一致。需要指定sourceMap
  
  ```
  "sourceMap": {
      // 远程路径 -> 本地路径映射
  },
  ```
  
  我这个项目是cmake本地编的，不需要配sourceMap。

* 这个警告是因为依赖的系统库没有符号表。我们只调试自己的代码，无视这个警告即可。
  
  ```
  warning: (aarch64) C:\Users\xuexiangyu\.lldb\module_cache\remote-android\.cache\F23D23E9-0DF3-EFE8-A2EC-F3F447FFFE11\libcgrouprc.so No LZMA support found for reading .gnu_debugdata section
  warning: (aarch64) C:\Users\xuexiangyu\.lldb\module_cache\remote-android\.cache\DE954B65-7FC5-0EC8-7205-88C65F6426C0\libnetd_client.so No LZMA support found for reading .gnu_debugdata section
  ```

* 一直等待某个task完成，时间长了后弹出提示，说任务还在执行是否继续debug。这是因为task里指定里background：true。后台任务即使完成了也不知道。
  
  `adb shell "/data/local/tmp/lldb-server platform --listen *:1234 --server &"`虽然后台进程启动了,但 `adb` 会保持连接直到所有**后台进程的标准输出/错误**被关闭,所以第一次执行会阻塞。必须加重定向输出`adb shell "/data/local/tmp/lldb-server platform --listen *:1234 --server > /dev/null 2>&1 &"`或者加nohup:`adb shell "nohup /data/local/tmp/lldb-server platform --listen *:1234 --server &"`

* "platform settings -w /data/local/tmp"必须指定工作目录，否则提示readonly权限不足。且在platform connect语句后面。

* "launch"模式，启动的程序，命令行参数不能直接写在程序名后面，要通过args指定。

* 没有考虑结束lldb-server。无伤大雅，能用就行。可手动kill。

* 一些和python有关的警告。大多是环境里有其它python。检查环境变量确保用ndk里的python即可。也可以在json里指定：
  
  ```
  "stopOnEntry": true,
  "env": {
      "PYTHONPATH": "D:\\xxx\\xxx\\Lib;D:\\xxxx\\xxxx\\DLLs"
  }
  ```
  
  由AI提供，未验证。

### 问题

1、起初，期望的方式是：在windows上映射网络驱动器，直接打开网络盘，实现在Windows上开发和编译。然而实测下来，理想丰满现实骨感。这种方式过程中遇到许多和映射磁盘相关的问题。所以尽量避免这种方式。如果非要用，尽量samba，不要sshfs。无论是性能，还是潜在的bug，samba都比sshfs好很多。下面记录一些遇到的问题。

* sshfs映射的磁盘，ninja处理带windows盘符的路径(如 S:/xxxx/aosp/xxx/file1.c)有问题,报文件不存在。比较奇怪的是samba映射的本地磁盘就没这个问题。直接用`\\192.168.66.252\xxxx\aosp\xxxx`也支持，甚至把反斜杠换成正斜杠也是OK的。

* samba映射：cmake过程中 “The CXX compiler identification is unknown”，然后执行ninja命令的过程中报错。把项目代码下载到本地则构建成功。将clang目录添加PATH也是一样。比较诡异，猜测大概率是以下原因之一：
  
  1）网络盘权限或同步有问题；
  
  2）网络延迟可能导致 CMake 失败；
  
  3）网络盘不是NTFS文件系统，不支持符号链接及其它文件系统提供的特性；
  
  4）**网络路径兼容性**：CMake 对网络路径或 UNC 路径的支持可能不完整
  
  解决这个问题比较简单，指定binaryDir为本地磁盘路径即可。让cmake生成物不要写到网络盘上。
  
  但这时奇诡的事情再次发生了，如果我指定：`"binaryDir": "D:/workspace/output"`，会触发一个新问题：output目录下的生成物缺少CMakeCache.txt文件，ninja命令一执行就报错。
  
  如果我指定：`"binaryDir": "D:/workspace/build"`，则顺利编译通过。难道output和build这两个名字还有区别吗。

* 编译巨慢。分两个阶段：
  
  1）cmake命令，生成中间文件、cache文件和build.ninja文件。是大量的小文件，这种非常受到网络延迟的影响。
  
  ​      解决方法：指定binaryDir为本地磁盘路径。避免频繁写网络盘。节省了十几分钟。
  
  2）执行ninja命令，真正编译代码。
  
  ​      binaryDir只解决了cmake命令阶段的速度问题。而执行ninja命令真正编译时，大量的include文件和链接库依然要建立网络请求。没太好的方法，让AI写个python脚本，解析CMakeLists.txt中的所有网络盘路径，把所有include目录、链接库所有目录都下载到本地。如果没有AI介入，这会消耗你很多时间，不推荐。每次，当依赖别人的库有更新时，都要重新下载。
  
  （脚本附在文末）

* 将整个项目代码下载到本地，依赖的库、头文件使用网络盘，也可以只把CMakeLists.txt、CMakePresets.json两个下载到windows上。没问题但没有意义，也不能提升编译速度。且可能导致有些头文件因include两次而报错。
  
  > tips: `#pragma once`依赖的是**文件路径**来判断是否重复。同一个头文件通过**不同的路径**被包含，编译器会认为它们是不同的文件。比如下载到本地后，本地有一份，远程目录也有一份，通过编译器参数同时被include了。或者同一个文件，通过路径引用一次，通过符号链接、网络驱动映射引用一次，那么路径不一样，`#pragma once`不能识别是相同头文件。
  > 
  > 传统的添加头文件保护宏方式没这个问题：
  > 
  > ```
  > 尽量用：
  > #ifndef DEDICATED_MEMORY_H
  > #define DEDICATED_MEMORY_H
  > 而不是：
  > #pragma once
  > 
  > #endif // DEDICATED_MEMORY_H
  > ```

* 总之最好还是避开任何的 windows编译、磁盘映射，潜在的坑比较多。即使我现在把问题都解决了，将来或许又会出现新的问题。Ubuntu上十几秒编完，兼容稳定，没理由非要在wndows上搞。对我来说，唯一的理由就是，有些IDE不支持ssh远程连接，必须要映射到本地驱动器。

2、环境变量污染。`PATH=D:\mingw64\bin;xxx`污染了NDK环境。去掉MSYS2/MinGW环境即可。

3、 link_directories官方不推荐用。但就我这个例子而言，省去了写多个相同的target_link_directories。link_directories必须放在add_library、add_executable前面。

4、系统存在多个CMake时，确保用的是ANDROID SDK里的cmake。

5、用NDK编译的坏处是，NDK里的clang版本和AOSP里的不一致，版本差异较大时可能存在源码环境mm能编过，用ndk的clang不能编过的情况。可以通过在CMakeLists.txt里重新设定CMAKE_C_COMPILER解决。

## 自动生成

1、自动拷贝到本地的脚本(AI生成)

```python
#!/usr/bin/env python3
import os
import re
import shutil

# CMakeLists文件路径 - 在此添加更多文件
CMAKE_FILES = [
    r"d:\workspace\24mmt2\hardware\securityta100\CA\qsee_api\CMakeLists.txt",
    r"d:\workspace\24mmt2\hardware\securityta100\CA\hal\default\CMakeLists.txt",
    r"d:\workspace\24mmt2\hardware\securityta100\CA\libhsckteec\CMakeLists.txt",
]

# ANDROID_ROOT原始路径
ANDROID_ROOT = r"W:/24mm_t2/apps/LINUX/android"

# 目标目录
TARGET_ROOT = r"D:\workspace\24mmt2_root"

def extract_android_root_paths(cmake_file):
    """从CMakeLists.txt文件中提取所有${ANDROID_ROOT}路径"""
    paths = []
    with open(cmake_file, 'r', encoding='utf-8', errors='ignore') as f:
        content = f.read()

    # 匹配 ${ANDROID_ROOT}/xxx 的模式
    pattern = r'\$\{ANDROID_ROOT\}/([^\s"\'\\]+)'
    matches = re.findall(pattern, content)

    for match in matches:
        full_path = os.path.join(ANDROID_ROOT, match)
        paths.append(full_path)

    return paths

def copy_files_to_target(source_paths, target_root):
    """将源路径复制到目标目录，保留目录结构"""
    copied_count = 0
    skipped_count = 0

    for source_path in source_paths:
        # 将路径转换为Windows格式
        source_path = source_path.replace('/', '\\')

        if not os.path.exists(source_path):
            print(f"跳过不存在的路径: {source_path}")
            skipped_count += 1
            continue

        # 计算相对路径（相对于ANDROID_ROOT）
        relative_path = os.path.relpath(source_path, ANDROID_ROOT.replace('/', '\\'))
        target_path = os.path.join(target_root, relative_path)
        target_dir = os.path.dirname(target_path)

        # 确保目标目录存在
        if not os.path.exists(target_dir):
            os.makedirs(target_dir)

        # 复制文件或目录
        if os.path.isfile(source_path):
            shutil.copy2(source_path, target_path)
            print(f"复制文件: {source_path} -> {target_path}")
            copied_count += 1
        elif os.path.isdir(source_path):
            if os.path.exists(target_path):
                shutil.rmtree(target_path)
            shutil.copytree(source_path, target_path)
            print(f"复制目录: {source_path} -> {target_path}")
            copied_count += 1

    return copied_count, skipped_count

# 将项目依赖的文件从网络映射盘复制到windows本地，以提高编译速度。
def main():
    print("=" * 60)
    print("从CMakeLists.txt提取并复制ANDROID_ROOT路径")
    print("=" * 60)

    # 收集所有路径
    all_paths = []
    for cmake_file in CMAKE_FILES:
        print(f"\n正在解析: {cmake_file}")
        paths = extract_android_root_paths(cmake_file)
        all_paths.extend(paths)
        print(f"  找到 {len(paths)} 个路径")

    # 去重
    all_paths = list(set(all_paths))
    print(f"\n去重后共 {len(all_paths)} 个唯一路径")

    # 复制文件
    print("\n开始复制文件...")
    copied, skipped = copy_files_to_target(all_paths, TARGET_ROOT)

    print("\n" + "=" * 60)
    print(f"复制完成!")
    print(f"成功复制: {copied} 个")
    print(f"跳过不存在: {skipped} 个")
    print(f"目标目录: {TARGET_ROOT}")
    print("=" * 60)

if __name__ == "__main__":
    main()
```

2、如果希望写一个脚本自动生成cmake，最好用python。因为shell解析bp文件是非常困难的，而python有一个库可以解析：

```python
# 检测是否安装了android-bp
try:
    from android_bp import BluePrint
except ImportError:
    print("pip3 install android-bp", file=sys.stderr)
    sys.exit(1)
# 读取解析bp文件
try:
    bp = BluePrint.from_file(bp_file)
except:
    print("解析失败", file=sys.stderr)
    sys.exit(1)
# 只关心和c++有关的模块
target = None
for m in bp.modules:
    t = m.__type__
    if t in ["cc_binary", "cc_library", "cc_library_shared", "cc_library_static", "cc_test"]:
        target = m
        break

if not target:
    print("无 C/C++ 模块", file=sys.stderr)
    sys.exit(1)
# 模块名
name = target.name
bp_dir = os.path.dirname(os.path.abspath(bp_file))
rel = os.path.relpath(bp_dir, aosp_root)
# 动态库和静态库
shared = getattr(target, "shared_libs", []) or []
static = getattr(target, "static_libs", []) or []
# 去掉前面的lib, 如libc++在CMakeLists里应写成c++
def fix_lib(l):
    if l.startswith("lib"):
        return l[3:]
    return l

libs = []
for l in shared + static:
    libs.append(fix_lib(l))
libs += ["c", "m", "dl", "c++"] # bp的基础上补充libc、libm、libdl等库
```

问题是，有时我们会提取出一个`cc_defaults`，cc_binary引用cc_defaults。推测android-bp或许无法得到cc_defaults里的shared_libs。因此最终还用人工核查一下。
