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

.PHONY: all clean

all: offload_$(TARGET).xo test_$(TARGET).xo 

# synth flags
COMMFLAGS := --platform $(PLATFORM) --target $(TARGET) --save-temps --debug -DSYNTHESIS
HLSCFLAGS := --compile $(COMMFLAGS)
LINKFLAGS := --link --optimize 3 $(COMMFLAGS)

offload_$(TARGET).xo: ./offload.cpp ./collectives.hpp
	v++ $(HLSCFLAGS) --temp_dir _x_offload --kernel offload --output $@ $^

test_$(TARGET).xo: ./test.cpp ./collectives.hpp
	v++ $(HLSCFLAGS) --temp_dir _x_test --kernel test --output $@ $^

clean:
	git clean -Xdf
