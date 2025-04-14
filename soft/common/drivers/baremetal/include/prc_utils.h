#include <stdio.h>
#ifndef __riscv
#include <stdlib.h>
#endif
#include <esp_probe.h>

#define LEN_DEVNAME_MAX 32

#define DECOUPLER_REG  0x30
#define PRC_INTERRUPT_REG 0x34
typedef struct pbs_map {
    char name [LEN_DEVNAME_MAX]; // name of the bitstream
    unsigned pbs_size;           // size of the bitstream
    unsigned long long pbs_addr; // address offset of the bitstream data
    unsigned pbs_tile_id;        // ID of the tile that the bitstream targets
    unsigned is_nested;          // whether the bitstream is a nested region
    unsigned parent_pbs_idx;     // index of the PBS for the parent tile configuration containing the valid region configuration
} pbs_map;

void init_nested_prc_validation();
int decouple_acc(struct esp_device *dev, unsigned val);
unsigned int reconfigure_FPGA(struct esp_device *dev, unsigned pbs_id);
