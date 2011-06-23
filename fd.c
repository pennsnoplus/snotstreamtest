#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "packet_types.h"
#include "sockutils.h"

char randomChar(){ // randomly generate a character
	return (char)((rand()*1.0/(RAND_MAX+1.0))*93 + 33);
}
int main(int argc, char *argv[]){
	int listener; 	// fd for listening for new connections
	int monitor;	// fd for the new connection
	
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;					// connector's size
	char s[INET6_ADDRSTRLEN];

	char *host = "localhost";
	int port = 3490;
	listener = bind_listener(host, port);
	
	int backlog = 0;
	if (listen(listener, backlog) ==  -1){
		fprintf(stderr, "server: listener error");
		exit(-1);
	}

	printf("server: waiting for connections...\n");
	
	while(1){	// main accept() loop
		sin_size = sizeof their_addr;
		monitor = accept(listener, (struct sockaddr *)&their_addr, &sin_size);
		if (monitor == -1){
			fprintf(stderr, "server: accept error\n");
			continue;
		}
	
		inet_ntop(their_addr.ss_family, // get hostname, store it in s
				get_in_addr((struct sockaddr *)&their_addr),
				s, sizeof s);
	
		printf("server: got connection from %s\n", s);
		
		//prepare the packet data
		
		xl3Packet xl3_data;
		strncpy(xl3_data.head, "1111", sizeof xl3_data.head);
		memset(&(xl3_data.body), '0', sizeof xl3_data.body);//randomChar(), sizeof xl3_data.body);

		mtcPacket mtc_data;
		strncpy(mtc_data.head, "0011", sizeof mtc_data.head);
		memset(&(mtc_data.body), '0', sizeof mtc_data.body);//randomChar(), sizeof mtc_data.body);
		
		caenPacket caen_data;
		strncpy(caen_data.head, "0111", sizeof caen_data.head);
		memset(&(caen_data.body), '0', sizeof caen_data.body);//randomChar(), sizeof caen_data.body);

		
		//send the data
		if(send(monitor, &xl3_data, sizeof xl3_data, 0) == -1){
			perror("send");
		}
		if(send(monitor, &mtc_data, sizeof mtc_data, 0) == -1){
			perror("send");
		}
		if(send(monitor, &caen_data, sizeof caen_data, 0) == -1){
			perror("send");
		}
		if(send(monitor, &caen_data, sizeof caen_data, 0) == -1){
			perror("send");
		}
		if(send(monitor, &xl3_data, sizeof xl3_data, 0) == -1){
			perror("send");
		}
		if(send(monitor, &mtc_data, sizeof mtc_data, 0) == -1){
			perror("send");
		}
		printf("server: finished sending messages\n");

		close(monitor);

	}
	return 0;
}
