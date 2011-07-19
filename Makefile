echo: clean
	gcc -o echo echo.c -levent -L/usr/local/lib -g
clean:
	-$(RM) echo
