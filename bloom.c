/*
 *  Copyright (c) 2012-2016, Jyri J. Virkki
 *  All rights reserved.
 *
 *  This file is under BSD license. See LICENSE file.
 */

/*
 * Refer to bloom.h for documentation on the public interfaces.
 */

#include <assert.h>
#include <fcntl.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "bloom.h"
#include "murmurhash2.h"

#define MAKESTRING(n) STRING(n)
#define STRING(n) #n

#ifndef BLOOM_VERSION
#define BLOOM_VERSION 1.5dev
#endif

inline static int test_bit_set_bit(unsigned char * buf,
        unsigned int x, int set_bit)
{
    unsigned int byte = x >> 3;
    unsigned char c = buf[byte];        // expensive memory access
    unsigned int mask = 1 << (x % 8);

    if (c & mask) {
        return 1;
    } else {
        if (set_bit) {
            buf[byte] = c | mask;
        }
        return 0;
    }
}


static int bloom_check_add(struct bloom * bloom,
        const void * buffer, int len, int add)
{
    if (bloom->ready == 0) {
        printf("bloom at %p not initialized!\n", (void *)bloom);
        return -1;
    }

    int hits = 0;
    register unsigned int a = murmurhash2(buffer, len, 0x9747b28c);
    register unsigned int b = murmurhash2(buffer, len, a);
    register unsigned int x;
    register unsigned int i;

    for (i = 0; i < bloom->hashes; i++) {
        x = (a + i*b) % bloom->bits;
        if (test_bit_set_bit(bloom->bf, x, add)) {
            hits++;
        }
    }

    if (hits == bloom->hashes) {
        return 1;                // 1 == element already in (or collision)
    }

    return 0;
}


int bloom_init_size(struct bloom * bloom, int entries, double error,
        unsigned int cache_size)
{
    return bloom_init(bloom, entries, error);
}


int bloom_init(struct bloom * bloom, int entries, double error)
{
    return bloom_init_buf(bloom, entries, error, NULL);
}

int bloom_init_buf(struct bloom *bloom, int entries, double error, unsigned char *buf){
    bloom->ready = 0;

    if (entries < 1 || error == 0) {
        return 1;
    }

    bloom->entries = entries;
    bloom->error = error;

    double num = log(bloom->error);
    double denom = 0.480453013918201; // ln(2)^2
    bloom->bpe = -(num / denom);

    double dentries = (double)entries;
    bloom->bits = (int)(dentries * bloom->bpe);

    if (bloom->bits % 8) {
        bloom->bytes = (bloom->bits / 8) + 1;
    } else {
        bloom->bytes = bloom->bits / 8;
    }

    bloom->hashes = (int)ceil(0.693147180559945 * bloom->bpe);  // ln(2)

    if(buf) {
        bloom->bf = buf; // caller need to make sure it has enough space
        memset(bloom->bf, 0, bloom->bytes);
    }
    else
        bloom->bf = (unsigned char *)calloc(bloom->bytes, sizeof(unsigned char));
    if (bloom->bf == NULL) {
        return 1;
    }

    bloom->ready = 1;
    return 0;

}

int bloom_check(struct bloom * bloom, const void * buffer, int len)
{
    return bloom_check_add(bloom, buffer, len, 0);
}


int bloom_add(struct bloom * bloom, const void * buffer, int len)
{
    return bloom_check_add(bloom, buffer, len, 1);
}


void bloom_print(struct bloom * bloom)
{
    printf("bloom at %p\n", (void *)bloom);
    printf(" ->entries = %d\n", bloom->entries);
    printf(" ->error = %f\n", bloom->error);
    printf(" ->bits = %d\n", bloom->bits);
    printf(" ->bits per elem = %f\n", bloom->bpe);
    printf(" ->bytes = %d\n", bloom->bytes);
    printf(" ->hash functions = %d\n", bloom->hashes);
}


void bloom_free(struct bloom * bloom)
{
    if (bloom->ready) {
        free(bloom->bf);
    }
    bloom->ready = 0;
}

void bloom_reset(struct bloom *bloom){
    memset(bloom->bf, 0, bloom->bytes);
}

void bloom_load(
        struct bloom * bloom,
        unsigned char *bits, 
        size_t size,  
        int entries,  
        double error){
    bloom_init(bloom, entries, error);
    memcpy(bloom->bf, bits, bloom->bytes);
}


uint8_t* bloom_dump(
        struct bloom * bloom,
        unsigned char *bits, 
        size_t * bytes,  
        int * entries){

    *bytes = bloom->bytes;
    *entries = bloom->entries;
    memcpy(bits, bloom->bf, bloom->bytes);

    return bits;
}



const char * bloom_version()
{
    return MAKESTRING(BLOOM_VERSION);
}
