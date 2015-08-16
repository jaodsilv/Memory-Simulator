ep1sh: ep1sh.o ep1.o
	gcc -o ep1sh ep1sh.o ep1.o -lreadline

ep1sh.o: ep1sh.c
	gcc -c ep1sh.c -Wall -ansi

ep1.o: ep1.c
	gcc -c ep1.c -Wall -ansi

clean:
	rm -rf *.o
	rm -rf *~
	rm -rf a.out
	rm ep1sh
