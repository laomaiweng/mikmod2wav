
CFLAGS	= -std=c11 -Wall -Wextra -O3 -flto -s $(shell libmikmod-config --cflags)
LDFLAGS	= -flto
LIBS	= $(shell libmikmod-config --libs)

mikmod2wav: mikmod2wav.c
	gcc $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS)
