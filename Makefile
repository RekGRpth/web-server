CFLAGS+=-Wall -Wextra -Werror -Wno-unused-parameter
ifeq (true,$(DEBUG))
CFLAGS+=-O0 -g
else
CFLAGS+=-O3 -s
endif

ifeq (true,$(RAGEL))
CPPFLAGS+=-DRAGEL=true
server: server.o ragel-http-parser/ragel_http_parser.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $+ -luv -o $@
else
CPPFLAGS+=-DRAGEL=false

ifeq (true,$(DEBUG))
server: server.o http-parser/http_parser_g.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $+ -luv -o $@

http-parser/http_parser_g.o: Makefile
	$(MAKE) -C http-parser http_parser_g.o
else
server: server.o http-parser/http_parser.o
	$(CC) $(CPPFLAGS) $(CFLAGS) $+ -luv -o $@

http-parser/http_parser.o: Makefile
	$(MAKE) -C http-parser http_parser.o
endif

endif

%.o: %.c %.h Makefile
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

ragel-http-parser/ragel_http_parser.o: Makefile
	$(MAKE) -C ragel-http-parser ragel_http_parser.o

clean:
	$(MAKE) -C http-parser clean
	$(MAKE) -C ragel-http-parser clean
	rm -f *.o server
