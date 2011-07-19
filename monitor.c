#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>
#include <stdio.h>

#include <event2/event.h>
#include <event2/dns.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/listener.h>

#include "monitor.h"

void setup_con(data_con *con, char *host, int port, con_type type);
void eventcb(struct bufferevent *bev, short events, void *ptr);
void readcb(struct bufferevent *bev, void *ptr);
void signalcb(evutil_socket_t sig, short events, void *user_data);
void listener_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sa,
		int socklen, void *user_data);

data_con controller_con; // the controller connection
int have_controller = 0; // so far, we don't have one

void print_connected(char *command){
    printf("nobody is conected\n");
}
void quit(char *command){
    raise(SIGINT);
}
/*
   void print_three_times(char *command){
    char *data = strchr(command, ' ');
    printf("data: %s\n", data);
    if (*data){
        int i;
        for(i = 0; i < 3; i++){
            printf("%s\n", data);
        }
    }
    else {
        printf("error: invalid format\n");
    }
}
*/
int get_num_tokens(char *in, char *toks){
    int num;
    char *pch;
    pch = strok(in, toks);
    while (pch != NULL){
        num++;
        pch = strtok(NULL, toks);
    }
    num++;
    return num;
}

void connect_to(char *command){
    char *host;
    int port;
    con_type type;
    int c;
    while(1){
        static struct option long_options[] = {
            /* These options set a flag */
            /* These options don't set a flag */
            {"host", required_argument, 0, 'h'},
            {"port", required_argument, 0, 'p'},
            {"type", required_argument, 0, 't'},
            {0, 0, 0, 0}
        };
        int option_index = 0;
        int argc = get_num_tokens(command, " ");

        c = getopt_long(argc, command, "h:p:t:", long_options, &option_index);
        if (c == -1){
            break;
        }
        switch (c){
            case 0:
                /* do nothing */
                break;
            case 'h':
                host = optarg;
                break;
            case 'p':
                port = atoi(optarg);
                break;
            case 't':
                if (strcmp(optar


command valid_commands[] = {
    {"print_connected", (com_ptr)&print_connected},
    //{"print_three_times", (com_ptr)(&print_three_times)},
    {"quit", (com_ptr)&quit},
};

int main(int argc, char* argv[])
{
	int i; // counter variable
	struct event_base *base;			// event loop
	struct evdns_base *dns_base;		// for hostname resolution
	struct evconnlistener *listener;	// listens for controller
	struct event *signal_event;			// listens for C-c
	struct sockaddr_in sin;

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
    free((char **)methods);
	// Show which method the program is using
	printf("Using %s.\n", event_base_get_method(base));

	// Array of possible monitoring connections
	data_con cons[NUM_CONS];
	setup_con(&(cons[0]), "localhost", 8080, EV_BUILDER);
	setup_con(&(cons[1]), "localhost", 8080, XL3);
	setup_con(&(cons[2]), "localhost", 8080, MTC);

	// Create the bases
	dns_base = evdns_base_new(base, 1);

	// new listener to accept the controller's connection
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(CONT_PORT);

	listener = evconnlistener_new_bind(base, listener_cb, (void *)base,
			LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE, -1,
			(struct sockaddr*) &sin,
			sizeof(sin));

	if (!listener) {
		fprintf(stderr, "Could not create a listener!\n");
		return 1;
	}
	printf("Finished creating listener\n");

	// Create our sockets
	for(i = 0; i < NUM_CONS; i++){
		cons[i].bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);

		bufferevent_setcb(cons[i].bev, readcb, NULL, eventcb, (void *)&cons[i]);
		bufferevent_enable(cons[i].bev, EV_READ|EV_WRITE); // enable read and write

		if(bufferevent_socket_connect_hostname(
					cons[i].bev, dns_base, AF_INET, cons[i].host, cons[i].port)
				< 0) {
			printf("Unable to connect to %s:%d (type %d)\n", cons[i].host, cons[i].port, cons[i].type);
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
	evconnlistener_free(listener); // free our listener
	event_free(signal_event);
    evdns_base_free(dns_base, 0);
	event_base_free(base);
	printf("Done.\n");
	return 0;
}

void listener_cb(struct evconnlistener *listener, evutil_socket_t fd,
        struct sockaddr *sa, int socklen, void *user_data) {
	if (have_controller == 1) {
		EVUTIL_CLOSESOCKET(fd); // close the connection
		fprintf(stderr, "Already have a connected controller\n");
		return;
	}
	else {
		printf("Connecting a new controller...\n");
	}
	// at this point, we need to connect a new controller
	struct event_base *base = user_data;
	// create the new bev_socket
	controller_con.bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
	// give it a name
	setup_con(&(controller_con), "", 0, CONTROLLER);
	if (!controller_con.bev) {
		fprintf(stderr, "Error constructing controller bufferevent!\n");
		event_base_loopbreak(base);
		return;
	}
	have_controller = 1; // we've now connected a controller
	// interested in hearing about reads, not writes
	printf("Controller has connected.\n");
	bufferevent_setcb(controller_con.bev, readcb, NULL, eventcb, &controller_con);
	bufferevent_enable(controller_con.bev, EV_READ|EV_WRITE);
}

void setup_con(data_con *con, char* host, int port, con_type type){
	// Initializes a data_con structure
	memset(&(con->host), '\0', sizeof con->host);
	strncpy(con->host, host, strlen(host));
	con->port = port;
    con->type = type;
}

void eventcb(struct bufferevent *bev, short events, void *ptr){
	data_con con = *((data_con *)(ptr));
	if (events & BEV_EVENT_CONNECTED) {
		printf("Finished connection request to %s:%d (type %d).\n",
                con.host, con.port, con.type);
	} 
	else {
		if (events & BEV_EVENT_ERROR) {
			/* An error occured while connecting. */
			printf("BEV_EVENT_ERROR: Unable to connect to %s:%d (type %d).\n",
                    con.host, con.port, con.type);
		}
		if (events & BEV_EVENT_EOF) {
            if(con.type == CONTROLLER){
				have_controller = 0;
			}
			printf("BEV_EVENT_EOF: Connection to %s:%d (type %d) closed.\n",
                    con.host, con.port, con.type);
		}
		bufferevent_free(con.bev);
	}
}

void readcb(struct bufferevent *bev, void *ptr)
{
	data_con con = *((data_con *)(ptr));

	char inbuf[MAXBUFSIZE];
    memset(&inbuf, 0, MAXBUFSIZE);
	int n;
	struct evbuffer *input = bufferevent_get_input(bev);
	while ((n = evbuffer_remove(input, inbuf, sizeof(inbuf))) > 0) {
        // this copies the data from input to the inbuf
	}
    if (con.type == CONTROLLER){
		bufferevent_write(bev, "Welcome, controller.\n", 21);
        int i;
        command com;
        int is_valid = 0;
        for (i = 0; i < sizeof(valid_commands)/sizeof(command); i++){
            com = valid_commands[i];
            if (strncmp(com.key, inbuf, strlen(com.key)) == 0){
                printf("Valid command %s=%s.\n", inbuf,com.key);
                com.function(inbuf);
                is_valid++;
            }
        }
        if (!is_valid){
            printf("%s is not a valid command.\n", inbuf);
        }
    }
    else{
        printf("should upload the data now...\n");
        // do_upload(ptr, inbuf);
    }
}

void signalcb(evutil_socket_t sig, short events, void *user_data){
    struct event_base *base = user_data;
    struct timeval delay = {0,0};
    printf("Caught an interrupt signal; exiting.\n");
    event_base_loopexit(base, &delay);
}
