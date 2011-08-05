#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <signal.h>
#include <pthread.h>
#include "ds.h"
#include "listener.h"

void *xl3(void *ptr);

int startport;
struct hostent *server;

int main(int argc, char *argv[])
{
    int i;

    if (argc < 3) {
	startport = 44601;
	server = gethostbyname("localhost");
    }else{

	startport = atoi(argv[2]);
	server = gethostbyname(argv[1]);
    }
    if (server == NULL) {
	fprintf(stderr,"ERROR, no such host\n");
	exit(0);
    }

    pthread_t txl3[19];
    int xl3_num[19];
    for (i=0;i<2;i++){
	xl3_num[i] = i;
	printf("starting crate %d\n",i);
	pthread_create(&txl3[i], NULL, xl3, (void*)&xl3_num[i]);
    }

    for (i=0;i<2;i++){
	pthread_join(txl3[i], NULL);
    }
    return 0;
}

void* xl3(void *ptr){
    int crate = *(int *)ptr;
    printf("thread for crate %d\n",crate);
    FILE *infile;
    char input_file[200];
    char ibuf[10000];
    XL3Packet *ppacket,packet[5];
    sprintf(input_file,"data/xl3%02d.dat",crate);

    struct sockaddr_in serv_addr;
    int sockfd;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
	error("ERROR opening socket");
    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(startport+crate);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0){ 
	printf("Error connecting crate %d\n",crate);
	//exit(0);
    }else{
	printf("connected crate %d\n",crate);
    }
    int n;
    while (1){
	infile = fopen(input_file,"r");
	while(fread(packet,1444,1,infile)){
	    if (packet[0].cmdHeader.packet_num != 0xABCD){
		printf("error: %08x\n",packet[0].cmdHeader.packet_num);
		for (n=0;n<10;n++){
		    printf("%08x ",*(((uint32_t *) packet)+n));
		}
		printf("\n");
	    }
	    n = send(sockfd,packet,sizeof(XL3Packet),0);
	}
	fclose(infile);
    }
}




