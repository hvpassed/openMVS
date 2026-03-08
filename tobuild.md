gcc 13.*
cuda 12.*
spack环境

------
cwk@ta-Super-Server:~/workplace/openMVS/build_spack$ cat ~/spack/var/spack/environments/openmvs_env/spack.yaml 
# This is a Spack Environment file.
#
# It describes a set of packages to be installed, along with
# configuration settings.
spack:
  # add package specs to the `specs` list
  specs:
  - zlib
  - bzip2
  - ninja
  - libaec
  - gmp
  - mpfr
  view: true
  concretizer:
    unify: false
  packages:
    eigen:
      externals:
      - spec: eigen@3.4.0
        prefix: /data/home/cwk/.local
      buildable: false


    boost:
      externals:
      - spec: boost@1.86.0+shared+python+iostreams+program_options+serialization+regex
        prefix: /data/home/cwk/.local
      buildable: false


    cuda:
      externals:
      - spec: cuda@12.6.20
        prefix: /data/home/cwk/cuda-12.6
      buildable: false

    hdf5:
      externals:
      - spec: hdf5@1.14.6
        prefix: /data/home/cwk/.local
      buildable: false
------
编译pcre2-10.42

./configure --prefix=/data/home/cwk/.local
make -j$(nproc)
make install

编译swig4.4.1 


./autogen.sh
# 配置：指定刚才安装的 PCRE2 路径和 SWIG 安装路径
./configure --prefix=/data/home/cwk/.local \
            --with-pcre2-prefix=/data/home/cwk/.local

make -j$(nproc)
make install


编译faiss1.10.0-gpu
无python
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=$HOME/.local \
  -DBUILD_SHARED_LIBS=ON \
  -DFAISS_ENABLE_GPU=ON \
  -DFAISS_ENABLE_PYTHON=OFF \
  -DCUDAToolkit_ROOT=/data/home/cwk/cuda-12.4 \
  -DCMAKE_CUDA_ARCHITECTURES=89 \
  -DBLA_VENDOR=Intel10_64_dyn \
  -DMKL_LIBRARIES="/data/home/cwk/intel/oneapi/mkl/2025.3/lib/libmkl_intel_lp64.so;/data/home/cwk/intel/oneapi/mkl/2025.3/lib/libmkl_intel_thread.so;/data/home/cwk/intel/oneapi/mkl/2025.3/lib/libmkl_core.so;/data/home/cwk/intel/oneapi/compiler/2025.3/lib/libiomp5.so;-lm;-ldl"

cmake --build ./ -j"$(nproc)" --target faiss faiss_gpu
cmake --install build
make install

编译 hdf5-1.14.6
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/data/home/cwk/.local \
    -DBUILD_SHARED_LIBS=ON \
    -DBUILD_STATIC_LIBS=OFF \
    -DHDF5_BUILD_CPP_LIB=ON \
    -DHDF5_BUILD_HL_LIB=ON \
    -DHDF5_ENABLE_Z_LIB_SUPPORT=ON \
    -DZLIB_ROOT=$ZLIB_PREFIX \
    -DHDF5_ENABLE_THREADSAFE=OFF

make
make install 

编译hdf5 lzf插件

cd ~/dep/h5py-3.16.0/lzf

