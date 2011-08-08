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

That's the vague version. Here's the full explanation.

The program monitor.c is monitoring software written
using [libevent](http://monkey.org/~provos/libevent/) for
asynchronous IO. It's primary purpose is to, well, monitor data connections.
By adding different types of data structures to the program,
it's possible to monitor any well-defined type of data. Currently,
the only defined monitoring type is for the XL3 - check out
[```pkt_types.h```](https://github.com/pennsnoplus/snot_mon/blob/master/pkt_types.h) to see the
definition. 

### How does the monitor work? ###
Using libevent, the monitor reads in data from multiple sources,
and then performs operations on this data.
These sources are added by connecting to the monitor over telnet
or using something like ```tut```. This "controller" connection
is used to initiate and end monitoring connections. For example, 
to start a monitoring connection to something that serves xl3 data,
the controller might send:
```start XL3 128.60.13.52 8081```.
Likewise, that connection can be closed with
```stop XL3 128.60.13.52 8081```.
A list of current monitoring connections is shown
with the command ```print_cons```.

But how does a connection actually get monitored?
when the controller tells the monitor to start a connection,
the monitor tries to connect() to the specified host and port.
When the connection is made, it gets added to a list of connections
to be "kept track of" by the main libevent event loop. For each connection,
libevent creates a buffer (an ```evbuffer```, actually) to hold all of the
incoming data on that connection. Each type of connection has a "packet size"
associated with it, and libevent keeps adding data to a connection's evbuffer until
there are "packet size" bytes in the buffer. At this point, libevent initiates a 
callback function specified by the program.

There are different callbacks for each type. For example, the XL3 callback
takes the data in the xl3 connection's evbuffer and unpacks some specific
values from the data. Then, those values get pushed to a holding area (a
globally defined ring buffer). At the same time that libevent is taking
in data from all of the monitoring connections, there is an xl3 watcher function
getting called every .05 seconds. This function checks to see if there is any
xl3 data in that global ring buffer. If there is, the monitor spawns a new thread
that averages all of the similar values in the ring buffer and then uploads
the result to a CouchDB database using [pouch](https://github.com/pennsnoplus/pouch),
a low-level C library based off of libcurl, which does the actual uploading.

Right now, the program is pretty messy. In the future, instead of spawning new
threads to do the uploads to the CouchDB, monitor.c will use libcurl's "multi"
interface to do non-blocking uploads (currently, pouch uses libcurl's blocking "easy"
interface, which is why new threads are spawned to perform the upload). Other than
the messiness of the uploading functionality, the monitor is well designed.
Please don't judge the quality of this program from the code in this repository right now
- it's gotten very messy in the past couple of days because of all the last minute
testing that's happened so that everything would be ready for this conference. In theory,
the program design is extremely elegant: to monitor a new type of data (maybe something
from TUBII, or the CAEN boards, or the Event Builder), is to:

1. define a data type (a ```struct```) for that type of connection,
2. create a call back function for operating on incoming data, and
3. create a function for uploading the operated-upon data.

Then, all the data is sent to a CouchDB instance, where it can be accessed from
anywhere.

### How do the pretty graphs work? ###
The graphs are drawn using the [flot](http://code.google.com/p/flot/) javascript
graphing library, connected to CouchDB in a CouchApp. Anytime the database
changes, the graphs are redrawn, all in real-time. 

# Run the Demo #

## Dependencies ##

+ [libCURL](http://curl.haxx.se/libcurl/)

+ [libevent](http://monkey.org/~provos/libevent/)

## How to ##

The way the demo program works is not at all simple.
Fake XL3 packets are generated and sent to a CouchDB
server hosted by Cloudant - view the source code to
see exactly where.

After the packets are uploaded, pretty graphs are created
in Javascript using the Flot graphing library. [Here is that page.](http://snoplus.cloudant.com/pmt_test/_design/grapher/index.html)

Data flow is as follows:

```xl3_data_generator/porca``` -------> ```penn_daq``` -------> ```monitor``` -------> CouchDB instance

To set up and run a demo, you must also have a copy of (Penn_daq)[https://github.com/pennsnoplus/Penn_daq].

- To compile penn_daq:
 - ```cd path/to/penn/daq/directory```
 - ```make clean; make```
- To compile monitor and porca:
 - ```cd path/to/snot_mon/```
 - ```make clean; make```
 - ```cd xl3_data_generator```
 - ```./make.sh```

To start the demo, start penn_daq. Run the monitor. Using either telnet or tut (which is included
in the Penn_daq directory), connect to the monitor (```(telnet|tut) localhost 2020```). Make sure
that the monitor accepts this connection; try typing some input and pressing return, the monitor
should print out what you typed. 

Then, from the telnet/tut connection, send ```start xl3 localhost 44598```; the monitor should say that it has started
a connection, and penn_daq should print out that it has a monitor connected. Now, run porca. This will start generating
XL3 data, which gets passed to penn_daq, from there to monitor, and from there to the CouchDB instance. Leave all of
this running and check out the graph page - everything should be updating.
If you're having trouble with any of these steps, feel free to email peter.l.downs@gmail.com. 

# CouchApp Resources #
+ [Anatomy of a CouchApp](http://mindeavor.com/blog/the-anatomy-of-a-couchapp)
+ [Formatting with Show and List](http://wiki.apache.org/couchdb/Formatting_with_Show_and_List)
+ [Evently and CouchApp](http://couchapp.couchone.com/docs/_design/docs/index.html#/topic/evently)
+ [jQuery Mobile + Templates + CouchApp](http://custardbelly.com/blog/2010/12/28/jquery-mobile-couchdb-part-3-templates-and-mustache-js/)
+ [Lists, Shows, and Models (slideshow)](http://xerexen.com/posts/lists-shows-and-models)
+ [Evently DIY pt. II - State](http://couchapp.org/page/evently-do-it-yourself-ii-state)
+ [Evently Primer](http://couchapp.org/page/evently-primer)
+ [Notes On Pages Files](http://couchapp.org/page/NotesOnPagesFiles)
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
