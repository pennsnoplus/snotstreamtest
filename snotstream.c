// Includes
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <arpa/inet.h>
#include <time.h>

#include <event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/dns.h>
#include <event2/util.h>
#include <event2/event.h>
#include <event2/event_struct.h>

#include <curl/curl.h>

#include "snotstream.h"
#include "pkt_utils.h"
#include "lib/json/json.h"
#include "lib/ringbuf/ringbuf.h"
#include "lib/pouch/pouch.h"


// Globals
int have_controller = 0;
int cur_mon_con = 0;
Ringbuf *xl3_buf;
connection monitoring_cons[MAX_MON_CONS];
command controller_coms[] = {
	{"url_get", &url_get},
	{"print_cons", &print_cons},
	{"start", &start_con},
	{"stop", &stop_con},
	{"help", &help},
	{(char *)NULL, (com_ptr)NULL} // leave this here: lets us loop easily (while com != NULL)
};
size_t pkt_size_of[] = {
	// Gets the size of a con_type
	0,
	sizeof(XL3_Packet),
	sizeof(mtcPacket),
	sizeof(caenPacket),
	0,
	0
};

// Helpers, Miscellaneous
struct event *mk_rectimer(struct event_base *base, struct timeval *interval, void (*user_cb)(evutil_socket_t, short, void *), void *user_data){
	/* Creates a recurring timer event. */
	struct event *ev;
	if(!(ev = event_new(base, -1, EV_TIMEOUT|EV_PERSIST, user_cb, user_data))){
		return NULL;
	}
	if(event_add(ev, interval) < 0){
		event_free(ev);
		return NULL;
	}
	return ev;
	/*
	struct timeval xl3_delay; // set the delay to...
	xl3_delay.tv_sec=0;
	//xl3_delay.tv_usec=500000; // .5 seconds
	xl3_delay.tv_usec=50000;	// .05 seconds
	//xl3_delay.tv_usec=5000;	// .005 seconds
	*/
}
void delete_con(connection * con) {
	/* 
		Deletes a connection and frees that memory,
		while keeping track of cur_mon_con appropriately.
	*/
		
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
	/* 
		Gets the connection type from
		a string. (i.e., "XL3" returns XL3/1 )
	*/
	char lowercase[strlen(typestr) + 1]; // lowercase the type string
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
	/*
		Spits out a string describing a
		connection type (i.e., XL3 ->
		"XL3" ).
	*/
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

// Controller Commands
void help(char *UNUSED, void *_UNUSED){
	/*
		Prints the keys of all available commands.
	*/
	printf("Valid commands:\n");
	int i;
	for (i = 0; controller_coms[i].key; i++){
		printf("\t%s\n", controller_coms[i].key);
	}
	if (!i){
		printf("(no valid commands)\n");
	}
}
void print_cons(char *UNUSED, void *_UNUSED) {
	/*
		Prints the type and location of all connected  
		monitoringpoints.
	*/
	int i;
	connection *con;
	printf("Connected monitoring points:\n");
	for (i = 0; i < MAX_MON_CONS; i++) {
		con = &monitoring_cons[i];
		if (con->valid) {
			printf("\t%s %s:%d\n", get_con_typestr(con->type), con->host,
					con->port);
		}
	}
	if (!i){
		printf("(no connected monitoring points\n");
	}
}
void url_get(char *url, void *data){
	/* 
		Starts a GET request to a given URL.
		For debugging only.
	*/
	PouchMInfo *pmi = (PouchMInfo *)data;
	PouchReq *pr;
	pr = pr_init();
	pr_set_url(pr, url);
	pr_set_method(pr, GET);
	pr = pr_domulti(pr, pmi->multi); // send a GET request to the url
	//fprintf(stderr, "Added easy %p to multi %p (%s)\n", pr->easy, pmi->multi, pr->url);
	debug_mcode("new_conn: curl_multi_add_handle", pr->curlmcode);
	/* curl_multi_add_handle() will set a time-out to trigger very soon so
	   that the necessary socket_action() call will be called by this app */
}
void stop_con(char *inbuf, void *UNUSED) {
	/*
		Tries to stop a monitoring connection of
		a specific type to a given destination.
	*/
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
	printf("No %s connection to %s:%d\n", type, host, port);
}
void start_con(char *inbuf, void *data) {
	/*
		If the max number of monitoring connections
		has not been reached, this tries to start
		monitoring a given port/host for data.
	*/
	PouchMInfo *pmi = (PouchMInfo *)data;
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
	con->bev = bufferevent_socket_new(pmi->base, -1, BEV_OPT_CLOSE_ON_FREE);
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
			(con->bev, pmi->dnsbase, AF_UNSPEC, con->host,
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

// Data Parsers
void parse_xl3(void *data_pkt){
	/* Pushes an xl3 packet to the global xl3 ringbuffer for uploading. */
	XL3_Packet *xpkt = (XL3_Packet *)data_pkt;
	XL3_CommandHeader cmhdr = (XL3_CommandHeader)(xpkt->cmdHeader);
	PMTBundle *bndl_array = (PMTBundle *)(xpkt->payload);
	
	int i;
	pmt_upls *data;
	for(i = 0; i < cmhdr.num_bundles; i++){
		data = (pmt_upls *)calloc(1, sizeof(pmt_upls));
		data->qlx = (uint32_t) UNPK_QLX((uint32_t *)&bndl_array[i]);
		data->qhs = (uint32_t) UNPK_QHS((uint32_t *)&bndl_array[i]);
		data->qhl = (uint32_t) UNPK_QHL((uint32_t *)&bndl_array[i]);
		data->crate = (uint32_t) UNPK_CRATE_ID((uint32_t *)&bndl_array[i]);
		data->board = (uint32_t) UNPK_BOARD_ID((uint32_t *)&bndl_array[i]);
		data->chan = (uint32_t) UNPK_CHANNEL_ID((uint32_t *)&bndl_array[i]);
		data->pmt = 512*(data->crate)+32*(data->board)+(data->chan);
		if(!ringbuf_isfull(xl3_buf)){
			ringbuf_push(xl3_buf, data, 0); // TODO: fix push size
		}
		else { // If the buffer is full, we've fallen behind.
			fprintf(stderr, "parse_xl3: ran out of room!\
							ring buffer overflow. data has been lost!");
		}
	}
	free(data_pkt);
}

// Data Uploaders
void upload_xl3(Ringbuf *rbuf, CURLM *multi){
	/* Uploads data on XL3 packets in the ringbuffer. */
	PouchReq *pr;
	int pmt;
	pmt_upls *data;
	pmt_upls pmts[19*16*32] = {0}; // 19*16*32 = num pmt's / does this initialize to 0? maybe...

	while(!ringbuf_isempty(rbuf)){
		ringbuf_pop(rbuf, (void **)&data);
		//pmt = 512*data.crate + 32*data.board + data.chan;
		pmt = data->pmt;
		pmts[pmt].qhs = pmts[pmt].qhs + data->qhs;
		pmts[pmt].qhl = pmts[pmt].qhl + data->qhl;
		pmts[pmt].qlx = pmts[pmt].qlx + data->qlx;
		pmts[pmt].count++;
		free(data);
	}
	
	// Holds the JSON to be uploaded
	JsonNode *doc = json_mkobject();
	
	struct timeval tv; // timestamp
	gettimeofday(&tv,0);
	double timestamp = ((double)tv.tv_sec) + ((double)tv.tv_usec)/1000000;
	json_append_member(doc, "ts", json_mknumber(timestamp));

	int i;
	JsonNode *pmt_array = json_mkarray();
	for(i = 0; i < 19*16*32; i++){
		// TODO: making a lot of objects. better structure?
		JsonNode *pmt_json = json_mkobject();
		if(!pmts[i].count){ // no divide by 0
			pmts[i].count = 1;
		}
		json_append_member(pmt_json, "crate", json_mknumber((double)pmts[i].crate));
		json_append_member(pmt_json, "board", json_mknumber((double)pmts[i].board));
		json_append_member(pmt_json, "chan", json_mknumber((double)pmts[i].chan));
		json_append_member(pmt_json, "pmt", json_mknumber((double)pmts[i].pmt));
		json_append_member(pmt_json, "qhs", json_mknumber(((double)pmts[i].qhs)/pmts[i].count));
		json_append_member(pmt_json, "qhl", json_mknumber(((double)pmts[i].qhl)/pmts[i].count));
		json_append_member(pmt_json, "qlx", json_mknumber(((double)pmts[i].qlx)/pmts[i].count));
		json_append_element(pmt_array, pmt_json);
	}

	json_append_member(doc, "pmts", pmt_array);

	char *datastr = json_encode(doc);
	pr = pr_init();
	pr = doc_create(pr, SERVER, DATABASE, datastr);
	free(datastr);
	json_delete(doc);
	pr_domulti(pr, multi);
}

// Libevent Callbacks
void signal_cb(evutil_socket_t sig, short events, void *user_data) {
	/* Catches C-c and exits cleanly by stopping the event loop. */
	struct event_base *base = user_data;
	printf("Caught an interrupt signal; exiting.\n");
	event_base_loopbreak(base);
}

static void xl3_watcher_cb(evutil_socket_t fd, short events, void *ctx){
	/*
		Uploads any xl3 data in the xl3 ringbuffer.
	*/
	if(!ringbuf_isempty(xl3_buf)){
		// TODO: could this be faster?
		Ringbuf *tmpbuf = ringbuf_copy(xl3_buf); // so that writes can keep happening while we read

		PouchMInfo *pmi = (PouchMInfo *)ctx;
		upload_xl3(tmpbuf, pmi->multi);	// upload the xl3 data
		ringbuf_clear(tmpbuf);
		free(tmpbuf);
	}
}

static void data_read_cb(struct bufferevent *bev, void *ctx) {
	//TODO: use peek() to not copy data, or something fast!!!
	/* Reads data in from a monitoring connection. */
	struct evbuffer *input = bufferevent_get_input(bev);

	connection *con = (connection *)ctx;
	char data_pkt[con->pktsize];
	memset(&data_pkt, 0, sizeof(data_pkt)); 

	int bytes_read = 1;
	while (evbuffer_get_length(input) >= con->pktsize && bytes_read > 0) {
		bytes_read = evbuffer_remove(input, data_pkt, con->pktsize);
		//printf("Processing %s data packet (%d bytes)\n", get_con_typestr(con->type), (int)(con->pktsize));
		// build a temporary packet for each thread
		// TODO: temporary not necessary?
		char *tmp_pkt = (char *)malloc(con->pktsize);
		if (!tmp_pkt){
			fprintf(stderr, "could not malloc()ate enough memory for a temporary packet\n");
			return;
		}
		memcpy(tmp_pkt, &data_pkt, con->pktsize);
		switch (con->type){
			case XL3:
				parse_xl3(tmp_pkt);
				break;
			default:
				printf("Not sure how to handle this data.\n");
				printf("Currently, sno+stream only has support for XL3 data.\n");
				break;
		}
	}
}
static void data_event_cb(struct bufferevent *bev, short events, void *ctx) {
	/* Handles errors and file closes on monitoring connections. */
	if (events & BEV_EVENT_ERROR)
		perror("Error from bufferevent");
	if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
		connection *con = (connection *)ctx;
		fprintf(stderr, "Received EOF or error: closing %s connection to %s:%d\n", get_con_typestr(con->type),con->host,con->port);
		delete_con((connection *)ctx);
	}
}

static void controller_read_cb(struct bufferevent *bev, void *ctx) {
	/* Reads a string in from the controller and tries to parse it as a command. */
	struct evbuffer *input = bufferevent_get_input(bev);
	char inbuf[500]; // arbitrary sizing
	memset(&inbuf, 0, sizeof(inbuf));
	int n;
	while ((n = evbuffer_remove(input, inbuf, sizeof(inbuf))) > 0) {
		// this copies the data from input to the inbuf
	}
	printf("Controller: %s", inbuf);
	
	if (inbuf[strlen(inbuf) - 1] != '\n')
		fputc('\n', stdout);
	char dup[sizeof(inbuf)];
	memset(&dup, 0, sizeof(dup)); // TODO: Unnecessary? make faster
	strncpy(dup, inbuf, sizeof(dup));
	char *com_key = strtok(dup, " "); // get the command name
	char *args = strchr(inbuf, ' ');
	if(args){
		args++; // get rid of the space at the beginning of commands
	}
	int i;
	for(i = 0; controller_coms[i].key; i++){
		if(!strncmp(com_key, controller_coms[i].key, strlen(controller_coms[i].key))){
			controller_coms[i].function(args, ctx);
			break;
		}
	}
}
void controller_event_cb(struct bufferevent *bev, short events,void *ctx) {
	/* Handles errors and file closes for the controller connection. */
	if (events & BEV_EVENT_ERROR)
		perror("Error from bufferevent (controller)");
	if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
		bufferevent_free(bev);
		have_controller--;
		fprintf(stderr, "Closed controller connection.\n");
	}
}

void listener_accept_cb(struct evconnlistener *listener,evutil_socket_t fd, struct sockaddr *address,int socklen, void *ctx) {
	/* Accepts and creates a controller connection if one doesn't already exist. */
	if (!have_controller) {
		struct event_base *base = evconnlistener_get_base(listener);
		struct bufferevent *bev =
		    bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
		bufferevent_setcb(bev, controller_read_cb, NULL,
				  controller_event_cb, ctx); // ctx holds the PouchMInfo object. pass this on.
		bufferevent_enable(bev, EV_READ | EV_WRITE);
		have_controller++;
	} else {
		EVUTIL_CLOSESOCKET(fd);
		fprintf(stderr, "Closed extra controller connection\n");
	}
}
void listener_error_cb(struct evconnlistener *listener, void *ctx) {
	/* Handles errors and file closes on the listener. If this callback
	ever runs, there is some sort of prblem and the program is closed. */
	struct event_base *base = evconnlistener_get_base(listener);
	int err = EVUTIL_SOCKET_ERROR();
	fprintf(stderr, "Got an error %d (%s) on the listener. "
		"Shutting down.\n", err, evutil_socket_error_to_string(err));
	event_base_loopexit(base, NULL);
}


// Pouch Callbacks
void pr_callback(PouchReq *pr, PouchMInfo *pmi){
	/*
		This callback is used to handle completed
		PouchReq(uests). Right now, it just notifies
		the user that a request was completed.
	*/
	printf("%s request to %s returned %s\n", pr->method, pr->url, pr->resp.data);
	pr_free(pr);
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
		return 2;
	}
	
	// Create a dns base for our network listener
	struct evdns_base *dnsbase = evdns_base_new(base, 1);
	if (!dnsbase) {
		fprintf(stderr, "Could not create a dns base\n");
		return 3;
	}
	
	// Create the PouchMInfo object for use in callbacks
	PouchMInfo *pmi = pr_mk_pmi(base, dnsbase, &pr_callback, NULL);
	if(!pmi){
		fprintf(stderr, "Could not create a PouchMInfo structure\n");
		return 4;
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
		return 5;
	}
	evconnlistener_set_error_cb(listener, listener_error_cb);

	// Create a signal watcher for C-c (SIGINT)
	struct event *signal_event;
	signal_event = evsignal_new(pmi->base, SIGINT, signal_cb, (void *)(pmi->base));
	if (!signal_event || event_add(signal_event, NULL) < 0) {
		fprintf(stderr, "Could not create/add a signal event!\n");
		return 6;
	}
	
	// Initialize the XL3 ringbuffer
	xl3_buf = ringbuf_init(&xl3_buf, 1000000); // some absurdly large size
	
	// Create the XL3 ringbuffer watcher
	struct timeval xl3_delay; // set the delay to...
	xl3_delay.tv_sec=0;
	xl3_delay.tv_usec=500000; // ... 0.5 seconds

	// TODO: use mk_rectimer instead
	struct event *xl3_watcher = event_new(pmi->base, -1, EV_TIMEOUT|EV_PERSIST, xl3_watcher_cb, pmi);
	if(!xl3_watcher){
		fprintf(stderr, "Unable to create a recurring timer to watch the xl3 ringbuffer.\n");
		return 7;
	}
	if(event_add(xl3_watcher, &xl3_delay) < 0){
		fprintf(stderr, "Could not set a timevalue for the xl3 ringbuffer watcher timer.\n");
		return 8;
	}

	// Run the main loop
	event_base_dispatch(pmi->base);
	
	// Clean up
	event_del(signal_event);
	free(signal_event);
	event_del(xl3_watcher);
	free(xl3_watcher);
	evconnlistener_free(listener);

	pr_del_pmi(pmi); // frees 'base' and 'dnsbase', too
	
	// Exit
	return 0;
}
