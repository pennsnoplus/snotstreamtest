// Standard Libraries
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <arpa/inet.h>
#include <time.h>

// Libevent
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/dns.h>
#include <event2/util.h>
#include <event2/event.h>

// Threading
#include <pthread.h>

// Custom
#include "monitor.h"
#include "lib/json/json.h"
#include "lib/pouch/pouch.h"

int bundle_counter = 0;

// Helper Functions
void delete_con(connection * con) {
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

int get_con_type(char *typestr){
	// lowercase the type string
	char lowercase[strlen(typestr) + 1];
	strcpy(lowercase, typestr);
	int i;
	for (i = 0; i < strlen(lowercase); i++) {
		lowercase[i] = tolower(lowercase[i]);
	}
	if (!strcmp(lowercase, "ev_builder"))
		return EV_BUILDER;
	if (!strcmp(lowercase, "xl3"))
		return XL3;
	if (!strcmp(lowercase, "mtc"))
		return MTC;
	if (!strcmp(lowercase, "caen"))
		return CAEN;
	if (!strcmp(lowercase, "orca"))
		return ORCA;
	return UNKNOWN;
}

char *get_con_typestr(con_type type){
	switch (type){
		case EV_BUILDER:
			return "EV_BUILDER";
		case XL3:
			return "XL3";
		case MTC:
			return "MTC";
		case CAEN:
			return "CAEN";
		case ORCA:
			return "ORCA";
		default:
			return "UNKNOWN";
	}
}

void *upload_buffer(void *ptr){
	Ringbuf *rbuf = (Ringbuf *)ptr;
	pouch_request *pr;
	//char *datastr;
	JsonNode *data;
	uint32_t qhs = 0;
	uint32_t qlx = 0;
	uint32_t qhl = 0;
	//printf("-- Uploading Ringbuf %p\n", rbuf);
	int i=0;
	//ringbuf_status(rbuf, "-- ");
	while(!ringbuf_isempty(rbuf)){
		//pr = pr_init();
		//ringbuf_pop(rbuf, (void **)&datastr);
		ringbuf_pop(rbuf, (void **)&data);
		qhs = qhs + json_get_number(json_find_member(data, "qhs"));
		qlx = qlx + json_get_number(json_find_member(data, "qlx"));
		qhl = qhl + json_get_number(json_find_member(data, "qhl"));
		i++;
		json_delete(data);
		//pr = doc_create(pr, SERVER, DATABASE, datastr);
		//pr_do(pr);
		//free(datastr);
		//pr_free(pr);
	}
	data = json_mkobject();
	struct timeval tv;
	gettimeofday(&tv,0);
	double timestamp = ((double)tv.tv_sec) + ((double)tv.tv_usec)/1000000;
	//printf("timestamp = %f\n", timestamp);
	json_append_member(data, "qhs", json_mknumber(((double)qhs)/i));
	json_append_member(data, "qlx", json_mknumber(((double)qlx)/i));
	json_append_member(data, "qhl", json_mknumber(((double)qhl)/i));
	json_append_member(data, "ts", json_mknumber(timestamp));
	char *datastr = json_encode(data);
	pr = pr_init();
	pr = doc_create(pr, SERVER, DATABASE, datastr);
	pr_do(pr);
	free(datastr);
	json_delete(data);
	pr_free(pr);

	//printf("-- Finished uploading Ringbuf %p\n", rbuf);
	ringbuf_clear(rbuf);
	free(rbuf);
	pthread_exit(NULL);
}
// Commands
void help(char *UNUSED, void *_UNUSED){
	int i;
	for(i = 0; controller_coms[i].key; i++){
		printf("\t%s\n", controller_coms[i].key);
	}
}

void print_cons(char *UNUSED, void *_UNUSED) {
	int i;
	connection *con;
	for (i = 0; i < MAX_MON_CONS; i++) {
		con = &monitoring_cons[i];
		if (con->valid) {
			printf("\t%s %s:%d\n", get_con_typestr(con->type), con->host,
					con->port);
		}
	}
}

void stop_con(char *inbuf, void *UNUSED) {
	char *type;
	char *host;
	char *portstr;
	int i;
	if (!(type = strtok(inbuf, " ")) ||
		!(host = strtok(NULL, " ")) ||
		!(portstr = strtok(NULL, " "))){
		printf("Incorrect number of arguments\n");
		printf("stop <type> <host> <port>\n");
		return;
	}
	int port = atoi(portstr);
	connection *con;
	int j;
	for(j = 0; j < cur_mon_con; j++){
		con = &monitoring_cons[j];
		if (con->port == port){
			if(con->type == get_con_type(type)){
				if (!strcmp(con->host, host)){
					delete_con(con);
					printf("Stopped monitoring %s %s:%d\n", get_con_typestr(con->type), con->host, con->port);
					return;
				}
			}
		}
	}
}

void start_con(char *inbuf, void *UNUSED) {
	char *type;
	char *host;
	char *portstr;
	int i;
	if (!(type = strtok(inbuf, " ")) ||
		!(host = strtok(NULL, " ")) ||
		!(portstr = strtok(NULL, " "))){
		printf("Incorrect number of arguments\n");
		printf("stop <type> <host> <port>\n");
		return;
	}
	if (cur_mon_con >= MAX_MON_CONS){ // make sure there are enough free cons
		fprintf(stderr, "no more free connections\n");
		return;
	}
	// create the connection
	connection *con = &(monitoring_cons[cur_mon_con]);
	con->bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
	bufferevent_setcb(con->bev, data_read_cb, NULL, data_event_cb, con);
	bufferevent_enable(con->bev, EV_READ | EV_WRITE);
	
	// fill out the connection information
	con->type = get_con_type(type);
	con->pktsize = pkt_size_of[con->type];
	con->valid++;

	// set the appropriate read callback watermarks
	bufferevent_setwatermark(con->bev, EV_READ, con->pktsize, 0);
	con->host = (char *)malloc(strlen(host)+1);
	strcpy(con->host, host);
	con->port = atoi(portstr);
	
	// make the connection
	if (!bufferevent_socket_connect_hostname
			(con->bev, dnsbase, AF_UNSPEC, con->host,
			 con->port)) {
		printf("Created monitoring connection: %s %s:%d\n", get_con_typestr(con->type), con->host, con->port);
	} else {
		printf("COULD NOT create monitoring connection: %s %s:%d\n", get_con_typestr(con->type), con->host, con->port);
		delete_con(con);
	}
	cur_mon_con = 0;
	while (monitoring_cons[cur_mon_con].valid) {
		cur_mon_con++;
	}
}


// Data Handlers
void handle_xl3(void *data_pkt){
	//return;
	XL3_Packet *xpkt = (XL3_Packet *)data_pkt;
	//int z;
	//char *bdata = (char *)data_pkt;
	//for(z = 0; z < sizeof(XL3_Packet); z++){
		//printf("%d : %08x\n", z, (int)(*(bdata+z)));
	//}
	XL3_CommandHeader cmhdr = (XL3_CommandHeader)(xpkt->cmdHeader);
	PMTBundle *bndl_array = (PMTBundle *)(xpkt->payload);
	
	//printf("Num bundles = %d\n", cmhdr.num_bundles);
	//printf("Packet type = %08x", cmhdr.packet_type);
	//printf("Packet num = %08x", cmhdr.packet_num);
	//printf("First PMT Bundle\n");
	//printf("word1= %08x\nword2= %08x\nword3= %08x\n", bndl_array[0].word1, bndl_array[0].word2, bndl_array[0].word3);
	//uint32_t _qlx, _qhs, _qhl;
	//_qlx = (uint32_t) UNPK_QLX((uint32_t *)&bndl_array[0]);
	//_qhs = (uint32_t) UNPK_QHS((uint32_t *)&bndl_array[0]);
	//_qhl = (uint32_t) UNPK_QHL((uint32_t *)&bndl_array[0]);
	//printf("qhl= %d\nqhs= %d\nqlx= %d\n", _qhl, _qhs, _qlx);
	//printf("---------------\n");
	
	
	int i;
	JsonNode *data;
	//for(i = 0; i < sizeof(xpkt->payload)/sizeof(PMTBundle); i++){
	for(i = 0; i < cmhdr.num_bundles; i++){
		//printf("----------\n");
		//printf("word1= %d\nword2= %d\nword3= %d\n", bndl_array[i].word1, bndl_array[i].word2, bndl_array[i].word3);
		// fill the bundle
		//uint32_t bndl[3];
		//memcpy(bndl, &bndl_array[i],sizeof(bndl));
		// unpack the bundle
		//uint32_t chan,cell,gt24,gt16,gt8,qlx,qhs,qhl,tac,cmos16,cgt16,cgt24,missed,lgi,nc_cc,crate,board;
		uint32_t qlx, qhs, qhl;
		//chan = (uint32_t) UNPK_CHANNEL_ID(bndl);
		//cell = (uint32_t) UNPK_CELL_ID(bndl);
		//gt24 = (uint32_t) UNPK_FEC_GT24_ID(bndl);
		//gt16 = (uint32_t) UNPK_FEC_GT16_ID(bndl);
		//gt8 = (uint32_t) UNPK_FEC_GT8_ID(bndl);
		qlx = (uint32_t) UNPK_QLX((uint32_t *)&bndl_array[i]);
		qhs = (uint32_t) UNPK_QHS((uint32_t *)&bndl_array[i]);
		qhl = (uint32_t) UNPK_QHL((uint32_t *)&bndl_array[i]);
		//printf("qhl= %d\nqhs= %d\nqlx= %d\n", qhl, qhs, qlx);
		//printf("----------\n");
		//tac = (uint32_t) UNPK_TAC(bndl);
		//cmos16 = (uint32_t) UNPK_CMOS_ES_16(bndl);
		//cgt16 = (uint32_t) UNPK_CGT_ES_16(bndl);
		//cgt24 = (uint32_t) UNPK_CGT_ES_24(bndl);
		//missed = (uint32_t) UNPK_MISSED_COUNT(bndl);
		//lgi = (uint32_t) UNPK_LGI_SELECT(bndl);
		//nc_cc = (uint32_t) UNPK_NC_CC(bndl);
		//crate = (uint32_t) UNPK_CRATE_ID(bndl);
		//board = (uint32_t) UNPK_BOARD_ID(bndl);
		// create the JSON object
		data = json_mkobject();
		//json_append_member(data, "chan", json_mknumber(chan));
		//json_append_member(data, "cell", json_mknumber(cell));
		//json_append_member(data, "gt24", json_mknumber(gt24));
		//json_append_member(data, "gt16", json_mknumber(gt16));
		//json_append_member(data, "gt8", json_mknumber(gt8));
		json_append_member(data, "qlx", json_mknumber(qlx));
		json_append_member(data, "qhs", json_mknumber(qhs));
		json_append_member(data, "qhl", json_mknumber(qhl));
		//json_append_member(data, "tac", json_mknumber(tac));
		//json_append_member(data, "cmos16", json_mknumber(cmos16));
		//json_append_member(data, "cgt16", json_mknumber(cgt16));
		//json_append_member(data, "cgt24", json_mknumber(cgt24));
		//json_append_member(data, "missed", json_mknumber(missed));
		//json_append_member(data, "lgi", json_mknumber(lgi));
		//json_append_member(data, "nc_cc", json_mknumber(nc_cc));
		//json_append_member(data, "crate", json_mknumber(crate));
		//json_append_member(data, "board", json_mknumber(board));
		
		// add data to buffer
		//char *datastr = json_encode(data);
		//ringbuf_push((xl3_buf), ((void *)datastr), (strlen(datastr)+1));
		//json_delete(data);
		//free(datastr);
		ringbuf_push(xl3_buf, data, 0); // TODO: fix push size
	}
	free(data_pkt);

	// If the buffer is full, spawn a new thread to upload it
	if(ringbuf_isfull(xl3_buf)){
		printf("RING_BUFFER_OVERFLOW_BUFFER_OVERFLOW\n");
		printf("RING_BUFFER_OVERFLOW_BUFFER_OVERFLOW\n");
		printf("RING_BUFFER_OVERFLOW_BUFFER_OVERFLOW\n");
		printf("RING_BUFFER_OVERFLOW_BUFFER_OVERFLOW\n");
		/*
		int rc = 0;
		pthread_t thread;
		Ringbuf *tmpbuf = ringbuf_copy(xl3_buf);
		rc = pthread_create(&thread, NULL, upload_buffer, tmpbuf);
		if (rc) {
			fprintf(stderr, "ERROR: return code from pthread_create() is %d\n", rc);
		}
		*/
	}
}

// Data Callbacks
//int __a=0;
static void data_read_cb(struct bufferevent *bev, void *ctx) {
	//TODO: use peek() to not copy data, or something fast!!!
	/*
	if (!(__a%100))
		printf("%d\n", __a++);
	else
		__a++;
	return;
	*/
	/* This callback is invoked when there is data to read on bev. */
	struct evbuffer *input = bufferevent_get_input(bev);
	//struct evbuffer *output = bufferevent_get_output(bev);

	/* Copy all the data from the input buffer to the output buffer. */
	//evbuffer_add_buffer(output, input);
	connection *con = (connection *)ctx;
	char data_pkt[con->pktsize];
	
	//create a pouch_request* object
	int n = 1;
	while (evbuffer_get_length(input) >= con->pktsize && n > 0) {
		memset(&data_pkt, 0, con->pktsize);

		n = evbuffer_remove(input, data_pkt, con->pktsize);
		//printf("Processing %s data packet (%d bytes)\n", get_con_typestr(con->type), (int)(con->pktsize));
		// build a temporary packet for each thread
		char *tmp_pkt = (char *)malloc(con->pktsize);
		if (!tmp_pkt){
			fprintf(stderr, "could not malloc()ate enough memory for a temporary packet\n");
			return;
		}
		memcpy(tmp_pkt, &data_pkt, con->pktsize);

		switch (con->type){
			case XL3:
				handle_xl3(tmp_pkt);
				break;
			default:
				printf("not sure how to handle this data.\n");
				break;
		}
	}
}

static void data_event_cb(struct bufferevent *bev, short events, void *ctx) {
	if (events & BEV_EVENT_ERROR)
		perror("Error from bufferevent");
	if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
		connection *con = (connection *)ctx;
		fprintf(stderr, "Received EOF or error: closing %s connection to %s:%d\n", get_con_typestr(con->type),con->host,con->port);
		delete_con((connection *)ctx);
	}
}

