#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>

#include <event2/dns.h>
#include <event2/util.h>
#include <event2/event.h>

#include <arpa/inet.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#define MAX_MON_CONS 10

struct event_base *base;
struct evdns_base *dnsbase;
struct evconnlistener *listener;
int have_controller = 0;

typedef struct {
    struct bufferevent *bev;
    char *type;
    char *host;
    int port;
    int valid;
} connection;

void connection_settype(connection *con, char *type){
    con->type = (char *)malloc(strlen(type)+1);
    strcpy(con->type, type);
}
void connection_sethost(connection *con, char *host){
    con->host = (char *)malloc(strlen(host)+1);
    strcpy(con->host, host);
}
void connection_free(connection *con){
    free(con->type);
    free(con->host);
    bufferevent_free(con->bev);
}

connection monitoring_cons[MAX_MON_CONS];
int cur_mon_con = 0;

void print_connections(){
    //TODO: //FIXME: after freeing a connection, it can still be printed out
    // if there are 3 connections, and we stop monitoring the 2nd one, then it's
    // still going to be "printed", but it's just garbage.
    if (cur_mon_con == 0){
        printf("No monitoring connections.\n");
        return;
    }
    int i;
    connection *con;
    for(i = 0; i < cur_mon_con; i++){
        con = &monitoring_cons[i];
        printf("%s %s:%d\n", con->type, con->host, con->port);
    }
}
static void echo_read_cb(struct bufferevent *bev, void *ctx){
    /* This callback is invoked when there is data to read on bev. */
    struct evbuffer *input = bufferevent_get_input(bev);
    struct evbuffer *output = bufferevent_get_output(bev);

    /* Copy all the data from the input buffer to the output buffer. */
    evbuffer_add_buffer(output, input);
}

    static void echo_event_cb(struct bufferevent *bev, short events, void *ctx){
        if (events & BEV_EVENT_ERROR)
            perror("Error from bufferevent");
        if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
            bufferevent_free(bev);
        }
    }
/* Controller callbacks */
static void controller_cb_read(struct bufferevent *bev, void *ctx){
    struct evbuffer *input = bufferevent_get_input(bev);

    char inbuf[100];
    memset(&inbuf, 0, 100);

    int n;
    while ((n = evbuffer_remove(input, inbuf, sizeof(inbuf))) > 0) {
        // this copies the data from input to the inbuf
    }
    printf("Controller: %s", inbuf);
    if (inbuf[strlen(inbuf)-1] != '\n')
        fputc('\n', stdout);
    if (!strncmp("print_connections", inbuf, 17))
        print_connections();
    else if(!strncmp("stop", inbuf, 4)){
        if (!cur_mon_con)
            printf("No connections to stop.\n");
        else {
            char *type;
            char *host;
            char *portstr;
            portstr = strtok(inbuf, " "); // get rid of command name
            int i;
            for(i = 0; i < 3 && portstr != NULL; i++){
                printf("portstr = %s\n", portstr);
                portstr = strtok(NULL, " ");
                if (portstr == NULL)
                    break;
                if (i == 0)
                    type = portstr;
                else if (i == 1)
                    host = portstr;
            }
            printf("i = %d\n", i);
            int error;
            if (i != 3){
                printf("Incorrect number of arguments\n");
                printf("stop <type> <host> <port>\n");
            }
            else {
                int port = atoi(portstr);
                int j;
                connection *con;
                for(j = 0; j < cur_mon_con; j++){
                   con = &monitoring_cons[j];
                   if (!strcmp(con->type, type)){
                       if (!strcmp(con->host, host)){
                           if (con->port == port){
                               printf("Stopped monitoring %s %s:%d\n", con->type, con->host, con->port);
                               connection_free(con);
                               break;
                           }
                       }
                   }
                }
            }
        }
    }
    else if (!strncmp("start", inbuf, 5)){
        if (cur_mon_con < MAX_MON_CONS) { // we can still make new cons
            connection *con = &monitoring_cons[cur_mon_con];
            con->bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
            bufferevent_setcb(con->bev, echo_read_cb, NULL, echo_event_cb, base);
            bufferevent_enable(con->bev, EV_READ|EV_WRITE);
            //FIXME: new option parsing thingy

            // howto: create xl3 localhost 4040
            char *type;
            char *host;
            char *portstr;
            portstr = strtok(inbuf, " "); // get rid of command name
            int i;
            for(i = 0; i < 3 && portstr != NULL; i++){
                printf("portstr = %s\n", portstr);
                portstr = strtok(NULL, " ");
                if (portstr == NULL)
                    break;
                if (i == 0)
                    type = portstr;
                else if (i == 1)
                    host = portstr;
            }
            printf("i = %d\n", i);
            int error;
            if (i != 3){
                printf("Incorrect number of arguments\n");
                printf("create <type> <host> <port>\n");
                bufferevent_free(con->bev);
            }
            else {
                connection_settype(con, type);
                connection_sethost(con, host);
                con->port = atoi(portstr);
                if (!bufferevent_socket_connect_hostname(con->bev, dnsbase, AF_UNSPEC, con->host, con->port)){
                    printf("Created socket\n");
                    cur_mon_con++;
                }
                else{
                    printf("Unable to create socket\n");
                    connection_free(con);
                }
            }
        }
        //bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
        //bufferevent_setcb(bev, echo_read_cb, NULL, echo_event_cb, base);
        //bufferevent_enable(bev, EV_READ|EV_WRITE);
        //evbuffer_add_printf(bufferevent_get_output(bev), "GET %s\r\n", argv[2]);
        //bufferevent_socket_connect_hostname(bev, dnsbase, AF_UNSPEC, argv[1], 80);
        //bufferevent_socket_connect_hostname(bev, dnsbase, AF_UNSPEC, "localhost", 4040);
    }
}
    static void controller_cb_event(struct bufferevent *bev, short events, void *ctx){
        if (events & BEV_EVENT_ERROR)
            perror("Error from bufferevent (controller)");
        if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
            bufferevent_free(bev);
            have_controller--;
            fprintf(stderr, "Closed controller connection.\n");
        }
    }
