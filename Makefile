CFLAGS+= -Werror -Wall


viewer: viewer.c
	gcc viewer.c -lvlc -lSDL2 -o viewer