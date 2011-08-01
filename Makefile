monitor: clean fakedata
	gcc -o monitor ringbuf.c monitor.c lib/pouch/pouch.c lib/json/json.c -lcurl -levent -lpthread -L/usr/local/lib -g
clean:
	-$(RM) monitor fakedata
fakedata:
	gcc -o fakedata fakedata.c -levent -L/usr/local/lib -g
