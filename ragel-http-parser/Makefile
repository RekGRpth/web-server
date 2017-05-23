CFLAGS+=-Wall -Wextra -Werror -Wno-unused-parameter
ifeq (true,$(DEBUG))
CFLAGS+=-O0 -g
RFLAGS+=-C -T0
else
CFLAGS+=-O3 -s
RFLAGS+=-C -G2
endif

ragel_http_parser.o: ragel_http_parser.c ragel_http_parser.h Makefile
	$(CC) $(CFLAGS) -c $< -o $@

ragel_http_parser.c: ragel_http_parser.rl.c Makefile
	ragel ${RFLAGS} $< -o $@

clean:
	rm -f *.o ragel_http_parser.c
