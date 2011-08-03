#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "ringbuf.h"
// allocate and init
Ringbuf *ringbuf_init(Ringbuf **rb, int num_keys){
	*rb = malloc(sizeof(Ringbuf));
	(*rb)->keys = malloc(num_keys*sizeof(void *));
	int m_alloced = sizeof(Ringbuf)+num_keys*sizeof(void *);
	int i;
	if (*rb){
		printf("Initializing ring buffer: keys[%d] (%d KB allocated)\n", num_keys, m_alloced/1024);
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

// clear
void ringbuf_clear(Ringbuf *rb){
	int i;
	for(i=0; i< rb->num_keys; i++){
		if(rb->keys[i] != NULL){
			free(rb->keys[i]);
		}
		rb->keys[i] = NULL;
	}
	free(rb->keys);
	rb->write = 0;
	rb->read = 0;
	rb->fill = 0;
}
Ringbuf *ringbuf_copy(Ringbuf *rb){
	Ringbuf *out;
	out = ringbuf_init(&out, rb->num_keys);
	int i;
	for(i = rb->read; i != rb->write; i=(i+1)%(rb->num_keys)){
		out->keys[i] = rb->keys[i]; // now out is in charge of the memory
		rb->keys[i] = NULL;
	}
	out->write = rb->write;
	out->read = rb->read;
	out->fill = rb->fill;
	rb->write = 0;
	rb->read = 0;
	rb->fill = 0;
	return out;
}

// debugging
void ringbuf_status(Ringbuf *rb, char *pref){
	printf("%sRingbuf at %p:\n", pref, rb);
	printf("%s  write: %lu\n", pref, rb->write);
	printf("%s   read: %lu\n", pref, rb->read);
	printf("%s   fill: %lu\n", pref, rb->fill);
	printf("%s   full: %d\n", pref, ringbuf_isfull(rb));
	printf("%s  empty: %d\n", pref, ringbuf_isempty(rb));
}
	
// state
int ringbuf_isfull(Ringbuf *rb){
	return ( ((rb->write) == (rb->read)) && (rb->fill != 0) );
}
int ringbuf_isempty(Ringbuf *rb){
	return ( ((rb->write) == (rb->read)) && (rb->fill == 0) );
}
// add / remove
int ringbuf_push(Ringbuf *rb, void *key, size_t size){
	int full = ringbuf_isfull(rb);
	if (!full){
		rb->keys[rb->write] = key;
		//rb->keys[rb->write] = malloc(sizeof(key)); // allocate some space
		//memcpy(rb->keys[rb->write], key, sizeof(key));
		//rb->keys[rb->write] = malloc(size);
		//memcpy(rb->keys[rb->write], key, size);
		rb->write++;
		rb->write %= rb->num_keys;
		rb->fill++;
	}
	return !full;
}
int ringbuf_pop(Ringbuf *rb, void **dest){
	int empty = ringbuf_isempty(rb);
	if (!empty){
		(*dest) = rb->keys[rb->read];
		rb->keys[rb->read] = NULL;
		//memcpy((*dest), rb->keys[rb->read], sizeof(rb->keys[rb->read]));
		//if(rb->keys[rb->read] != NULL){
			//free(rb->keys[rb->read]);
		//}
		rb->read++; // note: you can pop a NULL pointer off the end
		rb->read %= rb->num_keys;
		rb->fill--;
	}
	return !empty;
}
int ringbuf_insert(Ringbuf *rb, int elem, void *key, int overwrite){
	int loc = elem % rb->num_keys;
	int full = ringbuf_isfull(rb);
	if (!full || overwrite) {
		if(rb->keys[loc] != NULL){
			rb->fill--;
			free(rb->keys[loc]);
		}
		rb->keys[loc] = key;
		rb->fill++;
		return 0;
	}
	return 1;
}
int ringbuf_get(Ringbuf *rb, int elem, void **dest){
	int loc = elem % rb->num_keys;
	*dest = rb->keys[loc];
	return (dest == NULL) ? 1 : 0; // ?: is unnecessary?
}
