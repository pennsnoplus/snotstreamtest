monitor:
	gcc -o mon mon_async.c -levent -L/usr/local/lib

fakedata:
	gcc -o fakedata fakedata.c -levent -L/usr/local/lib
