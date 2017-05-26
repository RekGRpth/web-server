CFLAGS+=-Wall -Wextra -Werror -Wno-unused-parameter
ifeq (true,$(DEBUG))
    CFLAGS+=-O0 -g
else
    CFLAGS+=-O3 -s
endif

ifeq (true,$(RAGEL))
    CFLAGS+=-DRAGEL_HTTP_PARSER
    server: main.o request.o postgres.o parser.o response.o ragel-http-parser/http_parser.o
		$(CC) $(CFLAGS) $+ -luv -lpq -o $@

    ragel-http-parser/http_parser.o: Makefile
		$(MAKE) -C ragel-http-parser http_parser.o
else
    CFLAGS+=-DNODEJS_HTTP_PARSER
    ifeq (true,$(DEBUG))
        server: main.o request.o postgres.o parser.o response.o nodejs-http-parser/http_parser_g.o
			$(CC) $(CFLAGS) $+ -luv -lpq -o $@

        nodejs-http-parser/http_parser_g.o: Makefile
			$(MAKE) -C nodejs-http-parser http_parser_g.o
    else
        server: main.o request.o postgres.o parser.o response.o nodejs-http-parser/http_parser.o
			$(CC) $(CFLAGS) $+ -luv -lpq -o $@

        nodejs-http-parser/http_parser.o: Makefile
			$(MAKE) -C nodejs-http-parser http_parser.o
    endif
endif

%.o: %.c %.h Makefile
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(MAKE) -C nodejs-http-parser clean
	$(MAKE) -C ragel-http-parser clean
	rm -f *.o server
