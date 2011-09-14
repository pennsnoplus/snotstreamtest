#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "node.h"

int main(int argc, char *argv){
	char *m1 = malloc(5);
	char *m2 = malloc(2);
	char *m3 = malloc(5);
	char *m4 = malloc(2);
	
	memcpy(m1, "Hello", 5);
	memcpy(m2, ", ", 2);
	memcpy(m3, "World", 5);
	memcpy(m4, "!\n", 2);

	Node *msg_array = nd_make(NULL);
	
	nd_append(nd_last(msg_array), nd_make(m1));
	nd_append(nd_last(msg_array), nd_make(m2));
	nd_append(nd_last(msg_array), nd_make(m3));
	nd_append(nd_last(msg_array), nd_make(m4));

	Node *tmp;
	nd_foreach_child(tmp, msg_array){
		fputs(tmp->data, stdout);
	}

	nd_destroy(msg_array);

	return 0;
}
