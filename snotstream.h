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

