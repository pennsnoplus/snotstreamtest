#include "pkt_types.h"
#include "pkt_utils.h"
#include "lib/ringbuf/ringbuf.h"

#define MAX_MON_CONS 10
#define SERVER "localhost:5984"//"snoplus:snoplustest@snoplus.cloudant.com"
#define DATABASE "pmt_test"

/*
 * Structures
 */
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
// Gets the size of a con_type.
// (pkt_size_of[EV_BUILDER]);
size_t pkt_size_of[] =
{
	0,
	sizeof(XL3_Packet),
	sizeof(mtcPacket),
	sizeof(caenPacket),
	0,
	0
};
typedef void(*com_ptr)(char *, void *); // command pointer
typedef struct {
	// holds a list of commands
	char *key;
	com_ptr function;
} command;

/*
 * Prototypes
 */
// Commands
void start_con(char *inbuf, void *UNUSED);
void stop_con(char *inbuf, void *UNUSED);
void print_cons(char *UNUSED, void *_UNUSED);
void help(char *UNUSED, void *_UNUSED);

// Helper Functions
void delete_con(connection * con);
int get_con_type(char *typestr);
char *get_con_typestr(con_type type);
void *upload_buffer(void *ptr);

// Data Handlers
void handle_xl3(void *data_pkt);

// Data Callbacks
static void data_read_cb(struct bufferevent *bev, void *ctx);
static void data_event_cb(struct bufferevent *bev, short events, void *ctx);

// Controller Callbacks
static void controller_read_cb(struct bufferevent *bev, void *ctx);
static void controller_event_cb(struct bufferevent *bev, short events,void *ctx);

// Listener Callbacks
static void listener_accept_cb(struct evconnlistener *listener,
			       evutil_socket_t fd, struct sockaddr *address,
			       int socklen, void *ctx);
static void listener_error_cb(struct evconnlistener *listener, void *ctx);

// Signal Callbacks
void signal_cb(evutil_socket_t sig, short events, void *user_data);

/*
 * Globals
 */
struct event_base *base;
struct evdns_base *dnsbase;
struct evconnlistener *listener;
//struct evbuffer *xl3_pool;
Ringbuf *xl3_buf;
int have_controller = 0;
int cur_mon_con = 0;
connection monitoring_cons[MAX_MON_CONS];
command controller_coms[] = {
	{"print_cons", &print_cons},
	{"start", &start_con},
	{"stop", &stop_con},
	{"help", &help},
	{(char *)NULL, (com_ptr)NULL} // leave this here.
};
