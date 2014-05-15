#!/bin/sh
g++  -Wall -g `sdl-config --cflags` -pedantic  -std=c++98 -c test.cpp -o test.o -I../include 
g++ -o /usr/src/freecnc/test.aip -g test.o -fPIC -shared   -lz