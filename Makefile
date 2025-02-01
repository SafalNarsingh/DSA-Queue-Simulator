all:
	gcc -o simulator simulator.c -I./src/include -L./src/lib -lmingw32 -lSDL2main -lSDL2