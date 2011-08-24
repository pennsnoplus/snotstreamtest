// Standard libraries
#include <sys/stat.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>

// Libevent and Libcurl
#include <event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/dns.h>
#include <event2/util.h>
#include <event2/event.h>
#include <event2/event_struct.h>
#include <curl/curl.h>

#include "pouch.h"

// Miscellaneous helper functions
char *url_escape(CURL *curl, char *str){
	/*
	   URL escapes a string. Use this to 
	   escape database names.
	 */
	return curl_easy_escape(curl, str, strlen(str));
}
char *combine(char **out, char *f, char *s, char *sep){
	/*
	   Appends the strings f, sep, and s, in that order,
	   and copies the result to *out. If a separator is
	   unnecessary, call the function with sep=NULL. *out
	   must have been malloced() or initiated as NULL.
	 */
	size_t length = 0;
	length += strlen(f);
	length += strlen(s);
	if(sep)
		length += strlen(sep);
	length++; // must have room for terminating \0
	char buf[length];
	if(sep)
		sprintf(buf, "%s%s%s", f, sep, s);
	else
		sprintf(buf, "%s%s", f, s);
	buf[length-1] = '\0'; // null terminate
	if(*out){
		free(*out);
	}
	*out = (char *)malloc(length);
	memcpy(*out, buf, length);
	return *out;
}

// PouchReq functions
PouchReq *pr_init(void){
	/*
	   Initializes a new PouchReq
	   object.
	 */
	PouchReq *pr = calloc(1, sizeof(PouchReq));

	// initializes the request buffer
	pr->req.offset = pr->req.data = NULL;
	pr->req.size = 0;

	// initializes the response buffer
	pr->resp.offset = pr->resp.data = NULL;
	pr->resp.size = 0;

	return pr;
}
PouchReq *pr_add_header(PouchReq *pr, char *h){
	/*
	   Add a custom header to a request.
	 */
	pr->headers = curl_slist_append(pr->headers, h);
	return pr;
}
PouchReq *pr_add_usrpwd(PouchReq *pr, char *usrpwd, size_t length){
	if (pr->usrpwd){
		free(pr->usrpwd);
	}
	pr->usrpwd = (char *)malloc(length);
	memcpy(pr->usrpwd, usrpwd, length);
	return pr;
}
PouchReq *pr_add_param(PouchReq *pr, char *key, char *value){
	/*
	   Adds a parameter to a request's URL string,
	   regardless of whether or not other parameters already exist.
	 */
	pr->url = (char *)realloc(pr->url, // 3: new ? or &, new =, new '\0'
			strlen(pr->url) + 3 + sizeof(char)*(strlen(key)+strlen(value)));
	if (strchr(pr->url, '?') == NULL){
		strcat(pr->url, "?");
	}
	else{
		strcat(pr->url, "&");
	}
	strcat(pr->url, key);
	strcat(pr->url, "=");
	strcat(pr->url, value);
	strcat(pr->url, "\0");
	return pr;
}
PouchReq *pr_clear_params(PouchReq *pr){
	/*
	   Removes all parameters from a request's URL string,
	   if they exist. Otherwise, the URL string is left alone.
	 */
	char *div;
	if ( (div = strchr(pr->url, '?')) != NULL){ // if there are any params
		char *temp = &pr->url[strlen(pr->url)]; // end of the string
		while (*temp != '?'){
			*temp = '\0'; // wipe out the old character
			temp--;	// move back another character
		}
		*temp = '\0'; // get rid of the ?
	}
	return pr;
}
PouchReq *pr_set_method(PouchReq *pr, char *method){
	/*
	   Sets the HTTP method of
	   a specific request.
	 */
	size_t length = strlen(method)+1; // include '\0' terminator
	if (pr->method)
		free(pr->method);
	pr->method = (char *)malloc(length); // allocate space
	memcpy(pr->method, method, length);	 // copy the method
	return pr;
}
PouchReq *pr_set_url(PouchReq *pr, char *url){
	/*
	   Sets the target URL of
	   a CouchDB request.
	 */
	size_t length = strlen(url)+1; // include '\0' terminator
	if (pr->url)	// if there is an older url, get rid of it
		free(pr->url);
	pr->url = (char *)malloc(length); // allocate space
	memcpy(pr->url, url, length);	  // copy the new url

	return pr;
}
PouchReq *pr_set_data(PouchReq *pr, char *str){
	/*
	   Sets the data that a request
	   sends. If the request does not
	   need to send data, do NOT call
	   this function with an empty string,
	   just refrain from calling the function.
	 */
	size_t length = strlen(str);
	if (pr->req.data){	// free older data
		free(pr->req.data);
	}
	pr->req.data = (char *)malloc(length+1);	// allocate space, include '\0'
	memset(pr->req.data, '\0', length+1);		// write nulls to the new space
	memcpy(pr->req.data, str, length);	// copy over the data

	// Because of the way CURL sends data,
	// before sending the PouchPkt's
	// offset must point to the same address
	// in memory as the data pointer does.
	pr->req.offset = pr->req.data;
	pr->req.size = length; // do not send the last '\0' - JSON is not null terminated
	return pr;
}
PouchReq *pr_set_bdata(PouchReq *pr, void *dat, size_t length){
	if (pr->req.data){
		free(pr->req.data);
	}
	pr->req.data = (char *)malloc(length);
	memcpy(pr->req.data, dat, length);
	pr->req.offset = pr->req.data;
	pr->req.size = length;
	return pr;
}
PouchReq *pr_clear_data(PouchReq *pr){
	/*
	   Removes all data from a request's
	   data buffer, if it exists.
	 */
	if (pr->req.data){
		free(pr->req.data);
		pr->req.data = NULL;
	}
	pr->req.size = 0;
	return pr;
}
void pr_free(PouchReq *pr){
	/*
	   Frees any memory that may have been
	   allocated during the creation / 
	   processing of a request. Although it
	   is ok to reuse requests, this
	   MUST be called at the end of your program
	   in order to not leak memory like Assange
	   leaks secret documents.
	 */
	if (pr->easy){	// free request and remove it from multi
		if (pr->multi){
			curl_multi_remove_handle(pr->multi, pr->easy);
		}
		curl_easy_cleanup(pr->easy);
	}
	if (pr->resp.data){			// free response data
		free(pr->resp.data);
	}if (pr->req.data){
		free(pr->req.data);		// free request data
	}if (pr->method){			// free method string
		free(pr->method);
	}if (pr->url){				// free URL string
		free(pr->url);
	}if (pr->headers){
		curl_slist_free_all(pr->headers);	// free headers
	}if (pr->usrpwd){
		free(pr->usrpwd);
	}
	free(pr);				// free structure
}

