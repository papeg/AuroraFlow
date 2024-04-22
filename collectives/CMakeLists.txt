# 
#  Copyright 2024 Gerrit Pape
# 
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
# 
#      http://www.apache.org/licenses/LICENSE-2.0
# 
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
# 
cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(AuroraRingCollectivesTest)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(${CMAKE_SOURCE_DIR}/../emulation ${CMAKE_BINARY_DIR}/auroraemu)

set(KERNEL_FILES ${CMAKE_SOURCE_DIR}/offload.cpp ${CMAKE_SOURCE_DIR}/test.cpp)
set(SOURCE_FILES ${CMAKE_SOURCE_DIR}/main.cpp )

find_package(MPI REQUIRED)
include_directories(${MPI_CXX_INCLUDE_PATH})
link_libraries(${MPI_LIBRARIES})

#find_package(XRT REQUIRED)
include_directories(/opt/software/FPGA/Xilinx/xrt/xrt_2.15)
link_directories(/opt/software/FPGA/Xilinx/xrt/xrt_2.15/lib)
link_libraries(xrt_coreutil uuid)

add_executable(host_test_collectives ${SOURCE_FILES} ${KERNEL_FILES})
target_link_libraries(host_test_collectives PUBLIC auroraemu)