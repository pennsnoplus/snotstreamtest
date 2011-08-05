```
   _[_]_  SNO+_MON + monitoring software
    (")             for the SNO+ project
`++( : )++'
  (  :  )
""`+...+'"" 
```

# Description #
SNO+_MON tries to connect to defined monitor points, and
then takes the data from them and does stuff with it.

Yeah, that's vague, but that's the spec I have, too.

# Run the Demo #

The way the demo program works is not at all simple.
Fake XL3 packets are generated and sent to a CouchDB
server hosted by Cloudant - view the source code to
see exactly where.

After the packets are uploaded, pretty graphs are created
in Javascript using the Flot graphing library. [Here is that page.](http://snoplus.cloudant.com/pmt_test/_design/grapher/index.html)

Data flow is as follows:

```xl3_data_generator/porca``` -------> ```penn_daq``` -------> ```monitor``` -------> CouchDB instance

To set up and run a demo, you must also have a copy of (Penn_daq)[https://github.com/pennsnoplus/Penn_daq].

To compile penn_daq:
 - ```cd path/to/penn/daq/directory```
 - ```make clean; make```
To compile monitor and porca:
 - ```cd path/to/snot_mon/```
 - ```make clean; make```
 - ```cd xl3_data_generator```
 - ```./make.sh```
To start the demo, start penn_daq. Run the monitor. Using either telnet or tut (which is included in the Penn_daq directory), connect to the monitor (```(telnet|tut) localhost 2020```). Make sure that the monitor accepts this connection; try typing some input and pressing return, the monitor should print out what you typed. Then, from the telnet/tut connection, send ```start xl3 localhost 44598```; the monitor should say that it has started a connection, and penn_daq should print out that it has a monitor connected. Now, run porca. This will start generating XL3 data, which gets passed to penn_daq, from there to monitor, and from there to the CouchDB instance. Leave all of this running and check out the graph page - everything should be updating. If you're having trouble with any of these steps, feel free to email peter.l.downs@gmail.com. 

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