// generic callback functions
size_t recv_data_callback(char *ptr, size_t size, size_t nmemb, void *data){
	/*
	   This callback is used to save responses from CURL requests.
	   It loads the response into the PouchReq pointed to by
	   data.
	 */
	size_t ptrsize = nmemb*size; // this is the size of the data pointed to by ptr
	PouchReq *pr = (PouchReq *)data;
	pr->resp.data = (char *)realloc(pr->resp.data, pr->resp.size + ptrsize +1);
	if (pr->resp.data){	// realloc was successful
		memcpy(&(pr->resp.data[pr->resp.size]), ptr, ptrsize); // append new data
		pr->resp.size += ptrsize;
		pr->resp.data[pr->resp.size] = '\0'; // null terminate the new data
	}
	else { // realloc was NOT successful
		fprintf(stderr, "recv_data_callback: realloc failed\n");
	}
	return ptrsize; // theoretically, this is the amount of processed data
}
size_t send_data_callback(void *ptr, size_t size, size_t nmemb, void *data){
	/*
	   This callback is used to send data for a CURL request. The JSON
	   string stored in the PouchReq pointed to by data is read out
	   and sent, piece by piece.
	 */
	size_t maxcopysize = nmemb*size;
	if (maxcopysize < 1){
		return 0;
	}
	PouchReq *pr = (PouchReq *)data;
	if (pr->req.size > 0){ // only send data if there's data to send
		size_t tocopy = (pr->req.size > maxcopysize) ? maxcopysize : pr->req.size;
		memcpy(ptr, pr->req.offset, tocopy);
		pr->req.offset += tocopy;	// advance our offset by the number of bytes already sent
		pr->req.size -= tocopy;	//next time there are tocopy fewer bytes to copy
		return tocopy;
	}
	return 0;
}

