# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/brain/repo/mycode/apps/Salamis-Salmon/code/mycode/pf/mobilenode"
  "/home/brain/repo/build/mobilenode"
  "/home/brain/repo/build/_sysbuild/sysbuild/images/mobilenode-prefix"
  "/home/brain/repo/build/_sysbuild/sysbuild/images/mobilenode-prefix/tmp"
  "/home/brain/repo/build/_sysbuild/sysbuild/images/mobilenode-prefix/src/mobilenode-stamp"
  "/home/brain/repo/build/_sysbuild/sysbuild/images/mobilenode-prefix/src"
  "/home/brain/repo/build/_sysbuild/sysbuild/images/mobilenode-prefix/src/mobilenode-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/brain/repo/build/_sysbuild/sysbuild/images/mobilenode-prefix/src/mobilenode-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/brain/repo/build/_sysbuild/sysbuild/images/mobilenode-prefix/src/mobilenode-stamp${cfgdir}") # cfgdir has leading slash
endif()
