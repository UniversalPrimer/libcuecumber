
CC = -pthread 


all: lib bin

lib: lib_comp
	gcc -shared -Wl -lpthread -o libcuecumber.so  cuecumber.o

lib_comp: cuecumber.c cuecumber.h
	gcc $(CC) -c -fPIC cuecumber.c


bin: main.c cuecumber.c cuecumber.h
	gcc $(CC) -o cuecumber main.c cuecumber.c


clean:
	rm -rf cuecumber libcuecumber.so *.o
