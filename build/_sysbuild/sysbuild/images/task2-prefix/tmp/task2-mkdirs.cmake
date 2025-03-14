# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/brain/repo/mycode/apps/prac1/task2"
  "/home/brain/repo/build/task2"
  "/home/brain/repo/build/_sysbuild/sysbuild/images/task2-prefix"
  "/home/brain/repo/build/_sysbuild/sysbuild/images/task2-prefix/tmp"
  "/home/brain/repo/build/_sysbuild/sysbuild/images/task2-prefix/src/task2-stamp"
  "/home/brain/repo/build/_sysbuild/sysbuild/images/task2-prefix/src"
  "/home/brain/repo/build/_sysbuild/sysbuild/images/task2-prefix/src/task2-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/brain/repo/build/_sysbuild/sysbuild/images/task2-prefix/src/task2-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/brain/repo/build/_sysbuild/sysbuild/images/task2-prefix/src/task2-stamp${cfgdir}") # cfgdir has leading slash
endif()
