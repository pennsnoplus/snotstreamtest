echo: clean fakedata
	gcc -o echo echo.c pouch.c json.c -lcurl -levent -L/usr/local/lib -g
clean:
	-$(RM) echo fakedata
fakedata:
	gcc -o fakedata fakedata.c -levent -L/usr/local/lib -g
