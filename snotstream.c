// Standard Libraries
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <arpa/inet.h>
#include <time.h>

// Libevent and Libcurl
#include <event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/dns.h>
#include <event2/util.h>
#include <event2/event.h>
#include <event2/event_struct.h>
#include <curl/curl.h>

// Custom
#include "snotstream.h"

// Defines
#define MSG_OUT stderr

// Structs / Types
/* Information associated with a specific easy handle */
typedef struct _ConnInfo {
	CURL *easy;	// easy handle
	char *url;	// destination;
	char error[CURL_ERROR_SIZE];
} ConnInfo;

/* Information associated with a specific socket */
typedef struct _SockInfo {
	curl_socket_t sockfd;	// socket to be monitored
	struct event ev;		// event on the socket
	int ev_is_set;			// whether or not ev is set and being monitored
	int action;				// what action libcurl wants done
} SockInfo;

/* Update the event timer after curl_multi library calls */
static int multi_timer_cb(CURLM *multi, long timeout_ms, void *data){
	struct timeval timeout;
	timeout.tv_sec = timeout_ms/1000;
	timeout.tv_usec = (timeout_ms%1000)*1000;
	fprintf(MSG_OUT, "multi_timer_cb: Setting timeout to %ld ms\n", timeout_ms);
	if (evtimer_pending(&timer_event, NULL)){
		evtimer_del(&timer_event);
	}
	evtimer_add(&timer_event, &timeout);
	return 0;
}

/* Used to debug return codes from curl_multi_socket_action */
static void debug_mcode(const char *desc, CURLMcode code){
	if ((code != CURLM_OK) && (code != CURLM_CALL_MULTI_PERFORM)){
		const char *s;
		switch (code) {
			//case CURLM_CALL_MULTI_PERFORM:	s="CURLM_CALL_MULTI_PERFORM";break; //ignore
			case CURLM_BAD_HANDLE:			s="CURLM_BAD_HANDLE";break;
			case CURLM_BAD_EASY_HANDLE:		s="CURLM_BAD_EASY_HANDLE";break;
			case CURLM_OUT_OF_MEMORY:		s="CURLM_OUT_OF_MEMORY";break;
			case CURLM_INTERNAL_ERROR:		s="CURLM_INTERNAL_ERROR";break;
			case CURLM_UNKNOWN_OPTION:		s="CURLM_UNKNOWN_OPTION";break;
			case CURLM_LAST:				s="CURLM_LAST";break;
			case CURLM_BAD_SOCKET:			s="CURLM_BAD_SOCKET";break;
			default:						s="CURLM_unknown";
		}
		fprintf(MSG_OUT, "ERROR: %s returns %s\n", desc, s);
	}
}
			
/* Check for completed transfers, and remove their easy handles */
static void check_multi_info(){
	CURLMsg *msg;
	CURL *easy;
	CURLcode res;
	
	int msgs_left;
	ConnInfo *conn;

	while ((msg = curl_multi_info_read(multi, &msgs_left))){
		if(msg->msg == CURLMSG_DONE){ // if this action is done
			// unpack the message
			easy = msg->easy_handle;
			res = msg->data.result;
			curl_easy_getinfo(easy, CURLINFO_PRIVATE, &conn);
			// get rid of the easy handle
			curl_multi_remove_handle(multi, easy);
			curl_easy_cleanup(easy);
			// clean up the conn
			free(conn->url);
			free(conn);
		}
	}
}

/* Called by libevent when there is action on a socket being watched
by the libcurl multi interface */
static void event_cb(int fd, short kind, void *userp){
	CURLMcode rc; // result from curl_multi_socket_action
	int action = 
		(kind & EV_READ ? CURL_CSELECT_IN : 0) |
		(kind & EV_WRITE ? CURL_CSELECT_OUT : 0);
	rc = curl_multi_socket_action(multi, fd, action, &still_running);
	
	debug_mcode("event_cb: curl_multi_socket_action", rc);
	
	check_multi_info();
	if (still_running <= 0){ // last transfer is done
		if (evtimer_pending(&timer_event, NULL)){
			evtimer_del(&timer_event); // get rid of the libevent timer
		}
	}
}


/* Called by libevent when our timeout expires */
static void timer_cb(int fd, short kind, void *userp){
	CURLMcode rc; // result from curl_multi_socket_action
	rc = curl_multi_socket_action(multi, CURL_SOCKET_TIMEOUT, 0, &still_running);
	debug_mcode("timer_cb: curl_multi_socket_action", rc);
	check_multi_info();
}

