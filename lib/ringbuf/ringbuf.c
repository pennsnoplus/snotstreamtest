#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "ringbuf.h"

// Allocate and initialize a ringbuffer
Ringbuf *ringbuf_init(Ringbuf **rb, uint64_t num_keys){
	*rb = malloc(sizeof(Ringbuf));
	(*rb)->keys = malloc(num_keys*sizeof(void *));
	int i;
	if (*rb){
		memset(*rb, 0, sizeof(*rb));
		(*rb)->num_keys = num_keys;
		for(i = 0; i < num_keys; i++){
			(*rb)->keys[i] = NULL;
		}
		(*rb)->write = 0;
		(*rb)->read = 0;
		(*rb)->fill = 0;
	}
	return (*rb);
}

// Free all data in the buffer, as well as the keys structure
void ringbuf_clear(Ringbuf *rb){
	int i;
	for(i=0; i< rb->num_keys; i++){
		if(rb->keys[i] != NULL){
			free(rb->keys[i]);
			rb->keys[i] = NULL;
		}
	}
	free(rb->keys);
	rb->write = 0;
	rb->read = 0;
	rb->fill = 0;
}

// Allocate a new buffer and give it control of all of the argument buffer's keys.
// Only allocates enough space to hold all of the unread keys in the argument buffer,
//   and only actually copies the keys, not the memory they point to.
Ringbuf *ringbuf_copy(Ringbuf *rb){
	Ringbuf *out;
	out = ringbuf_init(&out, rb->fill+1);
	int i;
	uint64_t offset = rb->read;
	for(i = rb->read; i != rb->write; i=(i+1)%(rb->num_keys)){
		out->keys[i-offset] = rb->keys[i];
		rb->keys[i] = NULL;
	}
	out->read = 0;
	out->write = rb->fill;
	out->fill = rb->fill;
	rb->write = 0;
	rb->read = 0;
	rb->fill = 0;
	return out;
}

// Create an exact duplicate of the current ringbuffer,
//   including making a copy of every memory object pointed to
//   by any of the keys.
Ringbuf *ringbuf_dup(Ringbuf *rb){
	Ringbuf *out;
	out = ringbuf_init(&out, rb->num_keys);
	int i;
	for(i = 0; i < rb->num_keys; i++){
		if(rb->keys[i] != NULL){
			size_t memsize = sizeof(*(rb->keys[i]));
			out->keys[i] = calloc(1, memsize);
			memcpy(out->keys[i], rb->keys[i], memsize);
		}
		else {
			out->keys[i] = NULL;
		}
	}
	out->write = rb->write;
	out->read = rb->read;
	out->fill = rb->fill;
	return out;
}

// Print out values of the ringbuf (for debugging).
void ringbuf_status(Ringbuf *rb, char *pref){
	printf("%sRingbuf  %p:\n", pref, rb);
	printf("%s  write: %llu\n", pref, rb->write);
	printf("%s   read: %llu\n", pref, rb->read);
	printf("%s   fill: %llu\n", pref, rb->fill);
	printf("%s   full: %d\n", pref, ringbuf_isfull(rb));
	printf("%s  empty: %d\n", pref, ringbuf_isempty(rb));
}
	
// Check the state of a ringbuf
int ringbuf_isfull(Ringbuf *rb){
	return ( ((rb->write) == (rb->read)) && (rb->fill != 0) );
}
int ringbuf_isempty(Ringbuf *rb){
	return ( ((rb->write) == (rb->read)) && (rb->fill == 0) );
}

// Have the ringbuf keep track of some amount of data by storing
//   the address to it.
int ringbuf_push(Ringbuf *rb, void *memaddr){
	int full = ringbuf_isfull(rb);
	if (!full){
		rb->keys[rb->write] = memaddr;
		rb->write++;
		rb->write %= rb->num_keys;
		rb->fill++;
	}
	return !full;
}
// Copy some data and have the ringbuf keep track of the new copy.
int ringbuf_pushcp(Ringbuf *rb, void *memaddr, size_t size){
	int full = ringbuf_isfull(rb);
	if (!full){
		rb->keys[rb->write] = calloc(1, size);
		memcpy(rb->keys[rb->write], memaddr, size);
		rb->write++;
		rb->write %= rb->num_keys;
		rb->fill++;
	}
	return !full;
}
		
// Read some data from the ringbuf by copying the key into the given destination,
//   removing the key from the ringbuf in the process.
int ringbuf_pop(Ringbuf *rb, void **dest){
	int empty = ringbuf_isempty(rb);
	if (!empty){
		(*dest) = rb->keys[rb->read];
		rb->keys[rb->read] = NULL;
		rb->read++; // note: you can pop a NULL pointer off the end
		rb->read %= rb->num_keys;
		rb->fill--;
	}
	return !empty;
}

// Insert a pointer into an arbitrary location of the ringbuf, using
//   a modulus so that the location is always valid. If that key
//   already exists and the overwrite argument != 0, the memory it
//   points to is freed and the key overwritten.
int ringbuf_insert(Ringbuf *rb, int elem, void *key, int overwrite){
	int loc = elem % rb->num_keys;
	//int full = ringbuf_isfull(rb); // Unused?
	if(rb->keys[loc] != NULL){
		if(overwrite){
			rb->fill--;
			free(rb->keys[loc]);
		}
		else{
			return -1;
		}
	}
	rb->keys[loc] = key;
	rb->fill++;
	return 0;
}

// Grab the key at a specific location, using a modulus
//   to ensure always-valid locations, and store it in
//   dest.
int ringbuf_get(Ringbuf *rb, int elem, void **dest){
	int loc = elem % rb->num_keys;
	*dest = rb->keys[loc];
	return (dest == NULL) ? 1 : 0; // ?: is unnecessary?
}
