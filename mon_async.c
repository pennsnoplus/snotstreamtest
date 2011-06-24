#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <sys/socket.h>

#include <event2/event.h>
#include <event2/dns.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/util.h>

#define EB_HOST "localhost"
#define EB_PORT 8080
#define NUM_CONS 1

typedef struct data_con {
	struct bufferevent *bev;
	char name[50];
	char host[50];
	int port;
} data_con;

void setup_con(data_con *con, char* nm, char *hst, int prt){
	memset(&(con->name), '\0', sizeof con->name);
	memset(&(con->host), '\0', sizeof con->host);
	strncpy(con->name, nm, strlen(nm));
	strncpy(con->host, hst, strlen(hst));
	con->port = prt;
	printf("con->name=%s\n", con->name);
	printf("con->host=%s\n", con->host);
	printf("con->port=%d\n", con->port);
}

void eventcb(struct bufferevent *bev, short events, void *ptr)
{
	if (events & BEV_EVENT_CONNECTED) {
		printf("Connected to 127.0.0.1:8080\n");
		/* We're connected to 127.0.0.1:8080.   Ordinarily we'd do
		   something here, like start reading or writing. */
	} else if (events & BEV_EVENT_ERROR) {
		/* An error occured while connecting. */
		printf("Unable to connect.\n");
	} else if (events & BEV_EVENT_EOF) {
		printf("Connection closed.\n");
	}
}

void readcb(struct bufferevent *bev, void *ptr)
{
	printf("> ");
	char buf[1024];
	int n;
	struct evbuffer *input = bufferevent_get_input(bev);
	while ((n = evbuffer_remove(input, buf, sizeof(buf))) > 0) {
		fwrite(buf, 1, n, stdout);
	}
	printf("\n");
}

void signalcb(evutil_socket_t sig, short events, void *user_data){
	struct event_base *base = user_data;
	struct timeval delay = {0,0};
	printf("Caught an interrupt signal; exiting.\n");
	event_base_loopexit(base, &delay);
}


int main(int argc, char* argv[])
{
	struct event_base *base;
	struct evdns_base *dns_base;
	
	// Array of possible monitoring connections
	printf("creating arry of cons\n");
	data_con cons[NUM_CONS];
	printf("setting up first con\n");
	setup_con(&(cons[0]), "Event Builder", "localhost", 8080);
	printf("finished setting up first con\n");

	//struct bufferevent *bev;
	struct event *signal_event;
	
	struct sockaddr_in sin;

	// Create the bases
	base = event_base_new();
	dns_base = evdns_base_new(base, 1);
	printf("created bases");

	// Create our sockets
	int i;
	for(i = 0; i < NUM_CONS; i++){
		cons[i].bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);

		bufferevent_setcb(cons[i].bev, readcb, NULL, eventcb,(void *) &cons[i]);
		bufferevent_enable(cons[i].bev, EV_READ|EV_WRITE); // enable read and write

		//bufferevent_disable(bev, EV_WRITE);
		if(bufferevent_socket_connect_hostname(
			cons[i].bev, dns_base, AF_INET, cons[i].host, cons[i].port)
			< 0) {
			printf("Unable to connect to %s (%s:%d)\n", cons[i].name, cons[i].host, cons[i].port);
			bufferevent_free(cons[i].bev);
		}
	}


	//memset(&sin, 0, sizeof(sin));
	//sin.sin_family = AF_INET;
	//sin.sin_port = htons(EB_PORT); /* Port 8080 */
	
	//if (bufferevent_socket_connect_hostname(
	//        bev, dns_base, AF_INET, EB_HOST, EB_PORT) < 0) {
		/* Error connecting */
	//	printf("Unable to connect to %s:%d\n", EB_HOST, EB_PORT);
	//	bufferevent_free(bev);
	//	return -1;
	//}

	//if (bufferevent_socket_connect(bev,
	//			(struct sockaddr *)&sin, sizeof(sin)) < 0) {
		/* Error starting connection */
	//	printf("Unable to start connection\n");
	//	bufferevent_free(bev);
	//	return -1;
	//}
	
	// Create a handler for SIGINT (C-c) so that quitting is nice
	signal_event = evsignal_new(base, SIGINT, signalcb, (void *)base);
	if (!signal_event || event_add(signal_event, NULL)<0) {
		fprintf(stderr, "Could not create/add a signal event!\n");
		return 1;
	}

	// Run the event loop
	event_base_dispatch(base);

	// Cleanup and finish
	printf("Done.\n");
	event_free(signal_event);
	event_base_free(base);
	return 0;
}