/* Sets up a SockInfo structure and starts libevent monitoring. */
static void setsock(SockInfo *fdp, curl_socket_t s, int action){
	int kind =
		(action&CURL_POLL_IN ? EV_READ:0)|
		(action&CURL_POLL_OUT ? EV_WRITE:0)|
		EV_PERSIST; // always want persist
	fdp->action = action;
	fdp->sockfd = s;
	if (fdp->ev_is_set){
		event_del(&fdp->ev);
		fdp->ev_is_set = 0;
	}
	event_set(&fdp->ev, fdp->sockfd, kind, event_cb, NULL);
	event_base_set(base, &fdp->ev); // set the event to use the global event base
	fdp->ev_is_set = 1; // mark the event as set
	event_add(&fdp->ev, NULL); // add the event with no timeout (NULL)
}

/* The CURLMOPT_SOCKETFUNCTION. This is what tells libevent to start or stop
watching for events on different sockets. */
static int sock_cb(CURL *e, curl_socket_t s, int action, void *cbp, void *sockp){
	/*
	e       = easy handle that the callback happened on,
	s       = actual socket that is involved,
	action  = what to check for / do (?) (nothing, IN, OUT, INOUT, REMOVE)
	cbp     = private callback pointer (from where? ignored!)
	socketp = private data set by curl_multi_assign, NOT with easy_setopt(CURLOPT_PRIVATE)
	*/
	SockInfo *fdp = (SockInfo *)sockp;
	if (action == CURL_POLL_REMOVE){
		// stop watching this socket for events
		if(fdp){
			if(fdp->ev_is_set){
				event_del(&fdp->ev);
			}
			free(fdp);
		}
	}
	else {
		if (!fdp) {
			// start watching this socket for events
			SockInfo *fdp = calloc(1, sizeof(SockInfo));
			setsock(fdp, s, action);
			curl_multi_assign(multi, s, fdp);
		}
		else {
			// reset the sock with the new type of action
			setsock(fdp, s, action);
		}
	}
	return 0;
}


/* The function to deal with data received from an easy_handle (read data in)*/
static size_t write_cb(void *ptr, size_t size, size_t nmemb, void *data){
	// data is the conn that this thing came from
	return size*nmemb; // do nothing, claim that the data was read
}

/* Create a new easy handle, and add it to the global curl_multi.
In the future, replace this by updating pouch and letting it work with
the libcurl multi interface. Make sure to include the CURLOPT_PRIVATE setting,
and change everything over from ConnInfo to the POUCH equivalent? TODO*/
static void new_conn(char *url){
	ConnInfo *conn;
	CURLMcode rc;
	conn = calloc(1, sizeof(ConnInfo));
	conn->error[0]='\0'; // null terminate the string ? may not be necessary... (TODO remove, not necessary)
	conn->easy = curl_easy_init();
	if (!conn->easy){
		fprintf(MSG_OUT, "curl_easy_init() failed, exiting!\n");
		exit(2);
	}
	conn->url = strdup(url);
	curl_easy_setopt(conn->easy, CURLOPT_URL, conn->url);
	curl_easy_setopt(conn->easy, CURLOPT_WRITEFUNCTION, write_cb);
	curl_easy_setopt(conn->easy, CURLOPT_WRITEDATA, &conn);
	curl_easy_setopt(conn->easy, CURLOPT_VERBOSE, 1L);
	curl_easy_setopt(conn->easy, CURLOPT_ERRORBUFFER, conn->error);
	curl_easy_setopt(conn->easy, CURLOPT_PRIVATE, conn);
	curl_easy_setopt(conn->easy, CURLOPT_NOPROGRESS, 1L);
	fprintf(MSG_OUT, "Adding easy %p to multi %p (%s\n", conn->easy, multi, url);
	rc = curl_multi_add_handle(multi, conn->easy);
	debug_mcode("new_conn: curl_multi_add_handle", rc);
	/* curl_multi_add_handle() will set a time-out to trigger very soon so
	   that the necessary socket_action() call will be called by this app */
}

/* Used to catch C-c interrupt signals and exit cleanly */
void signal_cb(evutil_socket_t sig, short events, void *user_data) {
	struct event_base *base = user_data;
	printf("Caught an interrupt signal; exiting.\n");
	event_base_loopbreak(base);
}

/* Reads stuff in from the controller, and then does stuff with it. Right now,
it's just a demo function, doesn't do anything useful re: data handling / transfer.
Temporarily, it accepts URL's, and performs GET requests on them. */
static void controller_read_cb(struct bufferevent *bev, void *ctx) {
	struct evbuffer *input = bufferevent_get_input(bev);
	char inbuf[100]; // arbitrary sizing
	memset(&inbuf, 0, sizeof(inbuf));
	int n;
	while ((n = evbuffer_remove(input, inbuf, sizeof(inbuf))) > 0) {
		// this copies the data from input to the inbuf
	}
	printf("Controller: %s", inbuf);
	if (inbuf[strlen(inbuf) - 1] != '\n')
		fputc('\n', stdout);
	
	// create new curl request here
	new_conn(inbuf);
}

