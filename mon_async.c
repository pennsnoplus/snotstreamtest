#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <sys/socket.h>

#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/util.h>

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
	struct bufferevent *bev;
	struct event *signal_event;
	struct sockaddr_in sin;

	base = event_base_new();

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(8080); /* Port 8080 */

	bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);

	bufferevent_setcb(bev, readcb, NULL, eventcb, NULL);
	bufferevent_enable(bev, EV_READ);
	bufferevent_disable(bev, EV_WRITE);

	if (bufferevent_socket_connect(bev,
				(struct sockaddr *)&sin, sizeof(sin)) < 0) {
		/* Error starting connection */
		printf("Unable to start connection\n");
		bufferevent_free(bev);
		return -1;
	}
	
	signal_event = evsignal_new(base, SIGINT, signalcb, (void *)base);
	if (!signal_event || event_add(signal_event, NULL)<0) {
		fprintf(stderr, "Could not create/add a signal event!\n");
		return 1;
	}
	event_base_dispatch(base);
	printf("Done.\n");
	event_free(signal_event);
	event_base_free(base);
	return 0;
}
