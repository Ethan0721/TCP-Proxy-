cproxy sproxy: cproxy.c sproxy.c
	gcc -o cproxy cproxy.c
	gcc -o sproxy sproxy.c
clean:
	rm -f cproxy sproxy *.gcov *gcno

