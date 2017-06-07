CFLAGS+=-Wall -Wextra -Werror -Wno-unused-parameter
ifeq (true,$(DEBUG))
CFLAGS+=-O0 -g
RFLAGS+=-C -T0
else
CFLAGS+=-O3 -s
#RFLAGS+=-C -G2
RFLAGS+=-C -T0
endif

http_parser.o: http_parser.c http_parser.h Makefile
#	$(CC) $(CFLAGS) -E $< -o $@.c
	$(CC) $(CFLAGS) -c $< -o $@

http_parser.c: http_parser.rl.c Makefile
	ragel ${RFLAGS} $< -o $@

clean:
	rm -f *.o http_parser.c
