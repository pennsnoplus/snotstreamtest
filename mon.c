#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pillowtalk.h"

#include "packet_types.h"
#include "sockutils.h"


int main(int argc, char *argv[]){
	int server; // fd for the server we're connecting to
	
	char buf[MAXBUFSIZE]; // holds incoming data
	int numbytes;

	//if (argc != 2){
	//	fprintf(stderr, "usage: client hostname\n");
	//	exit(-1)
	//}
	
	char *host = "localhost";
	char *port = "3490";
	server = connect_to(host, port);

	if (server == -1) {
		fprintf(stderr, "could not connect to %s:%s\n", host, port);
		exit(-1);
	}
	memset(&buf, '\0', sizeof buf);
	while(1){
		if((numbytes = recv(server,buf,MAXBUFSIZE,0))==-1){
			perror("recv");
			exit(-1);
		}
		else {
			printf("numbytes=%d\n", numbytes);
			printf("%s\n", buf);
			memset(&buf, '\0', sizeof buf);
		}
		if (numbytes == 0){
			close(server);
			exit(-1);
		}
	}
	return 0;
}
