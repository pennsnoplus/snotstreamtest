```
   _[_]_  SNO+_MON + monitoring software
    (")             for the SNO+ project
`++( : )++'
  (  :  )
""`+...+'"" 
```

To compile everything, just use 'make'.

SNO+_MON tries to connect to defined monitor points, and
then takes the data from them and does stuff with it.

Yeah, that's vague, but that's the spec I have, too.

# CouchApp Resources #
+ [Anatomy of a CouchApp](http://mindeavor.com/blog/the-anatomy-of-a-couchapp)
+ [Formatting with Show and List](http://wiki.apache.org/couchdb/Formatting_with_Show_and_List)
+ [Evently and CouchApp](http://couchapp.couchone.com/docs/_design/docs/index.html#/topic/evently)
+ [jQuery Mobile + Templates + CouchApp](http://custardbelly.com/blog/2010/12/28/jquery-mobile-couchdb-part-3-templates-and-mustache-js/)
+ [Lists, Shows, and Models (slideshow)](http://xerexen.com/posts/lists-shows-and-models)
+ [Evently DIY pt. II - State](http://couchapp.org/page/evently-do-it-yourself-ii-state)
+ [Evently Primer](http://couchapp.org/page/evently-primer)
+ [Notes On Pages Files])(http://couchapp.org/page/NotesOnPagesFiles)
+ [Database Queries the CouchDB Way](http://sitr.us/2009/06/30/database-queries-the-couchdb-way.html)
+ [CouchDB - The Definitive Guide](http://guide.couchdb.org/index.html)
+ [Reference](http://daleharvey.github.com/jquery.couch.js-docs/symbols/%24.couch.db.html)
+ [Using jQuery and CouchDB for webapps](http://blog.edparcell.com/using-jquery-and-couchdb-to-build-a-simple-we)

# Notes #
+ possible to avoid doing data copies
    + http://www.wangafu.net/~nickm/libevent+book/Ref7_evbuffer.html
    + Avoiding data copies with evbuffer+based IO
+ threading vs. pure async
	+ threading seems easier
	+ should be possible to do multi_curl and libevent?
		+ I have not been successfull
		+ online examples
			+ sandbox.c
			+ hiperfifo.c

# Structure #
### Main ###
1. Set up an event_base (DONE)
    + only use epoll & kqueue (DONE)
    + create the config (DONE)
    + create the base (DONE)
    + delete the config (DONE)
2. Setup a dns_base (DONE)
    + make it asynchronous
    + this way, we can resolve hostnames asynchronously,
      as opposed to blocking on resolves.
2. Create a listener (DONE)
    + be ready to accept a controller (DONE)
 	+ have default connections?
3. Run the main loop (event_base_dispatch base)
    + accept controller commands: (DONE)
        + CREATE/DELETE connections (DONE)
        + SHOW          all connections (DONE)
		+ HELP			get list of valid commands (DONE)
    + get data from connections (filter buffers?) (DONE)
        + switch on the connection type, but basically: (DONE)
            + check to see how much data is in the buffer (DONE)
            + if there is enough data to fill a packet (DONE)
                + fill CONNECTION_TYPE packet (DONE)
                + if DEBUG: show data, information about data
                + UPLOAD to database (DONE)

### Callbacks ###
1. READ
    + uploads the data (YES)
2. WRITE
    + "successful write to <place>" (NO)
3. EVENT
    + nothing? (CLOSES CONNECTION)

