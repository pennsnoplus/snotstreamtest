typedef struct _ConnInfo {
	CURL *easy;	// easy handle
	char *url;	// destination;
	char error[CURL_ERROR_SIZE];
} ConnInfo;

typedef struct _SockInfo {
	curl_socket_t sockfd;	// socket to be monitored
	struct event ev;		// event on the socket
	int ev_is_set;			// whether or not ev is set and being monitored
	int action;				// what action libcurl wants done
} SockInfo;

/* Update the event timer after curl_multi library calls */
static int multi_timer_cb(CURLM *multi, long timeout_ms, GlobalInfo *g){
  struct timeval timeout;
  timeout.tv_sec = timeout_ms/1000;
  timeout.tv_usec = (timeout_ms%1000)*1000;
  fprintf(MSG_OUT, "multi_timer_cb: Setting timeout to %ld ms\n", timeout_ms);
  evtimer_del(&g->timer_event);
  evtimer_add(&g->timer_event, &timeout);
  return 0;
}

/* Used to debug return codes from curl_multi_socket_action */
static void debug_mcode(const char *desc, CURLMcode code){
	if (code != CURLM_OK){
		const char *s;
		switch (code) {
			case CURLM_CALL_MULTI_PERFORM:	s="CURLM_CALL_MULTI_PERFORM";break;
			case CURLM_BAD_HANDLE:			s="CURLM_BAD_HANDLE";break;
			case CURLM_BAD_EASY_HANDLE:		s="CURLM_BAD_EASY_HANDLE";break;
			case CURLM_OUT_OF_MEMORY:		s="CURLM_OUT_OF_MEMORY";break;
			case CURLM_INTERNAL_ERROR:		s="CURLM_INTERNAL_ERROR";break;
			case CURLM_UNKNOWN_OPTION:		s="CURLM_UNKNOWN_OPTION";break;
			case CURLM_LAST:				s="CURLM_LAST";break;
			case CURLM_BAD_SOCKET:			s="CURLM_BAD_SOCKET";break;
			default:						s="CURLM_unknown";
		}
		fprintf(MSG_OUT, "ERROR: %s returns %s\n", desc, s);
	}
}
			
/* Check for completed transfers, and remove their easy handles */
static void check_multi_info(){
	CURLMsg *msg;
	CURL *easy;
	CURLcode res;
	
	int msgs_left;
	ConnInfo *conn;

	while ((msg = curl_multi_info_read(multi, &msgs_left))){
		if(msg->msg == CURLMSG_DONE){ // if this action is done
			// unpack the message
			easy = msg->easy_handle;
			res = msg->data.result;
			curl_easy_getinfo(easy, CURLINFO_PRIVATE< &conn);
			// get rid of the easy handle
			curl_multi_remove_handle(multi, easy);
			curl_easy_cleanup(easy);
			// clean up the conn
			free(conn->url);
			free(con);
		}
	}
}

/* Sets up a SockInfo structure and starts libevent monitoring. */
static void setsock(SockInfo *fdp, curl_socket_t s, int action){
	int kind =
		(act&CURL_POLL_IN ? EV_READ:0)|
		(act&CURL_POLL_OUT ? EV_WRITE:0)|
		EV_PERSIST; // always want persist
	fdp->action = act;
	fdp->sockfd = s;
	if (fdp->ev_is_set){
		event_del(&fdp->ev);
		fdp->ev_is_set = 0;
	}
	event_set(&fdp->ev, fdp->sockfd, kind, event_cb, NULL);
	event_base_set(base, &fdp->ev); // set the event to use the global event base
	fdp->ev_is_set = 1; // mark the event as set
	event_add(&f->ev, NULL); // add the event with no timeout (NULL)
}

