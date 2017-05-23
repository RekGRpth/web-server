CFLAGS+=-Wall -Wextra -Werror -Wno-unused-parameter
ifeq (true,$(DEBUG))
CFLAGS+=-O0 -g
else
CFLAGS+=-O3 -s
endif

ragel_http_parser.o: ragel_http_parser.c ragel_http_parser.h Makefile
	$(CC) $(CFLAGS) -c $< -o $@

ragel_http_parser.c: ragel_http_parser.rl.c Makefile
#	ragel -G2 $< -o $@
	ragel -C -T0 $< -o $@

clean:
	rm -f *.o ragel_http_parser.c
