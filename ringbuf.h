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

// allocate and init
Ringbuf *ringbuf_init(Ringbuf **rb, int num_keys);
// clear
void ringbuf_clear(Ringbuf *rb);
// "copy" (move)
Ringbuf *ringbuf_copy(Ringbuf *rb);
// debugging
void ringbuf_status(Ringbuf *rb);
// state
int ringbuf_isfull(Ringbuf *rb);
int ringbuf_isempty(Ringbuf *rb);
// add / remove
int ringbuf_push(Ringbuf *rb, void *key);
int ringbuf_pop(Ringbuf *rb, void **dest);
int ringbuf_insert(Ringbuf *rb, int elem, void *key, int overwrite);
int ringbuf_get(Ringbuf *rb, int elem, void **dest);
