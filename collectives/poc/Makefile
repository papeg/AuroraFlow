#
# Copyright 2024 Gerrit Pape (papeg@mail.upb.de)
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# change here to test different boards
PART := xcu280-fsvh2892-2L-e
PLATFORM ?= xilinx_u280_gen3x16_xdma_1_202211_1

TARGET := hw

.PHONY: all host xclbin clean

all: host xclbin

aurora:
	$(MAKE) -C ../..

# synth flags
COMMFLAGS := --platform $(PLATFORM) --target $(TARGET) --save-temps --debug -DSYNTHESIS
HLSCFLAGS := --compile $(COMMFLAGS)
LINKFLAGS := --link --optimize 3 $(COMMFLAGS)

p2p_simplex_u32_$(TARGET).xo: ./p2p_simplex.cpp ../collectives.hpp ../offload/common.hpp
	v++ $(HLSCFLAGS) --temp_dir _x_p2p_simplex_u32_$(TARGET) --kernel p2p_simplex_u32 --output $@ $^

p2p_simplex_u32_hw.xclbin: aurora p2p_simplex_u32_$(TARGET).xo link_simplex_u32.cfg
	v++ $(LINKFLAGS) --temp_dir _x_p2p_simplex_u32_$(TARGET)_impl --config link_simplex_u32.cfg --output $@ p2p_simplex_u32_hw.xo ../../aurora_hls_0.xo ../../aurora_hls_1.xo

xclbin: p2p_simplex_u32_hw.xclbin

# host flags
CXX=mpicxx
CXXFLAGS += -std=c++17 -Wall -g
CXXFLAGS += -I$(XILINX_XRT)/include
CXXFLAGS += -fopenmp
LDFLAGS := -L$(XILINX_XRT)/lib
LDFLAGS += $(LDFLAGS) -lxrt_coreutil

poc: ./main.cpp
	$(CXX) -o poc $< $(CXXFLAGS) $(LDFLAGS)

host: poc


clean:
	git clean -Xdf
