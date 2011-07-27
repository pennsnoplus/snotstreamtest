   _[_]_  SNO+_MON - monitoring software
    (")             for the SNO+ project
`--( : )--'
  (  :  )
""`-...-'"" 

To compile everything, just use 'make'.

SNO+_MON tries to connect to defined monitor points, and
then takes the data from them and does stuff with it.

Yeah, that's vague, but that's the spec I have, too.

## Notes ##
- possible to avoid doing data copies
    - http://www.wangafu.net/~nickm/libevent-book/Ref7_evbuffer.html
    - Avoiding data copies with evbuffer-based IO
- threading vs. pure async
	- threading seems easier
	- should be possible to do multi_curl and libevent?
		- I have not been successfull
		- online examples
			- sandbox.c
			- hiperfifo.c

## Structure ##
1. Main
    <del> 1. Set up an event_base</del>
        BOOM - only use epoll & kqueue
        BOOM - create the config
        BOOM - create the base
        BOOM - delete the config
    BOOM 2. Setup a dns_base
        BOOM - make it asynchronous
        BOOM - this way, we can resolve hostnames asynchronously,
          as opposed to blocking on resolves.
        - 
    BOOM 2. Create a listener
        BOOM - be ready to accept a controller
        NOPE //- do nothing until the controller is accepted
    3. Run the main loop (event_base_dispatch base)
        - accept controller commands:
            BOOM - CREATE/DELETE connections
            BOOM - SHOW          all connections
            - DEBUG         connections
        - get data from connections (filter buffers?)
            - switch on the connection type, but basically:
                - check to see how much data is in the buffer
                - if there is enough data to fill a packet:
                    - fill CONNECTION_TYPE packet
                    - if DEBUG:
                        - show data, information about data
                    - UPLOAD to database
2. Callbacks
    1. READ
        - uploads the data
    2. WRITE
        - "successful write to <place>"
    3. EVENT
        - nothing?