/* Handles weird events from the controller, as opposed to data cons */
static void controller_event_cb(struct bufferevent *bev, short events,void *ctx) {
	if (events & BEV_EVENT_ERROR)
		perror("Error from bufferevent (controller)");
	if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
		bufferevent_free(bev);
		have_controller--;
		fprintf(stderr, "Closed controller connection.\n");
	}
}

/* This listener accepts connections from the controller. */
static void listener_accept_cb(struct evconnlistener *listener,
			       evutil_socket_t fd, struct sockaddr *address,
			       int socklen, void *ctx) {
	if (!have_controller) {
		struct event_base *base = evconnlistener_get_base(listener);
		struct bufferevent *bev =
		    bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
		bufferevent_setcb(bev, controller_read_cb, NULL,
				  controller_event_cb, NULL);
		bufferevent_enable(bev, EV_READ | EV_WRITE);
		have_controller++;
	} else {
		EVUTIL_CLOSESOCKET(fd);
		fprintf(stderr, "Closed extra controller connection\n");
	}
}

/* Error function for the listener */
static void listener_error_cb(struct evconnlistener *listener, void *ctx) {
	struct event_base *base = evconnlistener_get_base(listener);
	int err = EVUTIL_SOCKET_ERROR();
	fprintf(stderr, "Got an error %d (%s) on the listener. "
		"Shutting down.\n", err, evutil_socket_error_to_string(err));
	event_base_loopexit(base, NULL);
}

int main(int argc, char **argv){
	// Setup the port
	int port = 2020;
	if (argc > 1) {
		port = atoi(argv[1]);
	}
	if (port <= 0 || port > 65535) {
		puts("Invalid port");
		return 1;
	}
	// Configure our event_base to be fast
	struct event_config *cfg;
	cfg = event_config_new();
	event_config_avoid_method(cfg, "select"); // avoid select
	event_config_avoid_method(cfg, "poll"); // avoid poll

	base = event_base_new_with_config(cfg);	// use the custom config
	event_config_free(cfg);	// all done with this
	if (!base) {
		fprintf(stderr, "Could not initialize libevent!\n");
		return 1;
	}
	dnsbase = evdns_base_new(base, 1);
	if (!dnsbase) {
		fprintf(stderr, "Could not create a dns base\n");
		return 2;
	}
	// Show all available methods and the one that was chosen
	const char **methods = event_get_supported_methods();
	printf("Starting sno+stream. Using libevent %s. Supported methods are:\n",
	       event_get_version());
	int i;
	for (i = 0; methods[i] != NULL; ++i) {
		printf("\t%s\n", methods[i]);
	}
	free((char **)methods);
	printf("\t ... Using %s.\n", event_base_get_method(base));

	// Create the listener
	struct sockaddr_in sin;
	/* Clear the sockaddr before using it, in case there are extra
	 * platform-specific fields that can mess us up. */
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;	/* This is an INET address */
	sin.sin_addr.s_addr = htonl(0);	/* Listen on 0.0.0.0 */
	sin.sin_port = htons(port);	/* Listen on the given port. */
	listener = evconnlistener_new_bind(base, listener_accept_cb, NULL,
					   LEV_OPT_CLOSE_ON_FREE |
					   LEV_OPT_REUSEABLE, -1,
					   (struct sockaddr *)&sin,
					   sizeof(sin));
	if (!listener) {
		perror("Couldn't create listener");
		return 1;
	}
	evconnlistener_set_error_cb(listener, listener_error_cb);

	// Create a signal watcher
	struct event *signal_event;	// listens for C-c
	signal_event = evsignal_new(base, SIGINT, signal_cb, (void *)base);
	if (!signal_event || event_add(signal_event, NULL) < 0) {
		fprintf(stderr, "Could not create/add a signal event!\n");
		return 1;
	}

	// Setup libcurl
	multi = curl_multi_init();
	evtimer_set(&timer_event, timer_cb, NULL);
	event_base_set(base, &timer_event); // set the event to use the global event base
	// .... setup the generic multi interface options we want
	curl_multi_setopt(multi, CURLMOPT_SOCKETFUNCTION, sock_cb);
	curl_multi_setopt(multi, CURLMOPT_SOCKETDATA, NULL);
	curl_multi_setopt(multi, CURLMOPT_TIMERFUNCTION, multi_timer_cb);
	curl_multi_setopt(multi, CURLMOPT_TIMERDATA, NULL);

	// Run the main loop
	event_base_dispatch(base);
	
	// Clean up
	event_del(signal_event);
	event_del(&timer_event);
	free(signal_event);
	evdns_base_free(dnsbase, 0);
	event_base_free(base);
	
	// Exit
	return 0;
}
