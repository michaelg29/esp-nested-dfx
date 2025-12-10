
APPNAME := fft

# Define the compiler and the flags
CLANG ?= clang
CFLAGS = -O2 -target bpf -c -g
BPF_CFLAGS = -D__TARGET_ARCH_unknown
USERSPACE_CFLAGS = -O2 -Wall -I/usr/include
USERSPACE_LINKER_FLAGS = -lbpf

# Userspace programs
USERSPACE_SKEL = $(APPNAME).skel.h
# Define the BPF program source and the output object file
BPF_SRC = $(APPNAME).bpf.c
BPF_OBJ = $(BUILD_PATH)/$(BPF_SRC:.c=.o)

# Get Clang's default includes on this system. We'll explicitly add these dirs
# to the includes list when compiling with `-target bpf` because otherwise some
# architecture-specific dirs will be "missing" on some architectures/distros -
# headers such as asm/types.h, asm/byteorder.h, asm/socket.h, asm/sockios.h,
# sys/cdefs.h etc. might be missing.
#
# Use '-idirafter': Don't interfere with include mechanics except where the
# build would have failed anyways.
#CLANG_BPF_SYS_INCLUDES ?= $(shell $(CLANG) -v -E - </dev/null 2>&1 \
	| sed -n '/<...> search starts here:/,/End of search list./{ s| \(/.*\)|-idirafter \1|p }')
CLANG_BPF_SYS_INCLUDES ?= -I$(SOFT_BUILD)/linux-build/tools/bpf/resolve_btfids/libbpf/include -I$(SOFT_BUILD)/linux-build/usr/include -I$(DESIGN_PATH)/socgen/esp

all: $(BPF_OBJ) $(USERSPACE_SKEL)

$(BPF_OBJ): $(BPF_SRC) vmlinux.h
	$(CLANG) $(CFLAGS) $(BPF_CFLAGS) $(CLANG_BPF_SYS_INCLUDES) $(BPF_SRC) -o $(BPF_OBJ)

$(USERSPACE_SKEL): $(BPF_OBJ)
	bpftool gen skeleton $< > $@

vmlinux.h:
	bpftool btf dump file $(DESIGN_PATH)/soft-build/ariane/linux-build/vmlinux format c > vmlinux.h

clean:
	rm -f $(BPF_OBJ) $(USERSPACE_SKEL)

cleanall: clean
	rm -f vmlinux.h

.PHONY: all clean cleanall
