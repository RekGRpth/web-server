#CFLAGS+=-Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-function
#CFLAGS+=-Wall -Wextra -Werror -Wpedantic
CFLAGS+=-Wall -Wextra -Werror
ifeq (true,$(DEBUG))
    CFLAGS+=-O0 -g
else
    CFLAGS+=-O3 -s
endif

ifeq (false,$(RAGEL))
    CFLAGS+=-DNODEJS_HTTP_PARSER
    ifeq (true,$(DEBUG))
        server: main.o queue.o xbuffer.o server.o client.o request.o nodejs-http-parser/http_parser_g.o parser.o postgres.o response.o
			$(CC) $(CFLAGS) $+ -luv -lpq -o $@
        nodejs-http-parser/http_parser_g.o: Makefile
			$(MAKE) -C nodejs-http-parser http_parser_g.o
    else
        server: main.o queue.o xbuffer.o server.o client.o request.o nodejs-http-parser/http_parser_g.o parser.o postgres.o response.o
			$(CC) $(CFLAGS) $+ -luv -lpq -o $@
        nodejs-http-parser/http_parser.o: Makefile
			$(MAKE) -C nodejs-http-parser http_parser.o
    endif
else
    CFLAGS+=-DRAGEL_HTTP_PARSER
    server: main.o queue.o xbuffer.o server.o client.o request.o ragel-http-parser/http_parser.o parser.o postgres.o response.o
		$(CC) $(CFLAGS) $+ -luv -lpq -o $@
    ragel-http-parser/http_parser.o: Makefile
		$(MAKE) -C ragel-http-parser http_parser.o
endif

%.o: %.c %.h Makefile
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(MAKE) -C nodejs-http-parser clean
	$(MAKE) -C ragel-http-parser clean
	rm -f *.o server
