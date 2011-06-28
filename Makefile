monitor: clean fakedata
	gcc -o monitor monitor.c -levent -L/usr/local/lib

fakedata:
	gcc -o fakedata fakedata.c -levent -L/usr/local/lib

clean:
	-$(RM) monitor fakedata
