CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

PROJECT(xlua_lib)

SET(CMAKE_CXX_STANDARD 11)
ADD_SUBDIRECTORY(3rd)
ADD_SUBDIRECTORY(xlua)
ADD_SUBDIRECTORY(test)
