#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#ifndef WIN32
#include <netinet/in.h>
# ifdef _XOPEN_SOURCE_EXTENDED
#  include <arpa/inet.h>
# endif
#include <sys/socket.h>
#endif

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>

static const char MESSAGE[] = "Hello, World!\n";

static const int PORT = 8080;

static void listener_cb(struct evconnlistener *, evutil_socket_t,
		struct sockaddr *, int socklen, void *);
static void conn_writecb(struct bufferevent *, void *);
static void conn_eventcb(struct bufferevent *, short, void *);

static void signal_cb(evutil_socket_t, short, void *);

int main(int argc, char **argv) {
	struct event_base *base;
	struct evconnlistener *listener;
	struct event *signal_event;

	struct sockaddr_in sin;
#ifdef WIN32
	WSADATA wsa_data;
	WSAStartup(0x0201, &wsa_data);
#endif
	
	// configure our event_base to be fast
	struct event_config *cfg;
	cfg = event_config_new();
	event_config_avoid_method(cfg, "select"); // We don't want no select
	event_config_avoid_method(cfg, "poll"); // NO to poll

	base = event_base_new_with_config(cfg); // create a libevent base object for the event loop
	event_config_free(cfg); // all done with this

	if (!base) {
		fprintf(stderr, "Could not initialize libevent!\n");
		return 1;
	}
	
	int i;
	const char **methods = event_get_supported_methods();
	printf("Starting Libevent %s. Supported methods are:\n",
		event_get_version());
	for(i=0; methods[i] != NULL; ++i){
		printf("\t%s\n", methods[i]);
	}
	printf("\nUsing %s.\n", event_base_get_method(base));

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(PORT);

	// create a listener to accept new socket connections
	// -- if there's any event, call listener_cb and run that.
	listener = evconnlistener_new_bind(base, listener_cb, (void *)base,
			LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE, -1,
			(struct sockaddr*)&sin,
			sizeof(sin));

	if (!listener) {
		fprintf(stderr, "Could not create a listener!\n");
		return 1;
	}

	// bind a callback in case of C-c; this is how we exit
	signal_event = evsignal_new(base, SIGINT, signal_cb, (void *)base);

	if (!signal_event || event_add(signal_event, NULL)<0) {
		fprintf(stderr, "Could not create/add a signal event!\n");
		return 1;
	}

	event_base_dispatch(base); // start the event loop

	// if we're here, the event loop is done, and it's time to quit
	evconnlistener_free(listener); // free our listener
	event_free(signal_event); // free our signal event
	event_base_free(base); // free our base

	printf("Done.\n");
	return 0;
}

static void listener_cb(struct evconnlistener *listener, evutil_socket_t fd,struct sockaddr *sa, int socklen, void *user_data) {
	/*
	This function is called anytime there's an 
	event on the listener that was set up in the main loop.
	*/
	struct event_base *base = user_data;
	struct bufferevent *bev;

	bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
	if (!bev) {
		fprintf(stderr, "Error constructing bufferevent!");
		event_base_loopbreak(base);
		return;
	}
	bufferevent_setcb(bev, NULL, conn_writecb, conn_eventcb, NULL);
	bufferevent_enable(bev, EV_WRITE);
	bufferevent_disable(bev, EV_READ);

	int i;
	for(i=0; i<5; i++){
		bufferevent_write(bev, MESSAGE, strlen(MESSAGE));
	}
}

static void conn_writecb(struct bufferevent *bev, void *user_data) {
	struct evbuffer *output = bufferevent_get_output(bev);
	if (evbuffer_get_length(output) == 0) {
		printf("flushed answer\n");
		bufferevent_free(bev);
	}
}

static void conn_eventcb(struct bufferevent *bev, short events, void *user_data) {
	if (events & BEV_EVENT_EOF) {
		printf("Connection closed.\n");
	} else if (events & BEV_EVENT_ERROR) {
		printf("Got an error on the connection: %s\n",
				strerror(errno));/*XXX win32*/
	}
	/* None of the other events can happen here, since we haven't enabled
	 * timeouts */
	bufferevent_free(bev);
}

static void signal_cb(evutil_socket_t sig, short events, void *user_data) {
	struct event_base *base = user_data;
	struct timeval delay = { 0, 0 };

	printf("Caught an interrupt signal; exiting cleanly.\n");

	event_base_loopexit(base, &delay);
}
