# Copyright (c) 2011-2025 Columbia University, System Level Design Group
# SPDX-License-Identifier: Apache-2.0
# TODO: read HLS configuration from device registers instead

EXTRA_CFLAGS ?= -DFFT_FX_WIDTH=64 -L$(SOFT_BUILD)/sysroot/usr/lib -lbpf -lelf -lz -I$(SOFT_BUILD)/linux-build/tools/bpf/resolve_btfids/libbpf/include -I$(DESIGN_PATH)/hello_world_bpf
APPNAME := fft
include $(DRIVERS)/common.mk
