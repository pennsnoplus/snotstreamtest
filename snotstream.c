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
#include "lib/pouch/pouch.h"

// Defines
#define MSG_OUT stderr

// Globals
int have_controller = 0;


void new_conn(char *url, PouchMInfo *pmi){
	PouchReq *pr;
	pr = pr_init();
	pr_set_url(pr, url);
	pr_set_method(pr, GET);
	pr = pr_domulti(pr, pmi->multi); // send a GET request to the url
	fprintf(MSG_OUT, "Added easy %p to multi %p (%s)\n", pr->easy, pmi->multi, pr->url);
	debug_mcode("new_conn: curl_multi_add_handle", pr->curlmcode);
	/* curl_multi_add_handle() will set a time-out to trigger very soon so
	   that the necessary socket_action() call will be called by this app */
}
void signal_cb(evutil_socket_t sig, short events, void *user_data) {
	struct event_base *base = user_data;
	printf("Caught an interrupt signal; exiting.\n");
	event_base_loopbreak(base);
}
void controller_read_cb(struct bufferevent *bev, void *ctx) {
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
	new_conn(inbuf, ctx); // ctx is a PouchMInfo object. see listener_accept_cb
}
void controller_event_cb(struct bufferevent *bev, short events,void *ctx) {
	if (events & BEV_EVENT_ERROR)
		perror("Error from bufferevent (controller)");
	if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
		bufferevent_free(bev);
		have_controller--;
		fprintf(stderr, "Closed controller connection.\n");
	}
}
void listener_accept_cb(struct evconnlistener *listener,
			       evutil_socket_t fd, struct sockaddr *address,
			       int socklen, void *ctx) {
	// ctx holds the PouchMInfo object. pass this on to every event
	if (!have_controller) {
		struct event_base *base = evconnlistener_get_base(listener);
		struct bufferevent *bev =
		    bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
		bufferevent_setcb(bev, controller_read_cb, NULL,
				  controller_event_cb, ctx);
		bufferevent_enable(bev, EV_READ | EV_WRITE);
		have_controller++;
	} else {
		EVUTIL_CLOSESOCKET(fd);
		fprintf(stderr, "Closed extra controller connection\n");
	}
}
void listener_error_cb(struct evconnlistener *listener, void *ctx) {
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
	event_config_avoid_method(cfg, "select");
	event_config_avoid_method(cfg, "poll");

	// Create the event base.
	struct event_base *base = event_base_new_with_config(cfg);
	event_config_free(cfg);
	if (!base) {
		fprintf(stderr, "Could not initialize libevent!\n");
		return 1;
	}
	
	// Create the PouchMInfo object for use in callbacks
	PouchMInfo *pmi = (PouchMInfo *)malloc(sizeof(PouchMInfo));
	memset(pmi, 0, sizeof(*pmi));
	pmi = pouch_multi_init(pmi, base); // after this, pmi->base points *base

	// Create a dns base for our network listener
	struct evdns_base *dnsbase = evdns_base_new(pmi->base, 1);
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
	printf("\t ... Using %s.\n", event_base_get_method(pmi->base));

	// Create the listener
	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;	/* This is an INET address */
	sin.sin_addr.s_addr = htonl(0);	/* Listen on 0.0.0.0 */
	sin.sin_port = htons(port);	/* Listen on the given port. */
	struct evconnlistener *listener = evconnlistener_new_bind(pmi->base, listener_accept_cb, pmi,
					   LEV_OPT_CLOSE_ON_FREE |
					   LEV_OPT_REUSEABLE, -1,
					   (struct sockaddr *)&sin,
					   sizeof(sin));
	if (!listener) {
		perror("Couldn't create listener");
		return 1;
	}
	evconnlistener_set_error_cb(listener, listener_error_cb);

	// Create a signal watcher for C-c (SIGINT)
	struct event *signal_event;
	signal_event = evsignal_new(base, SIGINT, signal_cb, (void *)base);
	if (!signal_event || event_add(signal_event, NULL) < 0) {
		fprintf(stderr, "Could not create/add a signal event!\n");
		return 1;
	}


	// Run the main loop
	event_base_dispatch(pmi->base);
	
	// Clean up
	event_del(signal_event);
	free(signal_event);
	evdns_base_free(dnsbase, 0);
	pouch_multi_delete(pmi); // frees 'base', too
	
	// Exit
	return 0;
}