static void buffer_timeout_cb(evutil_socket_t fd, short events, void *ctx){
	//puts("BUFFER_TIMEOUT_CB\n");
	Ringbuf *rbuf = (Ringbuf *)ctx;
	//ringbuf_status(rbuf, "buffer_timeout_cb: ");
	if(!ringbuf_isempty(rbuf)){
		//puts("buffer_timeout_cb: Data to upload");
		int rc = 0;
		pthread_t thread;
		Ringbuf *tmpbuf = ringbuf_copy(rbuf);
		//printf("finished copy\n");
		rc = pthread_create(&thread, NULL, upload_buffer, tmpbuf);
		if (rc) {
			fprintf(stderr, "ERROR: return code from pthread_create() is %d\n", rc);
		}
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
	char *args = strchr(inbuf, ' ');
	if(args)
		args++; // get rid of the space at the beginning of commands
	int i;
	for(i = 0; controller_coms[i].key; i++){
		if(!strncmp(com_key, controller_coms[i].key, strlen(controller_coms[i].key))){
			controller_coms[i].function(args, NULL);
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
struct event *recurring_timer(struct event_base *base, struct timeval *interval, void (*user_cb)(evutil_socket_t, short, void *), void *user_data){
	struct event *ev;
	if(!(ev=event_new(base, -1, EV_PERSIST, user_cb, user_data)))
		return NULL;
	if(!(event_add(ev, interval))){
		event_free(ev);
		return NULL;
	}
	return ev;
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
	// Initialize the XL3 out buffer
	//xl3_buf = ringbuf_init(&xl3_buf, 3*120); // testing w/ fakedata
	//xl3_buf = ringbuf_init(&xl3_buf, 100000); //100,000, testing w/ Richie's fake data
	xl3_buf = ringbuf_init(&xl3_buf, 1000000);

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

	// Create the XL3 data watcher
//struct event *recurring_timer(struct event_base *base, struct timeval *interval, void (*user_cb)(evutil_socket_t, short, void *), void *user_data){
	
	struct timeval xl3_delay; // set the delay to...
	xl3_delay.tv_sec=0;
	//xl3_delay.tv_usec=500000; // .5 seconds
	xl3_delay.tv_usec=50000;	// .05 seconds
	//xl3_delay.tv_usec=5000;	// .005 seconds

	xl3_watcher = event_new(base, -1, EV_TIMEOUT|EV_PERSIST, buffer_timeout_cb, xl3_buf);
	if(!xl3_watcher){
		fprintf(stderr, "woops\n");
		return 1;
	}
	event_add(xl3_watcher, &xl3_delay);

/*
	xl3_watcher = recurring_timer(base, &xl3_delay, &buffer_timeout_cb, xl3_buf);
	if(!xl3_watcher){
		fprintf(stderr, "Could not create xl3 watcher timeout\n");
		return 1;
	}
	*/

	// Run the main loop
	event_base_dispatch(base);
	
	// Clean up
	for(cur_mon_con=0;cur_mon_con<MAX_MON_CONS;cur_mon_con++){
		if(monitoring_cons[cur_mon_con].valid)
			delete_con(&monitoring_cons[cur_mon_con]);
	}
	// TODO: fix the fact that when a connection closes, it isn't freed
	event_del(signal_event);
	event_del(xl3_watcher);
	free(signal_event);
	evdns_base_free(dnsbase, 0);
	event_base_free(base);
	pthread_exit(NULL);
}
