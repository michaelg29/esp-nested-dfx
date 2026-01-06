
#include "cfg.h"
#include "utils/fft_utils.h"

#include <stdio.h>
#include <unistd.h>
#include <time.h>

#define BPF_SKEL 1
#define BPF 1

#ifdef BPF
#include <bpf/libbpf.h>
#include <bpf/bpf.h>

#ifdef BPF_SKEL
#include <fft.skel.h>
#endif // BPF_SKEL
#endif // BPF

static unsigned in_words_adj;
static unsigned out_words_adj;
static unsigned in_len;
static unsigned out_len;
static unsigned in_size;
static unsigned out_size;
static unsigned out_offset;
static unsigned size;

const float ERR_TH = 0.05;

/* User-defined code */
static int validate_buffer(token_t *out, float *gold)
{
    int j;
    unsigned errors = 0;

    for (j = 0; j < 2 * len * num_batches; j++) {
        native_t val = fx2float(out[j], FX_IL);
        if ((fabs(gold[j] - val) / fabs(gold[j])) > ERR_TH) { errors++; }
    }

    printf("  + Relative error > %.02f for %d output values out of %d\n", ERR_TH, errors,
           2 * len * num_batches);

    return errors;
}

/* User-defined code */
static void init_buffer(token_t *in, float *gold)
{
    int j, b;
    const float LO = -1.0;
    const float HI = 1.0;

    srand((unsigned int)time(NULL));

    for (j = 0; j < 2 * len * num_batches; j++) {
        float scaling_factor = (float)rand() / (float)RAND_MAX;
        gold[j]              = LO + scaling_factor * (HI - LO);
    }
    // preprocess with bitreverse (fast in software anyway)
    if (!do_bitrev) fft_bit_reverse(gold, len, log_len);

    // convert input to fixed point
    for (j = 0; j < in_len; j++)
        in[j] = float2fx((native_t)gold[j], FX_IL);

    // Compute golden output
    for (b = 0; b < num_batches; b++)
        fft_comp(&gold[b * 2 * len], len, log_len, -1, do_bitrev);
}

/* User-defined code */
static void init_parameters()
{
    if (DMA_WORD_PER_BEAT(sizeof(token_t)) == 0) {
        in_words_adj  = 2 * len * num_batches;
        out_words_adj = 2 * len * num_batches;
    }
    else {
        in_words_adj  = round_up(2 * len * num_batches, DMA_WORD_PER_BEAT(sizeof(token_t)));
        out_words_adj = round_up(2 * len * num_batches, DMA_WORD_PER_BEAT(sizeof(token_t)));
    }
    in_len     = in_words_adj;
    out_len    = out_words_adj;
    in_size    = in_len * sizeof(token_t);
    out_size   = out_len * sizeof(token_t);
    out_offset = in_len;
    size       = (out_offset * sizeof(token_t)) + out_size;
}

// create BPF hook at this function
int __attribute__((noinline)) wait_for_fft(int i) {
    // iopoll ring buffer
    struct timespec spec;
    unsigned long long time_in_ns;
    clock_gettime(CLOCK_REALTIME, &spec);
    time_in_ns = (unsigned long long)1000000000 * spec.tv_sec + spec.tv_nsec;
    printf("wait_for_fft entry at %llu\n", time_in_ns);
    return i * 5;
}

int __attribute__((noinline)) fft(int i) {
    // iopoll ring buffer
    return i * 5;
}

// create BPF hook at this function
int __attribute__((noinline)) fft_exit(int i) {
    // iopoll ring buffer
    return i * 5;
}

