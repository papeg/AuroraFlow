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

CXX=mpicxx

# change here to test different boards
PART := xcu280-fsvh2892-2L-e
PLATFORM ?= xilinx_u280_gen3x16_xdma_1_202211_1

TARGET := hw



# synth flags
COMMFLAGS := --platform $(PLATFORM) --target $(TARGET) --save-temps --debug
ifeq ($(TARGET), hw)
	COMMFLAGS += -DSYNTHESIS
endif
HLSCFLAGS := --compile $(COMMFLAGS)
LINKFLAGS := --link --optimize 3 $(COMMFLAGS)

offload_$(TARGET).xo: ./offload.cpp ./collectives.hpp
	v++ $(HLSCFLAGS) --temp_dir _x_offload --kernel offload --output $@ $^

# host build for example
CXXFLAGS += -std=c++17 -Wall -g
CXXFLAGS += -I$(XILINX_XRT)/include
CXXFLAGS += -fopenmp
LDFLAGS := -L$(XILINX_XRT)/lib
LDFLAGS += $(LDFLAGS) -lxrt_coreutil

host_aurora_hls_test: ./host/host_aurora_hls_test.cpp ./host/Aurora.hpp
	$(CXX) -o host_aurora_hls_test $< $(CXXFLAGS) $(LDFLAGS)

host: host_aurora_hls_test

clean:
	git clean -Xdf
