#include <bpf/libbpf.h>
#include <bpf/bpf.h>
#include <stdio.h>
#include <unistd.h>

#include <user_ringbuf.skel.h>

int __attribute__((noinline)) get_message(int i) {
    return i * 5;
}

int main() {
    printf("Hello, FFT\n");

    struct bpf_object *obj;
    struct bpf_program *prog;
    struct bpf_link *link;
    int prog_fd;

    // Load and verify BPF application
    fprintf(stderr, "Loading BPF code in memory\n");
    obj = bpf_object__open_file("/applications/test/hello_world.bpf.o", NULL);

    if (libbpf_get_error(obj)) {
        fprintf(stderr, "ERROR: opening BPF object file failed\n");
        return 1;
    }

    // Load BPF program
    fprintf(stderr, "Loading and verifying the code in the kernel\n");
    if (bpf_object__load(obj)) {
        fprintf(stderr, "ERROR: loading BPF object file failed\n");
        return 1;
    }

    // Attach BPF program
    fprintf(stderr, "Attaching BPF program to tracepoint\n");
    prog = bpf_object__find_program_by_name(obj, "sched_wakeup");
    if (libbpf_get_error(prog)) {
        fprintf(stderr, "ERROR: finding BPF program failed\n");
        return 1;
    }
    prog_fd = bpf_program__fd(prog);
    if (prog_fd < 0) {
        fprintf(stderr, "ERROR: getting BPF program FD failed\n");
        return 1;
    }
    // Check it out at: /sys/kernel/debug/tracing/events/raw_syscalls/sys_enter
    //link = bpf_program__attach_tracepoint(prog, "raw_syscalls", "sys_enter");
    //link = bpf_program__attach_tracepoint(prog, "tp_btf", "sched_wakeup");
    link = bpf_program__attach_trace(prog);

    if (libbpf_get_error(link)) {
        fprintf(stderr, "ERROR: Attaching BPF program to tracepoint failed\n");
        return 1;
    }

/*
    struct user_ringbuf_bpf *skel;
	int err;

	// Load and verify BPF application
	skel = user_ringbuf_bpf__open();

	if (!skel)
	{
		fprintf(stderr, "Failed to open and load BPF skeleton\n");
		return 1;
	}

	// Load & verify BPF programs
	err = user_ringbuf_bpf__load(skel);
	if (err)
	{
		fprintf(stderr, "Failed to load and verify BPF skeleton\n");
		goto cleanup;
	}

	// Attach tracepoints
	err = user_ringbuf_bpf__attach(skel);
	if (err)
	{
		fprintf(stderr, "Failed to attach BPF skeleton\n");
		goto cleanup;
	}

	*/

    struct bpf_map *track_pid_map = bpf_object__find_map_by_name(obj, "track_pid_map");
    //struct bpf_map *track_pid_map = bpf_object__find_map_by_name(skel->obj, "track_pid_map");
    int fd = bpf_map__fd(track_pid_map);

    printf("BPF tracepoint program attached. Press E to exit...\n");
    int i = 0;
    int err, val;
    for (char c = getchar(); c != 'E' && c != 'e'; c = getchar()) {
        if (!(
            (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') ||
            (c >= 'A' && c <= 'Z')
        )) continue;

        val = (c % 10);
        printf("You entered %c => val is %d\n", c, val);

        printf("Trying probe: %d\n", get_message(2));

        // write element
        err = bpf_map_update_elem(fd, &i, &val, BPF_ANY);
        if (err < 0) {
            fprintf(stderr,
                        "ERROR: Attaching filter PID value to extension failed.\n");
        }
        printf("Wrote %d to index %d\n", val, i);

        printf("Reading: ");
        for (int j = 0; j < 16; ++j) {
            err = bpf_map_lookup_elem(fd, &j, &val);
            if (err < 0) {
                fprintf(stderr, "Failed to lookup element in histogram: %d\n", err);
                goto cleanup;
            }
            printf("%d, ", val);
        }
        printf("\n");
    }

    // Cleanup
cleanup:
    bpf_link__destroy(link);
    bpf_object__close(obj);
    //user_ringbuf_bpf__destroy(skel);

    return 0;
}
