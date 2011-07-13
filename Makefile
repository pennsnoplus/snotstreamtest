monitor: clean fakedata
	gcc -o monitor monitor.c -levent -L/usr/local/lib -g

fakedata:
	gcc -o fakedata fakedata.c -levent -L/usr/local/lib -g

clean:
	-$(RM) monitor fakedata
