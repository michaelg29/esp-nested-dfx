// Copyright (c) 2011-2025 Columbia University, System Level Design Group
// SPDX-License-Identifier: Apache-2.0

//#include <stdio.h>
#include <uart.h>

static char str[2] = { '0', '\0' };

void print_hex(unsigned long long val) {
    int seen_non_zero = 0;

    //for (unsigned long long mask = 0xf << 60; mask; mask >>= 4) {
    unsigned long long mask = 0xf;
    mask <<= 60;
    for (int lsb = 60; lsb >= 0; lsb -= 4, mask >>= 4) {
        unsigned long long masked = (val & mask) >> lsb;
        if (masked != 0) {
            seen_non_zero = 1;
        }
        if (seen_non_zero) {
            if (masked >= 10) {
                str[0] = 'a' + (masked - 10);
            }
            else {
                str[0] = '0' + (masked);
            }
            print_uart(str);
        }
    }

    if (!seen_non_zero) {
        str[0] = '0';
        print_uart(str);
    }
}

int main(int argc, char **argv)
{
    //printf("Hello from ESP!\n");
    print_uart("Hello from ESP!\n");

    for (int i = 0; i < 4; i++) {
        unsigned int *tile_id = (unsigned int *)(unsigned long long)(0x60090180 + i * 0x200 + 0x04);
        print_uart("Tile ");
        print_hex((unsigned long long)(i));
        print_uart(" has ID @ ");
        print_hex((unsigned long long)(tile_id));
        print_uart(" => ");
        print_hex((unsigned long long)(*tile_id));
        print_uart("\n");

        unsigned int *addr = (unsigned int *)(unsigned long long)(0x60090180 + i * 0x200 + 6 * 4);
        print_uart("  Write 0xBEEFCAFE @ ");
        print_hex((unsigned long long)addr);
        *addr = 0xBEEFCAFE;
        print_uart(" and read ");
        print_hex((unsigned long long)(*addr));
        print_uart("\n");
    }

    return 0;
}
