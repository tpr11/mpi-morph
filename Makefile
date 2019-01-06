main:
	mpicc src/image.c main.c -Iinclude -lm -Wall -O3 -o morph

main_debug:
	mpicc src/image.c main.c -Iinclude -lm -Wall -O0 -ggdb -o morph 

clean:
	rm -f morph