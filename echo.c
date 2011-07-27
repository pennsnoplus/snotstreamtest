// Standard Libraries
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <arpa/inet.h>

// Libevent
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/dns.h>
#include <event2/util.h>
#include <event2/event.h>

// Custom
#include "echo.h"
#include "lib/json/json.h"
#include "lib/pouch/pouch.h"

// Helper Functions
void delete_con(connection * con) {
	if (con->type)
		free(con->type);
	if (con->host)
		free(con->host);
	if (con->bev)
		bufferevent_free(con->bev);
	con->valid = 0;
	cur_mon_con = 0;
	while (monitoring_cons[cur_mon_con].valid && cur_mon_con < MAX_MON_CONS) {
		cur_mon_con++;
	}
}

// Commands
void help(char *UNUSED, void *_UNUSED){
	int i;
	puts("Commands:");
	for(i = 0; controller_coms[i].key; i++){
		puts(controller_coms[i].key);
	}
}

void print_cons(char *UNUSED, void *_UNUSED) {
	if (cur_mon_con != 0) {
		printf("cur_mon_con= %d\n", cur_mon_con);
		int i;
		connection *con;
		for (i = 0; i < cur_mon_con; i++) {
			con = &monitoring_cons[i];
			if (con->valid) {
				printf("%s %s:%d\n", con->type, con->host,
				       con->port);
			}
		}
	} else {
		printf("No monitoring connections.\n");
	}
}

void stop_con(char *inbuf, void *UNUSED) {
	if (!cur_mon_con)
		printf("No connections to stop.\n");
	else {
		char *type;
		char *host;
		char *portstr;
		int i;
		portstr = strtok(inbuf, " "); // get rid of command name
		for (i = 0; i < 3 && portstr != NULL; i++) {
			portstr = strtok(NULL, " ");
			if (portstr == NULL)
				break;
			if (i == 0)
				type = portstr;
			else if (i == 1)
				host = portstr;
		}
		int error;
		if (i != 3) {
			printf("Incorrect number of arguments\n");
			printf("stop <type> <host> <port>\n");
		} else {
			int port = atoi(portstr);
			int j;
			connection *con;
			for (j = 0; j < cur_mon_con; j++) {
				con = &monitoring_cons[j];
				if (!strcmp(con->type, type)) {
					if (!strcmp(con->host, host)) {
						if (con->port == port) {
							printf
							    ("Stopped monitoring %s %s:%d\n",
							     con->type,
							     con->host,
							     con->port);
							delete_con(con);
							break;
						}
					}
				}
			}
		}
	}
}

void start_con(char *inbuf, void *UNUSED) {
	if (cur_mon_con < MAX_MON_CONS) {	// we can still make new cons
		connection *con = &monitoring_cons[cur_mon_con];
		con->bev =
		    bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
		bufferevent_setcb(con->bev, data_read_cb, NULL, data_event_cb,
				  con);
		bufferevent_enable(con->bev, EV_READ | EV_WRITE);

		char *type;
		char *host;
		char *portstr;
		int i;
		portstr = strtok(inbuf, " "); // get rid of command name
		for (i = 0; i < 3 && portstr != NULL; i++) {
			portstr = strtok(NULL, " ");
			if (portstr == NULL)
				break;
			if (i == 0)
				type = portstr;
			else if (i == 1)
				host = portstr;
		}
		int error;
		if (i != 3) {
			printf("Incorrect number of arguments\n");
			printf("create <type> <host> <port>\n");
			bufferevent_free(con->bev);
		} else {
			char lowercase[strlen(type) + 1];
			strcpy(lowercase, type);
			int i;
			for (i = 0; i < strlen(type); i++) {
				lowercase[i] = tolower(lowercase[i]);
			}
			con->type = (char *)malloc(strlen(lowercase) + 1);
			strcpy(con->type, lowercase);
			if (!strcmp(con->type, "ev_builder")) {
				con->con_type = EV_BUILDER;
			} else if (!strcmp(con->type, "xl3")) {
				con->con_type = XL3;
			} else if (!strcmp(con->type, "mtc")) {
				con->con_type = MTC;
			} else if (!strcmp(con->type, "caen")) {
				con->con_type = CAEN;
			} else if (!strcmp(con->type, "orca")) {
				con->con_type = ORCA;
			} else {
				fprintf(stderr,
					"Not a valid monitoring type\n");
				//TODO: fix the memory leak here
				return;
			}
			con->pktsize = pkt_size_of[con->con_type];

			/* make sure there is enough data to fill a packet */
			bufferevent_setwatermark(con->bev, EV_READ,
						 con->pktsize, 0);
			con->host = (char *)malloc(strlen(host) + 1);
			strcpy(con->host, host);
			con->port = atoi(portstr);
			con->valid++;
			if (!bufferevent_socket_connect_hostname
			    (con->bev, dnsbase, AF_UNSPEC, con->host,
			     con->port)) {
				printf("Created socket\n");
				cur_mon_con = 0;
				while (monitoring_cons[cur_mon_con].valid) {
					cur_mon_con++;
				}
			} else {
				printf("Unable to create socket\n");
				delete_con(con);
			}
		}
	}
}

