
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

unsigned int out_i = 1;
unsigned int incr = 1;

//SEC("tracepoint")
//int handle_tracepoint(void *ctx)

SEC("tp_btf/sched_wakeup")
int BPF_PROG(sched_wakeup, struct task_struct *p)

//SEC("uprobe//applications/test/fft_stratus.exe:get_message")
//int BPF_UPROBE(trace_my_function)

{
    // bpf_get_current_pid_tgid is a helper function!
    int pid = bpf_get_current_pid_tgid() >> 32;
    bpf_printk("BPF triggered from PID %d.\n", pid);

    // read input
    unsigned int in_i = 0;
    unsigned int *pid_ptr = bpf_map_lookup_elem(&track_pid_map, &in_i);
    if (!pid_ptr) {
        bpf_map_update_elem(&track_pid_map, &in_i, &in_i, BPF_ANY);
        return 0;
    }

    // bound checking
    if (out_i > 15) {
        return 0;
    }

    // write output
    unsigned int cur = *pid_ptr + incr;
    bpf_map_update_elem(&track_pid_map, &out_i, &cur, BPF_ANY);
    __sync_fetch_and_add(&incr, 1);

    return 0;
}
