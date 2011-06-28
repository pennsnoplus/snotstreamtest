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

#define NUM_CONS 2 // maximum number of ports to be monitored

typedef struct data_con {
	// Holds the data for a single monitoring connection
	struct bufferevent *bev; // holds a buffer event
	char name[50]; // name of this connection (arbitrary)
	char host[50]; // host (e.g., "localhost")
	int port; // port (e.g., 8080)
} data_con;

void setup_con(data_con *con, char* nm, char *hst, int prt){
	// Initializes a data_con structure
	memset(&(con->name), '\0', sizeof con->name);
	memset(&(con->host), '\0', sizeof con->host);
	strncpy(con->name, nm, strlen(nm));
	strncpy(con->host, hst, strlen(hst));
	con->port = prt;
}

void eventcb(struct bufferevent *bev, short events, void *ptr)
{
	data_con con = *((data_con *)(ptr));
	if (events & BEV_EVENT_CONNECTED) {
		printf("Finished request to %s (%s:%d).\n", con.name, con.host, con.port);
		/* Ordinarily we'd do something here, like
		   start reading or writing. */
		if (strncmp(con.name, "mon_control", 11) == 0) { // if the controller is connected
			// TODO: execute controller commands here
		}
	} else if (events & BEV_EVENT_ERROR) {
		/* An error occured while connecting. */
		printf("BEV_EVENT_ERROR: Unable to connect to %s (%s:%d).\n", con.name, con.host, con.port);
	} else if (events & BEV_EVENT_EOF) {
		printf("BEV_EVENT_EOF: Connection to %s (%s:%d) closed.\n", con.name, con.host, con.port);
	}
}

void readcb(struct bufferevent *bev, void *ptr)
{
	data_con con = *((data_con *)(ptr));
	printf("--- %s ---\n", con.name);
	char buf[1024];
	int n;
	struct evbuffer *input = bufferevent_get_input(bev);
	while ((n = evbuffer_remove(input, buf, sizeof(buf))) > 0) {
		fwrite(buf, 1, n, stdout);
	}
}

void signalcb(evutil_socket_t sig, short events, void *user_data){
	struct event_base *base = user_data;
	struct timeval delay = {0,0};
	printf("Caught an interrupt signal; exiting.\n");
	event_base_loopexit(base, &delay);
}


int main(int argc, char* argv[])
{
	int i; // counter variable
	struct event_base *base;		// event loop
	struct evdns_base *dns_base;	// for hostname resolution
	
	// Configure our event_base to be fast
	struct event_config *cfg;
	cfg = event_config_new();
	event_config_avoid_method(cfg, "select"); // We don't want no select
	event_config_avoid_method(cfg, "poll"); // NO to poll

	base = event_base_new_with_config(cfg); // use the custom config
	event_config_free(cfg); // all done with this

	if (!base) {
		fprintf(stderr, "Could not initialize libevent!\n");
		return 1;
	}
	// Show all available methods
	const char **methods = event_get_supported_methods();
	printf("Starting Libevent %s. Supported methods are:\n",
		event_get_version());
	for(i=0; methods[i] != NULL; ++i){
		printf("\t%s\n", methods[i]);
	}
	// Show which method the program is using
	printf("Using %s.\n", event_base_get_method(base));
	
	// Array of possible monitoring connections
	data_con cons[NUM_CONS];
	setup_con(&(cons[0]), "Event Builder", "localhost", 8080);
	setup_con(&(cons[1]), "Second Event Builder", "localhost", 8080);

	struct event *signal_event;
	
	struct sockaddr_in sin;

	// Create the bases
	base = event_base_new();
	dns_base = evdns_base_new(base, 1);

	// Create our sockets
	for(i = 0; i < NUM_CONS; i++){
		cons[i].bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);

		bufferevent_setcb(cons[i].bev, readcb, NULL, eventcb, (void *)&cons[i]);
		bufferevent_enable(cons[i].bev, EV_READ|EV_WRITE); // enable read and write

		if(bufferevent_socket_connect_hostname(
			cons[i].bev, dns_base, AF_INET, cons[i].host, cons[i].port)
				< 0) {
			printf("Unable to connect to %s (%s:%d)\n", cons[i].name, cons[i].host, cons[i].port);
			bufferevent_free(cons[i].bev);
		}
	}

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
