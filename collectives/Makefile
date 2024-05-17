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

.PHONY: all aurora xclbin clean

all: rx_$(TARGET).xo tx_$(TARGET).xo offload_$(TARGET).xo test_$(TARGET).xo 

aurora:
	$(MAKE) -C ..

emconfig.json:
	emconfigutil --platform $(PLATFORM) --od emconfig
	cp emconfig/emconfig.json .

# synth flags
COMMFLAGS := --platform $(PLATFORM) --target $(TARGET) --save-temps --debug -DSYNTHESIS
HLSCFLAGS := --compile $(COMMFLAGS)
LINKFLAGS := --link --optimize 3 $(COMMFLAGS)

rx_$(TARGET).xo: ./offload/rx.cpp ./offload/common.hpp ./collectives.hpp
	v++ $(HLSCFLAGS) --temp_dir _x_test_$(TARGET) --kernel rx --output $@ $^

tx_$(TARGET).xo: ./offload/tx.cpp ./offload/common.hpp ./collectives.hpp
	v++ $(HLSCFLAGS) --temp_dir _x_test_$(TARGET) --kernel tx --output $@ $^

offload_$(TARGET).xo: ./offload/offload.cpp ./offload/common.hpp ./collectives.hpp
	v++ $(HLSCFLAGS) --temp_dir _x_offload_$(TARGET) --kernel offload --output $@ $^

test_$(TARGET).xo: ./test.cpp ./collectives.hpp
	v++ $(HLSCFLAGS) --temp_dir _x_test_$(TARGET) --kernel test --output $@ $^

collectives_test_hw.xclbin: aurora offload_hw.xo test_hw.xo rx_hw.xo tx_hw.xo link.cfg
	v++ $(LINKFLAGS) --temp_dir _x_collectives_test_hw --config link.cfg --output $@ ../aurora_hls_0.xo ../aurora_hls_1.xo offload_hw.xo rx_hw.xo tx_hw.xo test_hw.xo

collectives_test_sw_emu.xclbin: offload_sw_emu.xo test_sw_emu.xo rx_sw_emu.xo tx_sw_emu.xo link_emu.cfg
	v++ $(LINKFLAGS) --temp_dir _x_collectives_test_sw_emu --config link_emu.cfg --output $@ offload_sw_emu.xo rx_sw_emu.xo tx_sw_emu.xo test_sw_emu.xo

collectives_test_hw_emu.xclbin: emconfig.json offload_hw_emu.xo test_hw_emu.xo rx_hw_emu.xo tx_hw_emu.xo link_emu.cfg
	v++ $(LINKFLAGS) --temp_dir _x_collectives_test_hw_emu --config link_emu.cfg --output $@ offload_hw_emu.xo rx_hw_emu.xo tx_hw_emu.xo test_hw_emu.xo

xclbin: collectives_test_$(TARGET).xclbin

clean:
	git clean -Xdf