// libevent/multi interface helpers and callbacks
void debug_mcode(const char *desc, CURLMcode code){
	if ((code != CURLM_OK) && (code != CURLM_CALL_MULTI_PERFORM)){
		const char *s;
		switch (code) {
			//case CURLM_CALL_MULTI_PERFORM:	s="CURLM_CALL_MULTI_PERFORM";break; //ignore
			case CURLM_BAD_HANDLE:			s="CURLM_BAD_HANDLE";break;
			case CURLM_BAD_EASY_HANDLE:		s="CURLM_BAD_EASY_HANDLE";break;
			case CURLM_OUT_OF_MEMORY:		s="CURLM_OUT_OF_MEMORY";break;
			case CURLM_INTERNAL_ERROR:		s="CURLM_INTERNAL_ERROR";break;
			case CURLM_UNKNOWN_OPTION:		s="CURLM_UNKNOWN_OPTION";break;
			case CURLM_LAST:				s="CURLM_LAST";break;
			case CURLM_BAD_SOCKET:			s="CURLM_BAD_SOCKET";break;
			default:						s="CURLM_unknown";
		}
		fprintf(stderr, "ERROR: %s returns %s\n", desc, s);
	}
}
void check_multi_info(PouchMInfo *pmi /*, function pointer process_func*/){
	// TODO: implement a callback here to handle completed functions
	// NOTE: keep it in PouchMInfo? this function already takes that
	CURLMsg *msg;
	CURL *easy;
	CURLcode res;
	
	int msgs_left;
	PouchReq *pr;

	while ((msg = curl_multi_info_read(pmi->multi, &msgs_left))){
		if(msg->msg == CURLMSG_DONE){ // if this action is done
			// unpack the message
			easy = msg->easy_handle;
			res = msg->data.result;
			curl_easy_getinfo(easy, CURLINFO_PRIVATE, &pr);
			printf("Finished request (easy=%p, url=%s)\n", easy, pr->url);
			printf("Response: %s\n", pr->resp.data);
			// process the result
			//pmi->process_func(pr);
			// TODO: move this to process function?
			// clean up the pouch request and associated CURL handle
			pr_free(pr);
		}
	}
}
int multi_timer_cb(CURLM *multi, long timeout_ms, void *data){
	/*
		Update the event timer after curl_multi
		library calls.
	*/
	PouchMInfo *pmi = (PouchMInfo *)data;
	struct timeval timeout;
	timeout.tv_sec = timeout_ms/1000;
	timeout.tv_usec = (timeout_ms%1000)*1000;
	fprintf(stderr, "multi_timer_cb: Setting timeout to %ld ms\n", timeout_ms);
	if (evtimer_pending(&pmi->timer_event, NULL)){
		evtimer_del(&pmi->timer_event);
	}
	evtimer_add(&pmi->timer_event, &timeout);
	return 0;
}
void event_cb(int fd, short kind, void *userp){
	printf("event!\n");
	/*
		Called by libevent when there is any type
		of action on a socket being watched.
	 */
	PouchMInfo *pmi = (PouchMInfo *)userp;
	CURLMcode rc; // result from curl_multi_socket_action
	int action = 
		(kind & EV_READ ? CURL_CSELECT_IN : 0) |
		(kind & EV_WRITE ? CURL_CSELECT_OUT : 0);
	rc = curl_multi_socket_action(pmi->multi, fd, action, &pmi->still_running);
	
	debug_mcode("event_cb: curl_multi_socket_action", rc);
	
	check_multi_info(pmi);
	if (pmi->still_running <= 0){ // last transfer is done
		if (evtimer_pending(&pmi->timer_event, NULL)){
			evtimer_del(&pmi->timer_event); // get rid of the libevent timer
		}
	}
}
void timer_cb(int fd, short kind, void *userp){
	/*
		Called by libevent when the global timeout
		that is used to take action on different easy_handles
		expires.
	*/
	PouchMInfo *pmi = (PouchMInfo *)userp;
	CURLMcode rc; // result from curl_multi_socket_action
	rc = curl_multi_socket_action(pmi->multi, CURL_SOCKET_TIMEOUT, 0, &pmi->still_running);
	debug_mcode("timer_cb: curl_multi_socket_action", rc);
	check_multi_info(pmi);
}
void setsock(SockInfo *fdp, curl_socket_t s, int action, PouchMInfo *pmi){
	/*
		Sets up a SockInfo structure and starts libevent
		monitoring on a socket.
	*/
	int kind =
		(action&CURL_POLL_IN ? EV_READ:0)|
		(action&CURL_POLL_OUT ? EV_WRITE:0)|
		EV_PERSIST; // always want persist
	fdp->action = action;
	fdp->sockfd = s;
	if (fdp->ev_is_set){
		event_del(&fdp->ev);
		fdp->ev_is_set = 0;
	}
	event_set(&fdp->ev, fdp->sockfd, kind, event_cb, pmi);
	event_base_set(pmi->base, &fdp->ev); // set the event to use the global event base
	fdp->ev_is_set = 1; // mark the event as set
	event_add(&fdp->ev, NULL); // add the event with no timeout (NULL)
}
int sock_cb(CURL *e, curl_socket_t s, int action, void *cbp, void *sockp){
	/*
		The CURLMOPT_SOCKETFUNCTION. This is what tells libevent to start
		or stop watching for events on different sockets.

		e       = easy handle that the callback happened on,
		s       = actual socket that is involved,
		action  = what to check for / do (?) (nothing, IN, OUT, INOUT, REMOVE)
		cbp     = private data set by curl_multi_setop(CURLMOPT_SOCKETDATA)
		sockp	= private data set by curl_multi_assign(multi, socket, sockp)
	*/
	PouchMInfo *pmi = (PouchMInfo *)cbp;
	SockInfo *fdp = (SockInfo *)sockp;
	if (action == CURL_POLL_REMOVE){
		// stop watching this socket for events
		if(fdp){
			if(fdp->ev_is_set){
				event_del(&fdp->ev);
			}
			free(fdp);
		}
	}
	else {
		if (!fdp) {
			// start watching this socket for events
			SockInfo *fdp = calloc(1, sizeof(SockInfo));
			setsock(fdp, s, action, pmi);
			curl_multi_assign(pmi->multi, s, fdp);
		}
		else {
			// reset the sock with the new type of action
			setsock(fdp, s, action, pmi);
		}
	}
	return 0;
}
PouchMInfo *pouch_multi_init(PouchMInfo *pmi, struct event_base *base){
	/*
		Initialize a PouchMInfo structure for use with libevent.
		This creates and initializes the CURLM handle pointer,
		as well as the libevent timer_event which is used to deal
		with the multi interface, but takes in a pointer to a 
		libevent event_base for use as a sort of "global" base throughout
		all of the callbacks, so that the user can define their own base.
	*/
	pmi->base = base;
	pmi->still_running = 0;
	pmi->multi = curl_multi_init();
	evtimer_set(&pmi->timer_event, timer_cb, (void *)pmi);
	event_base_set(pmi->base, &pmi->timer_event);
	// setup the generic multi interface options we want
	curl_multi_setopt(pmi->multi, CURLMOPT_SOCKETFUNCTION, sock_cb);
	curl_multi_setopt(pmi->multi, CURLMOPT_SOCKETDATA, pmi);
	curl_multi_setopt(pmi->multi, CURLMOPT_TIMERFUNCTION, multi_timer_cb);
	curl_multi_setopt(pmi->multi, CURLMOPT_TIMERDATA, pmi);
	return pmi;
}
void *pouch_multi_delete(PouchMInfo *pmi){
	/*
		Cleans up and deletes a PouchMInfo struct.
		It gets rid of the timer event, frees the event base
		(don't do this manuallya fter calling pouch_multi_delete!)
		and cleans up the CURLM handle. Afterwards, it frees
		the object. Don't try to free it again.
	*/
	event_del(&pmi->timer_event);
	event_base_free(pmi->base);
	curl_multi_cleanup(pmi->multi);
	free(pmi);
}

