
#include "vmlinux.h"

#include <bpf/bpf_endian.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>

char LICENSE[] SEC("license") = "Dual BSD/GPL";

struct {
	__uint(type, BPF_MAP_TYPE_ARRAY);
	__type(key, u32);
	__type(value, u32);
	__uint(max_entries, 16);
} track_pid_map SEC(".maps");

unsigned long long _iomapped = 0;
unsigned int incr = 0;

//SEC("tracepoint")
//int handle_tracepoint(void *ctx)

//SEC("tp_btf/sched_wakeup")
//int BPF_PROG(sched_wakeup, struct task_struct *p)

SEC("uprobe//applications/test/fft_stratus.exe:fft_exit")
int BPF_UPROBE(fft_exit)
{
    bpf_esp_map((void*)&_iomapped, 0x1000, (void*)(u64)0x60010000);
    if (_iomapped) {
        bpf_esp_map((void*)&_iomapped, 0x1000, (void*)_iomapped);
    }
    return 0;
}

SEC("uprobe//applications/test/fft_stratus.exe:wait_for_fft")
int BPF_UPROBE(sched_wakeup)

{
    unsigned int in_i = 0;
    unsigned int out_i = 1;
    unsigned int cur = 0;
    unsigned long long addr;

    unsigned long long entrytime = bpf_ktime_get_ns();
    unsigned long long exittime;

    // _iomapped will have a value if successfully mapped
    // cannot be a local variable, helper function cannot write to something in the function stack frame
    //bpf_esp_map((void*)&_iomapped, 0x1000, (void*)(u64)0x60090000);
    bpf_esp_map((void*)&_iomapped, 0x1fc, (void*)(u64)60010000);

    if (!_iomapped) {
        out_i = 1;
        in_i = (u32)(_iomapped >> 32);
        bpf_map_update_elem(&track_pid_map, &out_i, &in_i, BPF_ANY);

        out_i = 2;
        in_i = (u32)_iomapped;
        bpf_map_update_elem(&track_pid_map, &out_i, &in_i, BPF_ANY);
        return 0;
    }

    // bpf_get_current_pid_tgid is a helper function!
    int pid = bpf_get_current_pid_tgid() >> 32;
    bpf_printk("BPF triggered from PID %d.\n", pid);

    // read input
    unsigned int *pid_ptr = bpf_map_lookup_elem(&track_pid_map, &in_i);
    if (!pid_ptr) {
        bpf_map_update_elem(&track_pid_map, &in_i, &in_i, BPF_ANY);
        return 0;
    }

    // write first output (raw input)
    out_i = 1;
    bpf_map_update_elem(&track_pid_map, &out_i, pid_ptr, BPF_ANY);

    // write second output (incremented input)
    out_i = 2;
    cur = *pid_ptr + incr;
    bpf_map_update_elem(&track_pid_map, &out_i, &cur, BPF_ANY);
    __sync_fetch_and_add(&incr, 1);

    // write third output (counter)
    out_i = 3;
    bpf_map_update_elem(&track_pid_map, &out_i, &incr, BPF_ANY);

    // write fourth output (target tile)
    out_i = 4;
    cur = 0x0000BEEF + incr;
    bpf_map_update_elem(&track_pid_map, &out_i, &cur, BPF_ANY);

    // write fifth output (read value from accelerator tile)
    out_i = 5;
    //addr = _iomapped + 0x584; // tile 2: tile ID
    //addr = _iomapped + 0x598; // 0x580 + 0x6 * 4 = 0x580 + 0x18
#define REG 0x40
    bpf_esp_write(&cur, 4, (void*)(_iomapped));
    cur = bpf_esp_read(&cur, 4, (void*)(_iomapped + 0x20));
    bpf_map_update_elem(&track_pid_map, &out_i, &cur, BPF_ANY);

    exittime = bpf_ktime_get_ns();

    out_i = 6;
    cur = (u32)(entrytime >> 32);
    bpf_map_update_elem(&track_pid_map, &out_i, &cur, BPF_ANY);

    out_i = 7;
    cur = (u32)entrytime;
    bpf_map_update_elem(&track_pid_map, &out_i, &cur, BPF_ANY);

    out_i = 8;
    cur = (u32)(exittime >> 32);
    bpf_map_update_elem(&track_pid_map, &out_i, &cur, BPF_ANY);

    out_i = 9;
    cur = (u32)exittime;
    bpf_map_update_elem(&track_pid_map, &out_i, &cur, BPF_ANY);

    return 0;
}
