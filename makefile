ep2: ep2.o memory.o
	gcc -o ep2 ep2.o memory.o -pthread -lreadline -g

ep2.o: source/ep2.c
	gcc -c source/ep2.c -Wall -Wextra -pedantic -g

memory.o: source/memory.c
	gcc -c source/memory.c -Wall -Wextra -pedantic -g

clean:
	rm -rf source/obj/*.o
	rm -rf source/*~
	rm -rf a.out
	rm ep2
	rm /tmp/ep2.mem
	rm /tmp/ep2.vir

all: ep2
	mv ./*.o source/obj/
