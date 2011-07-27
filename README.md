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
    1. Set up an event_base DONE
        - only use epoll & kqueue DONE
        - create the config DONE
        - create the base DONE
        - delete the config DONE
    2. Setup a dns_base DONE
        - make it asynchronous
        - this way, we can resolve hostnames asynchronously,
          as opposed to blocking on resolves.
    2. Create a listener DONE
        - be ready to accept a controller DONE
        NOPE //- do nothing until the controller is accepted
			- have default connections?
    3. Run the main loop (event_base_dispatch base)
        - accept controller commands: DONE
            - CREATE/DELETE connections DONE
            - SHOW          all connections DONE
			- HELP			get list of valid commands
        - get data from connections (filter buffers?) DONE
            - switch on the connection type, but basically: DONE
                - check to see how much data is in the buffer DONE
                - if there is enough data to fill a packet: DONE
                    - fill CONNECTION_TYPE packet DONE
                    - if DEBUG:
                        - show data, information about data
                    - UPLOAD to database DONE
2. Callbacks
    1. READ
        - uploads the data
    2. WRITE
        - "successful write to <place>"
    3. EVENT
        - nothing?