/* The CURLMOPT_SOCKETFUNCTION. This is what tells libevent to start or stop
watching for events on different sockets. */
static int sock_cb(CURL *e, curl_socket_t s, int action, void *cbp, void *sockp){
	/*
	e       = easy handle that the callback happened on,
	s       = actual socket that is involved,
	action  = what to check for / do (?) (nothing, IN, OUT, INOUT, REMOVE)
	cbp     = private callback pointer (from where? ignored!)
	socketp = private data set with setop(CURLOPT_PRIVATE) // TODO: set this in POUCH
	*/
	SockInfo *fdp = (SockInfo *)sockp;
	if (action == CURL_POLL_REMOVE){
		// stop watching this socket for events
		if(fdp){
			if(fdp->ev_is_set){
				event_del(fdp->ev);
			}
			free(fdp);
		}
	}
	else {
		if (!fdp) {
			// start watching this socket for events
			SockInfo *fdp = calloc(1, sizeof(SockInfo));
			setsock(fdp, s, action);
		}
		else {
			// reset the sock with the new type of action
			setsock(fdp, s, action);
		}
	}
	return 0;
}

/* Called by libevent when there is action on a socket being watched
by the libcurl multi interface */
static void event_cb(int fd, short kind, void *userp){
	CURLMcode rc; // result from curl_multi_socket_action
	int action = 
		(kind & EV_READ ? CURL_CSELECT_IN : 0) |
		(kind & EV_WRITE ? CURL_CSELECT_OUT : 0);
	rc = curl_multi_socket_action(multi, fd, action, &still_running);
	
	debug_mcode("event_cb: curl_multi_socket_action", rc);
	
	check_multi_info();
	if (still_running <= 0){ // last transfer is done
		if (evtimer_pending(&timer_event, NULL)){
			evtimer_del(&timer_event); // get rid of the libevent timer
		}
	}
}

/* Called by libevent when our timeout expires */
static void timer_cb(int fd, short kind, void *userp){
	CURLMcode rc; // result from curl_multi_socket_action
	rc = curl_multi_socket_action(multi, CURL_SOCKET_TIMEOUT, 0, &still_running);
	debug_mcode("timer_cb: curl_multi_socket_action", rc);
	check_multi_info();
}

/* The function to deal with data received from an easy_handle (read data in)*/
static size_t write_cb(void *ptr, size_t size, size_t nmemb, void *data){
	// data is the conn that this thing came from
	return size*nmemb; // do nothing, claim that the data was read
}

/* Create a new easy handle, and add it to the global curl_multi.
In the future, replace this by updating pouch and letting it work with
the libcurl multi interface. Make sure to include the CURLOPT_PRIVATE setting,
and change everything over from ConnInfo to the POUCH equivalent? TODO*/
static void new_conn(char *url){
	ConnInfo *conn;
	CURLMcode rc;
	conn = calloc(1, sizeof(ConnInfo));
	conn->error[0]='\0'; // null terminate the string ? may not be necessary... (TODO remove, not necessary)
	conn->easy = curl_easy_init();
	if (!conn->easy){
		fprintf(MSG_OUT, "curl_easy_init() failed, exiting!\n");
		exit(2);
	}
	conn->url = strdup(url);
	curl_easy_setopt(conn->easy, CURLOPT_URL, conn->url);
	curl_easy_setopt(conn->easy, CURLOPT_WRITEFUNCTION, write_cb);
	curl_easy_setopt(conn->easy, CURLOPT_WRITEDATA, &conn);
	curl_easy_setopt(conn->easy, CURLOPT_VERBOSE, 1L);
	curl_easy_setopt(conn->easy, CURLOPT_ERRORBUFFER, conn->error);
	curl_easy_setopt(conn->easy, CURLOPT_PRIVATE, conn);
	curl_easy_setopt(conn->easy, CURLOPT_NOPROGRESS, 1L);
	fprintf(MSG_OUT, "Adding easy %p to multi %p (%s\n", conn->easy, multi, url);
	rc = curl_multi_add_handle(multi, conn->easy);
	debug_mcode("new_conn: curl_multi_add_handle", rc);
	/* curl_multi_add_handle() will set a time-out to trigger very soon so
	   that the necessary socket_action() call will be called by this app */
}
