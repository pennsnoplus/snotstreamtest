#include "monitor.h"

#define MAX_MON_CONS 10

/* Structs */
typedef struct {
    struct bufferevent *bev;
    char *type;
    con_type con_type;
    char *host;
    int port;
    int valid;
    size_t pktsize;
} connection;

size_t pkt_size_of[] =
{
	0,
	0,
	sizeof(XL3_Packet),
	sizeof(mtcPacket),
	sizeof(caenPacket),
	0
};

/* Globals */
struct event_base *base;
struct evdns_base *dnsbase;
struct evconnlistener *listener;
int have_controller = 0;
connection monitoring_cons[MAX_MON_CONS];
int cur_mon_con = 0;

/* Prototypes */
static void echo_read_cb(struct bufferevent *bev, void *ctx);
static void echo_event_cb(struct bufferevent *bev, short events, void *ctx);