// Data Callbacks
static void data_read_cb(struct bufferevent *bev, void *ctx) {
	/* This callback is invoked when there is data to read on bev. */
	struct evbuffer *input = bufferevent_get_input(bev);
	//struct evbuffer *output = bufferevent_get_output(bev);

	/* Copy all the data from the input buffer to the output buffer. */
	//evbuffer_add_buffer(output, input);
	connection *con = (connection *) ctx;
	char data_pkt[con->pktsize];
	
	//create a pouch_request* object
	pouch_request *pr = pr_init();
	int n = 1;
	while (evbuffer_get_length(input) >= con->pktsize && n > 0) {
		//TODO: spawn a new thread here. (pthread vs. fork?)
		//TODO: make a separate function for each type of data upload, so that threading is easier/more logical
		memset(&data_pkt, 0, con->pktsize);
		n = evbuffer_remove(input, data_pkt, con->pktsize);
		printf("Received data packet (%d bytes)\n", con->pktsize);
		if (con->con_type == XL3){
			printf("XL3 packet!\n");
			XL3_Packet *xpkt = (XL3_Packet *)data_pkt;
			XL3_CommandHeader cmhdr = (XL3_CommandHeader)xpkt->cmdHeader;
			PMTBundle *bndl_array = (PMTBundle *)(xpkt->payload);
			int i;
			JsonNode *data;
			for(i = 0; i < sizeof(xpkt->payload)/sizeof(PMTBundle); i++){
				// fill the bundle
				uint32_t bndl[3];
				memcpy(bndl, &bndl_array[i],sizeof(bndl));
				// unpack the bundle
				uint32_t chan,cell,gt24,gt16,gt8,qlx,qhs,qhl,tac,cmos16,cgt16,cgt24,missed,lgi,nc_cc,crate,board;
				chan = (uint32_t) UNPK_CHANNEL_ID(bndl);
                cell = (uint32_t) UNPK_CELL_ID(bndl);
                gt24 = (uint32_t) UNPK_FEC_GT24_ID(bndl);
				gt16 = (uint32_t) UNPK_FEC_GT16_ID(bndl);
				gt8 = (uint32_t) UNPK_FEC_GT8_ID(bndl);
                qlx = (uint32_t) UNPK_QLX(bndl);
                qhs = (uint32_t) UNPK_QHS(bndl);
                qhl = (uint32_t) UNPK_QHL(bndl);
                tac = (uint32_t) UNPK_TAC(bndl);
                cmos16 = (uint32_t) UNPK_CMOS_ES_16(bndl);
                cgt16 = (uint32_t) UNPK_CGT_ES_16(bndl);
                cgt24 = (uint32_t) UNPK_CGT_ES_24(bndl);
                missed = (uint32_t) UNPK_MISSED_COUNT(bndl);
                lgi = (uint32_t) UNPK_LGI_SELECT(bndl);
                nc_cc = (uint32_t) UNPK_NC_CC(bndl);
                crate = (uint32_t) UNPK_CRATE_ID(bndl);
                board = (uint32_t) UNPK_BOARD_ID(bndl);
				// create the JSON object
				data = json_mkobject();
				json_append_member(data, "chan", json_mknumber(chan));
				json_append_member(data, "cell", json_mknumber(cell));
				json_append_member(data, "gt24", json_mknumber(gt24));
				json_append_member(data, "gt16", json_mknumber(gt16));
				json_append_member(data, "gt8", json_mknumber(gt8));
				json_append_member(data, "qlx", json_mknumber(qlx));
				json_append_member(data, "qhs", json_mknumber(qhs));
				json_append_member(data, "qhl", json_mknumber(qhl));
				json_append_member(data, "tac", json_mknumber(tac));
				json_append_member(data, "cmos16", json_mknumber(cmos16));
				json_append_member(data, "cgt16", json_mknumber(cgt16));
				json_append_member(data, "cgt24", json_mknumber(cgt24));
				json_append_member(data, "missed", json_mknumber(missed));
				json_append_member(data, "lgi", json_mknumber(lgi));
				json_append_member(data, "nc_cc", json_mknumber(nc_cc));
				json_append_member(data, "crate", json_mknumber(crate));
				json_append_member(data, "board", json_mknumber(board));
				// send the data
				char *datastr = json_encode(data);
				pr = doc_create(pr, "http://peterldowns:2rlz54NeO3@peterldowns.cloudant.com", "testing", datastr);
				pr_do(pr);
				// clean up
				json_delete(data);
				free(datastr);
			}
			printf("finished XL3\n");
		}
	}
	// get rid of the pouch object
	pr_free(pr);
}

