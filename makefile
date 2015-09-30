ep2: ep2.o
	gcc -o ep2 ep2.o -pthread -lreadline

ep2.o: source/ep2.c
	gcc -c source/ep2.c -Wall -Wextra -pedantic -ansi

clean:
	rm -rf source/obj/*.o
	rm -rf source/*~
	rm -rf outputs/*.txt
	rm -rf a.out
	rm ep2

all: ep2
	mv ./*.o source/obj/