int main() {

    struct timespec spec, spec2;
    unsigned long long time_in_ns;

    printf("Hello, FFT\n");

#ifdef BPF

    int err;
    unsigned int val;
    //libbpf_set_print(libbpf_print_fn);

#ifndef BPF_SKEL
    struct bpf_object *obj;
    struct bpf_program *prog;
    struct bpf_link *link;
    int prog_fd;

    // Load and verify BPF application
    fprintf(stderr, "Loading BPF code in memory\n");
    obj = bpf_object__open_file("/applications/test/fft.bpf.o", NULL);

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

    link = bpf_program__attach_trace(prog);

    //link = bpf_program__attach_uprobe_multi(prog,
    //                             0,
    //                             "/applications/test/fft_stratus.exe",
    //                             "get_message",
    //                             NULL);

    //static int attach_uprobe(const struct bpf_program *prog, long cookie, struct bpf_link **link)

    if (libbpf_get_error(link)) {
        fprintf(stderr, "ERROR: Attaching BPF program to tracepoint failed\n");
        return 1;
    }

#else

    struct fft_bpf *skel;
    LIBBPF_OPTS(bpf_uprobe_opts, uprobe_opts);

	// Load and verify BPF application
	clock_gettime(CLOCK_REALTIME, &spec);
	skel = fft_bpf__open();

	if (!skel)
	{
		fprintf(stderr, "Failed to open and load BPF skeleton\n");
		return 1;
	}

	// Load & verify BPF programs
	err = fft_bpf__load(skel);
	if (err)
	{
		fprintf(stderr, "Failed to load and verify BPF skeleton\n");
		goto cleanup;
	}
	if (!skel->progs.sched_wakeup)
	{
	    fprintf(stderr, "sched_wakeup program has no value\n");
	}

	// Attach tracepoints
	//fprintf(stderr, "Attaching BPF program to uprobe\n");
    //err = fft_bpf__attach(skel);

	uprobe_opts.func_name = "wait_for_fft";
	skel->links.sched_wakeup = bpf_program__attach_uprobe_opts(
		skel->progs.sched_wakeup, 0 /* self pid */,
		"/applications/test/fft_stratus.exe" /* binary path */,
		0 /* offset for function */,
		&uprobe_opts /* opts */);
	if (!skel->links.sched_wakeup) {
		err = -errno;
		fprintf(stderr, "Failed to attach uprobe sched_wakeup: %d\n", err);
	}

	uprobe_opts.func_name = "fft_exit";
	skel->links.fft_exit = bpf_program__attach_uprobe_opts(
		skel->progs.fft_exit, 0 /* self pid */,
		"/applications/test/fft_stratus.exe" /* binary path */,
		0 /* offset for function */,
		&uprobe_opts /* opts */);
	if (!skel->links.fft_exit) {
		err = -errno;
		fprintf(stderr, "Failed to attach uprobe fft_exit: %d\n", err);
	}

	if (err)
	{
		fprintf(stderr, "Failed to attach BPF skeleton\n");
		goto cleanup;
	}

#endif // BPF_SKEL

#ifndef BPF_SKEL
    struct bpf_map *track_pid_map = bpf_object__find_map_by_name(obj, "track_pid_map");
#else
    struct bpf_map *track_pid_map = bpf_object__find_map_by_name(skel->obj, "track_pid_map");
#endif // BPF_SKEL
    int fd = bpf_map__fd(track_pid_map);

    clock_gettime(CLOCK_REALTIME, &spec2);
    time_in_ns = (unsigned long long)1000000000 * spec.tv_sec + spec.tv_nsec;
    printf("Program started load at %lld\n", time_in_ns);
    time_in_ns = (unsigned long long)1000000000 * spec2.tv_sec + spec2.tv_nsec;
    printf("Program ended load at %lld\n", time_in_ns);

    printf("BPF tracepoint program attached. Press E to exit...\n");
    int i = 0;
    for (char c = getchar(); c != 'E' && c != 'e'; c = getchar()) {
        if (!(
            (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') ||
            (c >= 'A' && c <= 'Z')
        )) continue;

        val = (c % 10);
        printf("You entered %c => val is %x\n", c, val);

        // write element
        i = 0;
        err = bpf_map_update_elem(fd, &i, &val, BPF_ANY);
        if (err < 0) {
            fprintf(stderr,
                        "ERROR: Attaching filter PID value to extension failed.\n");
        }
        printf("Wrote %x to index %d\n", val, i);

        printf("Reading before: ");
        for (int j = 0; j < 16; ++j) {
            err = bpf_map_lookup_elem(fd, &j, &val);
            if (err < 0) {
                fprintf(stderr, "Failed to lookup element in histogram: %d\n", err);
                goto cleanup;
            }
            printf("%llx, ", val);
        }
        printf("\n");

        clock_gettime(CLOCK_REALTIME, &spec);
        time_in_ns = (unsigned long long)1000000000 * spec.tv_sec + spec.tv_nsec;
        wait_for_fft(val);
        printf("wait_for_fft invocation at %llu\n", time_in_ns);

        printf("Reading after: ");
        for (int j = 0; j < 16; ++j) {
            err = bpf_map_lookup_elem(fd, &j, &val);
            if (err < 0) {
                fprintf(stderr, "Failed to lookup element in histogram: %d\n", err);
                goto cleanup;
            }
            printf("%llx, ", val);
        }
        printf("\n");

        i = 5;
        val = 0;
        bpf_map_update_elem(fd, &i, &val, BPF_ANY);
        bpf_map_lookup_elem(fd, &i, &val);
        printf("array[%d] = %d\n", i, val);
    }

    // Cleanup
cleanup:
#ifndef BPF_SKEL
    bpf_link__destroy(link);
    bpf_object__close(obj);
#else
    fft_exit(1);
    fft_bpf__destroy(skel);
#endif // BPF_SKEL

#else

    int errors;

    float *gold;
    token_t *buf;

    const float ERROR_COUNT_TH = 0.01;

    // set parameters
    init_parameters();

    // allocate buffers
    buf               = (token_t *)esp_alloc(size);
    cfg_000[0].hw_buf = buf;
    gold              = malloc(out_len * sizeof(float) * num_batches);

    printf("\n====== %s ======\n\n", cfg_000[0].devname);
    printf("  .len = %d\n", len);
    printf("  .batch_size = %d\n", fft_cfg_000[0].batch_size);

    unsigned coherence = ACC_COH_NONE;

    init_buffer(buf, gold);

    // set coherence mode
    fft_cfg_000[0].esp.coherence = coherence;

    // run accelerator
    printf("  ** START **\n");
    clock_gettime(CLOCK_REALTIME, &spec);
    time_in_ns = (unsigned long long)1000000000 * spec.tv_sec + spec.tv_nsec;
    printf("Invocation started at %llu ns\n", time_in_ns);
    esp_run(cfg_000, NACC);
    printf("  ** DONE **\n");

    // validate output
    errors         = validate_buffer(buf, gold);
    float err_rate = (float)errors / (float)(2 * len * num_batches);

    if (err_rate > ERROR_COUNT_TH) printf("\n	+ TEST FAIL: exceeding error count "
                                          "threshold\n");
    else
        printf("\n	+ TEST PASS: not exceeding error count threshold\n");

    printf("\n====== %s ======\n\n", cfg_000[0].devname);

    // cleanup
    free(gold);
    esp_free(buf);

    return errors;

#endif // BPF

    return 0;
}
