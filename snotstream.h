#include "pkt_types.h"
#include "lib/pouch/pouch.c"
#include "lib/ringbuf/ringbuf.c"

// Structs, Typedefs
typedef void(*com_ptr)(char *, void *); // command pointer
typedef enum {
	EV_BUILDER, // TODO: EV_BUILDER data
	XL3,
	MTC,
	CAEN,
	ORCA,       // TODO: ORCA data
    UNKNOWN,    // TODO: default size
	con_type_max
} con_type;
typedef struct {
    struct bufferevent *bev;
    con_type type;
    char *host;
    int port;
    int valid;
    size_t pktsize;
} connection;
typedef struct {
//{ crate=..., board=..., chan=..., qlx=..., qhl=..., qhs=... } //pmt0000
	uint32_t crate;
	uint32_t board;
	uint32_t chan;
	uint32_t pmt;// = crate*512+board*32+chan
	uint32_t qlx;
	uint32_t qhl;
	uint32_t qhs;
	int count;
} pmt_upls;
typedef struct { /* Holds a list of commands */
	char *key; 
	com_ptr function;
} command;

// Helpers, Miscellaneous
struct event *mk_rectimer(struct event_base *base, struct timeval *interval, void (*user_cb)(evutil_socket_t, short, void *), void *user_data);
void delete_con(connection * con);
int get_con_type(char *typestr);
char *get_con_typestr(con_type type);

// Controller Commands
void help(char *UNUSED, void *_UNUSED);
void print_cons(char *UNUSED, void *_UNUSED);
void url_get(char *url, void *data);
void stop_con(char *inbuf, void *UNUSED);
void start_con(char *inbuf, void *data);

// Data Parsers
void parse_xl3(void *data_pkt);

// Data Uploaders
void upload_xl3(Ringbuf *rbuf, CURLM *multi);

// Libevent Callbacks
void signal_cb(evutil_socket_t sig, short events, void *user_data);

static void xl3_watcher_cb(evutil_socket_t fd, short events, void *ctx);

static void data_read_cb(struct bufferevent *bev, void *ctx);
static void data_event_cb(struct bufferevent *bev, short events, void *ctx);

static void controller_read_cb(struct bufferevent *bev, void *ctx);
void controller_event_cb(struct bufferevent *bev, short events,void *ctx);

void listener_accept_cb(struct evconnlistener *listener,evutil_socket_t fd, struct sockaddr *address,int socklen, void *ctx);
void listener_error_cb(struct evconnlistener *listener, void *ctx);


// Pouch Callbacks
void pr_callback(PouchReq *pr, PouchMInfo *pmi);
