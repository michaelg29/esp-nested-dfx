# Copyright (c) 2011-2025 Columbia University, System Level Design Group
# SPDX-License-Identifier: Apache-2.0
# TODO: read HLS configuration from device registers instead
EXTRA_CFLAGS ?= -DFFT_FX_WIDTH=64 -L/home/mgrieco/src/esp-nested-dfx/socs/xilinx-vcu118-xcvu9p/soft-build/ariane/sysroot/usr/lib -lbpf -lelf -lz -I/home/mgrieco/src/esp-nested-dfx/socs/xilinx-vcu118-xcvu9p/soft-build/ariane/linux-build/tools/bpf/resolve_btfids/libbpf/include -I/home/mgrieco/src/esp-nested-dfx/socs/xilinx-vcu118-xcvu9p/hello_world_bpf
APPNAME := fft
include $(DRIVERS)/common.mk