static void data_event_cb(struct bufferevent *bev, short events, void *ctx) {
	if (events & BEV_EVENT_ERROR)
		perror("Error from bufferevent");
	if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
		delete_con((connection *) ctx);
	}
}

// Controller Callbacks
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
	char dup[sizeof(inbuf)];
	memset(&dup, 0, sizeof(dup));
	strncpy(dup, inbuf, sizeof(dup));
	char *com_key = strtok(dup, " "); // get the command name
	// TODO: fix the whole strtok/command parsing thing.
	int i;
	for(i = 0; controller_coms[i].key; i++){
		if(!strncmp(com_key, controller_coms[i].key, strlen(controller_coms[i].key))){
			controller_coms[i].function(inbuf, NULL);
			break;
		}
	}
}

static void controller_event_cb(struct bufferevent *bev, short events,void *ctx) {
	if (events & BEV_EVENT_ERROR)
		perror("Error from bufferevent (controller)");
	if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
		bufferevent_free(bev);
		have_controller--;
		fprintf(stderr, "Closed controller connection.\n");
	}
}

// Listener Callbacks
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

static void listener_error_cb(struct evconnlistener *listener, void *ctx) {
	struct event_base *base = evconnlistener_get_base(listener);
	int err = EVUTIL_SOCKET_ERROR();
	fprintf(stderr, "Got an error %d (%s) on the listener. "
		"Shutting down.\n", err, evutil_socket_error_to_string(err));
	event_base_loopexit(base, NULL);
}

// Signal Callbacks
void signal_cb(evutil_socket_t sig, short events, void *user_data) {
	struct event_base *base = user_data;
	printf("Caught an interrupt signal; exiting.\n");
	event_base_loopbreak(base);
}

int main(int argc, char **argv) {
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
	event_config_avoid_method(cfg, "select");	// We don't want no select
	event_config_avoid_method(cfg, "poll");	// NO to poll

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
	printf("Starting Libevent %s. Supported methods are:\n",
	       event_get_version());
	int i;
	for (i = 0; methods[i] != NULL; ++i) {
		printf("\t%s\n", methods[i]);
	}
	free((char **)methods);
	printf("Using %s.\n", event_base_get_method(base));

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
	// Run the main loop
	event_base_dispatch(base);

	// Clean up
	evdns_base_free(dnsbase, 0);
	event_base_free(base);
	return 0;
}
