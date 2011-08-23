monitor: clean fakedata
	gcc -o monitor lib/ringbuf/ringbuf.c monitor.c lib/pouch/pouch.c lib/json/json.c -lcurl -levent -lpthread -L/usr/local/lib -g
clean:
	-$(RM) monitor fakedata snotstream
fakedata:
	gcc -o fakedata fakedata.c -levent -L/usr/local/lib -g
snotstream: clean
	gcc -o snotstream snotstream.c -lcurl -levent -L/usr/local/lib -g
