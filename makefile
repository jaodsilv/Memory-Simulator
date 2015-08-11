ep1sh: ep1sh.o
	gcc -o ep1sh ep1sh.o

ep1sh.o: ep1sh.c
	gcc -c ep1sh.c -Wall -ansi -O2

clean:
	rm -rf *.o
	rm -rf *~
	rm ep1sh
