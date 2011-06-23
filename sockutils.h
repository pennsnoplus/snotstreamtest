#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
int connect_to(char* host, char* port){
	/*
		returns a file descriptor which is
		connected to the host on the specified
		port. should work.
	*/ 
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv=getaddrinfo(host, port, &hints, &servinfo)) != 0){
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return -1;
	}

	// loop through all the results and connect if possible
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1){
			perror("socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1){
			close(sockfd);
			perror("connect");
			continue;
		}

		break;
	}

	if (p == NULL) { // didn't connect properly
		fprintf(stderr, "failed to connect\n");
		return -1;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
	printf("connected to %s\n", s);
	freeaddrinfo(servinfo);
	return sockfd;
}


int bind_listener(char *host, int port){
	/*
	   Given a host and a port, return a file descriptor
	   (basically an integer) / socket to listen on that
	   port for new requests (such as clients trying to
	   connect).
	 */
	int rv, listener;
	int yes = 1;
	char str_port[10];
	sprintf(str_port,"%d",port);
	struct addrinfo hints, *ai, *p;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if ((rv = getaddrinfo(host, str_port, &hints, &ai)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return -1;
	}
	for(p = ai; p != NULL; p = p->ai_next) {
		listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (listener < 0) { 
			continue;
		}
		// lose the pesky "address already in use" error message
		setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

		if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
			close(listener);
			continue;
		}
		break;
	}
	freeaddrinfo(ai); // all done with this
	if (p == NULL) {
		// If p == NULL, that means that listener
		// did not get bound to the port
		fprintf(stderr, "failed to bind listener\n");
		return -1;
	}
	else{
		return listener;
	}
}
int accept_connection(int socket, int listener_port){
	/*
	   This function, given a socket and a listener port,
	   accept()s the new socket and returns a file descriptor
	   (an integer) to the new socket.
	 */
	// handle new connections
	struct sockaddr_storage remoteaddr; // client address
	socklen_t addrlen;
	int newfd; // the new file descriptor
	char remoteIP[INET6_ADDRSTRLEN]; // character array to hold the remote IP address
	addrlen = sizeof remoteaddr;
	newfd = accept(socket, (struct sockaddr *)&remoteaddr, &addrlen);
	return newfd;
}