PouchReq *pr_domulti(PouchReq *pr, CURLM *multi){
	// empty the response buffer
	if (pr->resp.data){
		free(pr->resp.data);
	}
	pr->resp.data = NULL;
	pr->resp.size = 0;

	// initialize the CURL object
	pr->easy = curl_easy_init();
	pr->multi = multi;
	
	// setup the CURL object/request
	curl_easy_setopt(pr->easy, CURLOPT_USERAGENT, "pouch/0.1");				// add user-agent
	curl_easy_setopt(pr->easy, CURLOPT_URL, pr->url);						// where to send this request
	curl_easy_setopt(pr->easy, CURLOPT_CONNECTTIMEOUT, 2);					// Timeouts
	curl_easy_setopt(pr->easy, CURLOPT_TIMEOUT, 2);
	curl_easy_setopt(pr->easy, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(pr->easy, CURLOPT_WRITEFUNCTION, recv_data_callback);	// where to store the response
	curl_easy_setopt(pr->easy, CURLOPT_WRITEDATA, (void *)pr);
	curl_easy_setopt(pr->easy, CURLOPT_PRIVATE, (void *)pr);				// associate this request with the PouchReq holding it
	curl_easy_setopt(pr->easy, CURLOPT_NOPROGRESS, 1L);						// Don't use a progress function to watch this request
	curl_easy_setopt(pr->easy, CURLOPT_ERRORBUFFER, pr->errorstr);			// Store multi error descriptions in pr->errorstr
	
	if(pr->usrpwd){	// if there's a valid auth string, use it
		curl_easy_setopt(pr->easy, CURLOPT_USERPWD, pr->usrpwd);
	}

	if (pr->req.data && pr->req.size > 0){ // check for data upload
		//printf("--> %s\n", pr->req.data);
		// let CURL know what data to send
		curl_easy_setopt(pr->easy, CURLOPT_READFUNCTION, send_data_callback);
		curl_easy_setopt(pr->easy, CURLOPT_READDATA, (void *)pr);
	}

	if (!strncmp(pr->method, PUT, 3)){ // PUT-specific option
		curl_easy_setopt(pr->easy, CURLOPT_UPLOAD, 1);
		// Note: Content-Type: application/json is automatically assumed
	}
	else if (!strncmp(pr->method, POST, 4)){ // POST-specific options
		curl_easy_setopt(pr->easy, CURLOPT_POST, 1);
		pr_add_header(pr, "Content-Type: application/json");
	}

	if (!strncmp(pr->method, HEAD, 4)){ // HEAD-specific options
		curl_easy_setopt(pr->easy, CURLOPT_NOBODY, 1); // no body to this request, just a head
		curl_easy_setopt(pr->easy, CURLOPT_HEADER, 1); // includes header in body output (for debugging)
	} // THIS FIXED HEAD REQUESTS
	else {
		curl_easy_setopt(pr->easy, CURLOPT_CUSTOMREQUEST, pr->method);
	} 

	// add the custom headers
	pr_add_header(pr, "Transfer-Encoding: chunked");
	curl_easy_setopt(pr->easy, CURLOPT_HTTPHEADER, pr->headers);

	// start the request by adding it to the multi handle
	pr->curlmcode = curl_multi_add_handle(pr->multi, pr->easy);
	printf("pr->curlmcode = %d\n", pr->curlmcode);
	debug_mcode("pr_domulti: ", pr->curlmcode);
	
	return pr;
	/*
	if (pr->headers){
		curl_slist_free_all(pr->headers);	// free headers
		pr->headers = NULL;
	}
	if (!pr->curlcode){
		pr->curlcode = curl_easy_getinfo(pr->easy, CURLINFO_RESPONSE_CODE, &pr->httpresponse);
		if (pr->curlcode != CURLE_OK)
			pr->httpresponse = 500; //  Internal Server Error (no specific error)
	}
	curl_easy_cleanup(curl);		// clean up the curl object

	// Print the response
	//printf("Received %d bytes, status = %d\n",
	//		(int)pr->resp.size, pr->curlcode);
	//printf("--> %s\n", pr->resp.data);
	*/
}
