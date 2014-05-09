CC = gcc
all: ray-sphere

ray-sphere: ray-sphere.c geo.c geo.h
	${CC}  `sdl2-config --cflags` -o ray-sphere ray-sphere.c geo.c  -lm `sdl2-config --libs` -Wall -O3

clean:
	rm ray-sphere
