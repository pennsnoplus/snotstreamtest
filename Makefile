snotstream: clean
	gcc -Wall -o snotstream lib/node/node.c lib/ringbuf/ringbuf.c snotstream.c lib/pouch/pouch.c lib/pouch/multi_pouch.c lib/json/json.c -lcurl -levent -L/usr/local/lib -g
clean:
	-$(RM) snotstream
