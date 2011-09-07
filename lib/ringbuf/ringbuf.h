#ifndef __RINGBUF_H
#define __RINGBUF_H

#include <stdio.h>
#include <string.h>
#include <stdint.h>

typedef struct {
	uint64_t write;		// write pointer
	uint64_t read;		// read pointer
	uint64_t num_keys;	// number of keys in the buffer
	void **keys;		// holds the data
	uint64_t fill;		// fillcount (+writemoves -readmoves);
} Ringbuf;

Ringbuf *ringbuf_init(Ringbuf **rb, uint64_t num_keys);
void ringbuf_clear(Ringbuf *rb);

Ringbuf *ringbuf_copy(Ringbuf *rb);
Ringbuf *ringbuf_dup(Ringbuf *rb);

void ringbuf_status(Ringbuf *rb, char *pref);

int ringbuf_isfull(Ringbuf *rb);
int ringbuf_isempty(Ringbuf *rb);

int ringbuf_push(Ringbuf *rb, void *memaddr);
int ringbuf_pushcp(Ringbuf *rb, void *memaddr, size_t size);

int ringbuf_pop(Ringbuf *rb, void **dest);
int ringbuf_get(Ringbuf *rb, int elem, void **dest);

int ringbuf_insert(Ringbuf *rb, int elem, void *key, int overwrite);
#endif
