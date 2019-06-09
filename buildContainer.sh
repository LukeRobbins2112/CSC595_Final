# !/bin/bash

gcc -ggdb -o pivot_root.o pivot_root.c -c -larchive
gcc -ggdb container.c pivot_root.o img.o -o container -larchive
