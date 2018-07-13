#!/usr/bin/env bash

# first, download and install CMake
export CMAKE_URL="https://cmake.org/files/v3.11/cmake-3.11.4-Linux-x86_64.sh" ;
echo "Downloading CMake"
wget "$CMAKE_URL" > /dev/null 2>&1 ;
sudo sh cmake-* --prefix=/usr/local --exclude-subdir ;
export PATH="/usr/local/bin:$PATH" ;
cmake --version ;

# set env for CC and CXX
export CC=gcc-7 ;
export CXX=g++-7 ;

# now, let's download and install Boost, only specific libraries,
# of course
export BOOST_URL="https://dl.bintray.com/boostorg/release/1.67.0/source/boost_1_67_0.tar.bz2" ;
export BOOST_BUILD_CMD="./b2 --build-dir=/tmp/build-boost 
toolset=gcc-7
variant=release
link=shared
threading=multi
runtime-link=shared
--with-program_options
--with-system
--with-filesystem
--with-test
--with-locale
--with-iostreams" ;
echo "Downloading Boost"
wget "${BOOST_URL}" > /dev/null 2>&1 ;
mkdir boost
tar xf boost*.tar.bz2 -C ./boost --strip-components=1 ;
cd ./boost ;
echo 'using gcc : 7 : /usr/bin/g++-7 ; ' >> tools/build/src/user-config.jam ;
./bootstrap.sh ;
${BOOST_BUILD_CMD} > boost_build_log.txt ;
sudo ${BOOST_BUILD_CMD} install > boost_install_log.txt ;
cd .. ; # up one directory

# configure
mkdir build && cd build ;
cmake .. ;

