#include "ringbuf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc, char **argv){
	Ringbuf *buf;
	buf = ringbuf_init(&buf, 5);
	int *a = malloc(sizeof(int));
	*a = 0;
	while (!ringbuf_isfull(buf)){
		ringbuf_push(buf, a);
		ringbuf_status(buf);
		printf("a = %d\n", *a);
		(*a)++;
	}
	free(a);

	int isEmpty = 0;
	while( !ringbuf_isempty(buf)){
		ringbuf_pop(buf, (void **)&a);
		printf("%d\n", *a);
		free(a);
		ringbuf_status(buf);
	}

	printf("clearing buf\n");
	ringbuf_clear(buf);
	free(buf);
	return 0;
}
