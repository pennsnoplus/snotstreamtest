echo: clean fakedata
	gcc -o echo echo.c -levent -L/usr/local/lib -g
clean:
	-$(RM) echo fakedata
fakedata:
	gcc -o fakedata fakedata.c -levent -L/usr/local/lib -g
