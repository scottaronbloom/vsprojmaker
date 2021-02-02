# The MIT License (MIT)
#
# Copyright (c) 2020-2021 Scott Aron Bloom
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sub-license, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

cmake_minimum_required(VERSION 3.1)
if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

project( <PROJECT_NAME> ) 

#include( ${CMAKE_BINARY_DIR}/Project.cmake )

add_custom_target( <PROJECT_NAME> ALL
                 COMMAND echo "Debug Only"
                 OUTPUT "<DEBUG_COMMAND>"
                 SOURCES ${project_SRCS}
                 ${project_H}
                 ${qtproject_UIS}
                 ${qtproject_QRC}
                 ${otherFILES}
                 COMMENT Debug Only Directory
           )

if( NOT EXISTS "<VSPROJDIR>/DebugDir/<PROJECT_NAME>/<PROPSFILENAME>" )
    MESSAGE( FATAL_ERROR "Could not find property file '<VSPROJDIR>/DebugDir/<PROJECT_NAME>/<PROPSFILENAME>'" )
endif()
set_target_properties( <PROJECT_NAME> PROPERTIES VS_USER_PROPS <VSPROJDIR>/DebugDir/<PROJECT_NAME>/<PROPSFILENAME> )