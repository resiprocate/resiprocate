### Prerequisites

#### Preparing OpenHarmony SDK 

OpenHarmony provides SDKs for Linux, Windows, and macOS platforms, enabling cross-compilation across these systems. This guide focuses on Linux-based cross-compilation.

1. Download the SDK for your target platform from the [official release channel](https://gitee.com/openharmony/docs/blob/master/en/release-notes/OpenHarmony-v5.0.1-release.md#acquiring-source-code-from-mirrors).

2. Extract the SDK package:

   ```shell
   owner@ubuntu:~/workspace$ tar -zxvf ohos-sdk-windows_linux-public.tar.gz
   ```

3. Navigate to the SDK's Linux directory and extract all toolchain packages: <a id="ohos_sdk"> </a>

   ```shell
   owner@ubuntu:~/workspace$ cd ohos_sdk/linux
   owner@ubuntu:~/workspace/ohos-sdk/linux$ for i in *.zip;do unzip ${i};done
   owner@ubuntu:~/workspace/ohos-sdk/linux$ ls
   total 1228400
   85988 -rw-r--r-- 1 wshi wshi  88050148 Nov 20  2024 ets-linux-x64-5.0.1.111-Release.zip          # ArkTS compiler tools
   56396 -rw-r--r-- 1 wshi wshi  57747481 Nov 20  2024 js-linux-x64-5.0.1.111-Release.zip           # JS compiler tools
   888916 -rw-r--r-- 1 wshi wshi 910243125 Nov 20  2024 native-linux-x64-5.0.1.111-Release.zip      # C/C++ cross-compilation tools
   175084 -rw-r--r-- 1 wshi wshi 179281763 Nov 20  2024 previewer-linux-x64-5.0.1.111-Release.zip   # App preview tools
   22008 -rw-r--r-- 1 wshi wshi  22533501 Nov 20  2024 toolchains-linux-x64-5.0.1.111-Release.zip   # Utilities (e.g., signing tool, device connector)
   ```
Once you extract these 5 archives, the SDK setup will be complete, and its path will be `~/workspace/ohos-sdk/linux`. 


### Compiling resiprocate 1.13 and later versions
Compared to earlier versions, resiprocate 1.13 switched its build system to CMake. The OpenHarmony SDK provides full support for CMake, making cross-compilation tasks easy to accomplish.

### Obtaining the Source Code

Download the [resiprocate-1.13.2 source code package](https://github.com/resiprocate/resiprocate/archive/refs/tags/resiprocate-1.13.2.tar.gz) from GitHub and extract it to `~/workspace/resiprocate`.

```shell
tar -zxvf resiprocate-1.13.2.tar.gz -C ~/workspace/resiprocate
```

### Starting the Compilation

1. Set the SDK path
   ```shell
   owner@ubuntu:~/workspace/resiprocate$ export OHOS_SDK=/home/owner/workspace/ohos-sdk/linux                   ## Configure the SDK path. Replace this with your own SDK decompression directory.  
   owner@ubuntu:~/workspace/resiprocate$ export PATH=$OHOS_SDK/native/build-tools/cmake/bin:$PATH               ## Add the SDK's cmake to the PATH environment variable  
   ```
2. Execute cmake to generate Makefile
   
   By default, the 64-bit version is compiled. If the 32-bit version is needed, set `OHOS_ARCH` to `armeabi-v7a`. `CMAKE_INSTALL_PREFIX` is set to `/home/owner/install/resiprocate`, which is the installation directory for cross-compiled outputs.

   ```shell
   owner@ubuntu:~/workspace/resiprocate$ cmake . -DCMAKE_TOOLCHAIN_FILE=$OHOS_SDK/native/build/cmake/ohos.toolchain.cmake -DWITH_SSL=OFF -DUSE_DTLS=OFF -DWITH_C_ARES=OFF -DBUILD_QPID_PROTON=OFF -DRESIP_ASSERT_SYSLOG=OFF -DREGENERATE_MEDIA_SAMPLES=OFF -DBUILD_DSO_PLUGINS=OFF  -DUSE_POPT=OFF -DUSE_MAXMIND_GEOIP=OFF -DBUILD_REPRO=OFF -DBUILD_RETURN=OFF -DBUILD_RECON=OFF -DBUILD_REFLOW=OFF -DBUILD_REND=OFF  -DBUILD_TFM=OFF -DUSE_KURENTO=OFF -DHAVE_CLOCK_GETTIME_MONOTONIC=ON -DCMAKE_INSTALL_PREFIX=/home/owner/install/resiprocate  
   ```

   Here, all optional compilation options in cmake are disabled. If needed, you can cross-compile the corresponding third-party libraries for OpenHarmony, set their paths in `CMAKE_FIND_ROOT_PATH`, and then enable the relevant compilation options.

   Due to cross-compilation, the `try_run()` for `HAVE_CLOCK_GETTIME_MONOTONIC` in the cmake script will fail. Therefore, the test result is passed to the cmake script via a parameter here.

3.  Execute the `make` command:

   After a successful `cmake`, run `make` to start cross-compilation:

   ```shell
   owner@ubuntu:~/workspace/resiprocate$ make                       # Run the make command to compile  
   ```

4. Execute the installation command:

   ```shell
   owner@ubuntu:~/workspace/resiprocate$ make install  
   ```

   After installation, the cross-compiled resiprocate library files can be found in the installation directory `/home/owner/install/resiprocate`.

### Compiling resiprocate versions prior to 1.13

### Obtaining the Source Code

Download the [resiprocate-1.12.0 source code package](https://github.com/resiprocate/resiprocate/archive/refs/tags/resiprocate-1.12.0.tar.gz) from GitHub and extract it to `~/workspace/resiprocate`.

```sh
tar -zxvf resiprocate-1.12.0.tar.gz -C ~/workspace/resiprocate  
```

### Starting the Compilation

1. Set the toolchain path:

   ```shell
   export OHOS_SDK=/home/owner/workspace/ohos-sdk/linux             ## Configure the SDK path. Replace this with your own SDK extraction directory.  
   export AS=${OHOS_SDK}/native/llvm/bin/llvm-as  
   export CC="${OHOS_SDK}/native/llvm/bin/clang --target=aarch64-linux-ohos"    ## For 32-bit targets, use --target=arm-linux-ohos  
   export CXX="${OHOS_SDK}/native/llvm/bin/clang++ --target=aarch64-linux-ohos" ## For 32-bit targets, use --target=arm-linux-ohos  
   export LD=${OHOS_SDK}/native/llvm/bin/ld.lld  
   export STRIP=${OHOS_SDK}/native/llvm/bin/llvm-strip  
   export RANLIB=${OHOS_SDK}/native/llvm/bin/llvm-ranlib  
   export OBJDUMP=${OHOS_SDK}/native/llvm/bin/llvm-objdump  
   export OBJCOPY=${OHOS_SDK}/native/llvm/bin/llvm-objcopy  
   export NM=${OHOS_SDK}/native/llvm/bin/llvm-nm  
   export AR=${OHOS_SDK}/native/llvm/bin/llvm-ar  
   export CFLAGS="-fPIC -D__MUSL__=1"                                            ## For 32-bit, add -march=armv7a  
   export CXXFLAGS="-fPIC -D__MUSL__=1"                                          ## For 32-bit, add -march=armv7a  
   ```

2. Execute the `autoreconf` command:

   The resiprocate source code root directory does not contain a `configure` file. Run `autoreconf` in the root directory to generate it:

   ```shell
   owner@ubuntu:~/workspace/resiprocate$ autoreconf -ifv  
   ```

3. Execute the `configure` command:

   Once generated, run the `configure` script to create the Makefile. The following example configures for ARM64. For 32-bit, replace `aarch64-linux` with `arm-linux`. The `prefix` is set to `/home/owner/install/resiprocate`, which is the installation directory for the cross-compiled output.

   ```shell
   owner@ubuntu:~/workspace/resiprocate$ ./configure --prefix=/home/owner/install/resiprocate --host=aarch64-linux       # Run the configure command to set up cross-compilation  
   ```

4.  Execute the `make` command:

   After a successful `configure`, run `make` to start cross-compilation:

   ```shell
   owner@ubuntu:~/workspace/resiprocate$ make                       # Run the make command to compile  
   ```

5. Execute the installation command:

   ```shell
   owner@ubuntu:~/workspace/resiprocate$ make install  
   ```

   After installation, the cross-compiled resiprocate library files can be found in the installation directory `/home/owner/install/resiprocate`.