static void controller_cb_accept(struct evconnlistener *listener,evutil_socket_t fd, struct sockaddr *address, int socklen,void *ctx){
    if (!have_controller){
        struct event_base *base = evconnlistener_get_base(listener);
        struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
        bufferevent_setcb(bev, controller_cb_read, NULL, controller_cb_event, NULL);
        bufferevent_enable(bev, EV_READ|EV_WRITE);
        have_controller++;
    }
    else {
        EVUTIL_CLOSESOCKET(fd);
        fprintf(stderr, "Closed extra controller connection\n");
    }
}

static void accept_error_cb(struct evconnlistener *listener, void *ctx){
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
    if (port<=0 || port>65535) {
        puts("Invalid port");
        return 1;
    }

    // Configure our event_base to be fast
    struct event_config *cfg;
    cfg = event_config_new();
    event_config_avoid_method(cfg, "select"); // We don't want no select
    event_config_avoid_method(cfg, "poll"); // NO to poll

    base = event_base_new_with_config(cfg); // use the custom config
    event_config_free(cfg); // all done with this
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
    for(i=0; methods[i] != NULL; ++i){
        printf("\t%s\n", methods[i]);
    }
    free((char **)methods);
    printf("Using %s.\n", event_base_get_method(base));

    // Create the listener
    struct sockaddr_in sin;
    /* Clear the sockaddr before using it, in case there are extra
     * platform-specific fields that can mess us up. */
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;/* This is an INET address */
    sin.sin_addr.s_addr = htonl(0);/* Listen on 0.0.0.0 */
    sin.sin_port = htons(port);/* Listen on the given port. */
    listener = evconnlistener_new_bind(base, controller_cb_accept, NULL,
            LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE, -1,
            (struct sockaddr*)&sin, sizeof(sin));
    if (!listener) {
        perror("Couldn't create listener");
        return 1;
    }
    evconnlistener_set_error_cb(listener, accept_error_cb);

    // Run the main loop
    event_base_dispatch(base);

    // Clean up
    evdns_base_free(dnsbase, 0);
    event_base_free(base);
    return 0;
}