gcc -O3 -fPIC -shared \
  -I$HOME/.local/include \
  -I./lzf \
  lzf/*.c lzf_filter.c \
  -o liblzf_filter.so

mkdir -p $HOME/.hdf5_plugins
cp liblzf_filter.so $HOME/.hdf5_plugins/

~/.bashrc 中添加
export HDF5_PLUGIN_PATH=$HOME/.hdf5_plugins
unset HDF5_PLUGIN_PRELOAD

编译安装libjpeg
wget https://ijg.org/files/jpegsrc.v10.tar.gz
tar -xzvf jpegsrc.v10.tar.gz
./configure --prefix=$HOME/.local
make
make install

编译安装libpng
下载https://sourceforge.net/projects/libpng/files/libpng16/1.6.55/libpng-1.6.55.tar.gz/download
tar -xzvf libpng-1.6.55.tar.gz
./configure --prefix=$HOME/.local
make
make install

编译安装libtiff
wget https://download.osgeo.org/libtiff/tiff-4.7.1.zip
unzip tiff-4.7.1.zip
mkdir build_spack
cd build_spack
cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/.local  
make
make install


安装nv-codec-headers
git clone git@github.com:FFmpeg/nv-codec-headers.git --branch n12.1.14.0 nv-codec-headers-12.1.14.0
cd nv-codec-headers-12.1.14.0
make PREFIX=$HOME/.local
make install PREFIX=$HOME/.local

安装 SuiteSparse-7.12.2
cmake .. -DSUITESPARSE_DEMOS=OFF  -DBUILD_TESTING=OFF -DCMAKE_INSTALL_PREFIX=$HOME/.local -DBUILD_SHARED_LIBS=ON

安装 gflags-2.3.0 gtest-1.17.0 glog-0.8.0
安装 ceres-solver-2.2.0
cmake .. -DCMAKE_INSTALL_PREFIX=/data/home/cwk/.local          -DBUILD_TESTING=OFF          -DBUILD_EXAMPLES=OFF          -DBUILD_SHARED_LIBS=ON          -DCMAKE_CUDA_ARCHITECTURES="89"
-DCMAKE_CUDA_ARCHITECTURES="89" 应该无效，手动去CMakeLists.txt中改

编译安装opencv-4.10.0 + contribu
export CMAKE_PREFIX_PATH="$HOME/.local:$HOME/spack/var/spack/environments/openmvs_env/.spack-env/view"
export PREFIX=$HOME/.local

cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$PREFIX" \
  -DOPENCV_EXTRA_MODULES_PATH=../../opencv_contrib-4.10.0/modules \
  -DBUILD_SHARED_LIBS=ON \
  -DBUILD_TESTS=OFF \
  -DBUILD_PERF_TESTS=OFF \
  -DBUILD_EXAMPLES=OFF \
  -DBUILD_DOCS=OFF \
  -DBUILD_opencv_apps=OFF \
  -DBUILD_JAVA=OFF \
  -DBUILD_ANDROID_PROJECTS=OFF \
  -DBUILD_ANDROID_EXAMPLES=OFF \
  -DWITH_CUDA=ON \
  -DWITH_CUBLAS=ON \
  -DWITH_CUFFT=ON \
  -DWITH_CUDNN=ON \
  -DOPENCV_DNN_CUDA=ON \
  -DCUDA_FAST_MATH=ON \
  -DENABLE_FAST_MATH=ON \
  -DCUDA_ARCH_BIN=8.9 \
  -DCUDA_ARCH_PTX= \
  -DOPENCV_ENABLE_NONFREE=ON \
  -DWITH_OPENGL=ON \
  -DWITH_TBB=ON \
  -DWITH_OPENMP=ON \
  -DWITH_IPP=ON \
  -DWITH_EIGEN=ON \
  -DWITH_FFMPEG=ON \
  -DWITH_GSTREAMER=ON \
  -DWITH_V4L=ON \
  -DWITH_QT=OFF \
  -DWITH_GTK=ON \
  -DWITH_NVCUVID=ON \
  -DWITH_NVCUVENC=ON \
  -DCUDA_TOOLKIT_ROOT_DIR="$CUDA_HOME" \
  -DCMAKE_INSTALL_RPATH="$PREFIX/lib;$PREFIX/lib64;$CUDA_HOME/lib64" \
  -DJPEG_INCLUDE_DIR="$HOME/.local/include" \
  -DJPEG_LIBRARY="$HOME/.local/lib/libjpeg.so" \
  -DCMAKE_BUILD_WITH_INSTALL_RPATH=ON


安装CGAL 6.1.1
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/.local -DCMAKE_BUILD_TYPE=Release
make 
make install

安装VCG
cd ~/dep
git clone https://gh-proxy.org/https://github.com/cnr-isti-vclab/vcglib.git vcglib