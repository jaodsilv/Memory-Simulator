ep1sh: ep1sh.o
	gcc -o ep1sh ep1sh.o -lreadline -g

ep1: ep1.o sjf.o fcfs.o rr.o srtn.o ps.o edf.o
	gcc -o ep1 ep1.o sjf.o fcfs.o rr.o srtn.o ps.o edf.o -lm -pthread -g

ep1sh.o: source/ep1sh.c
	gcc -c source/ep1sh.c -Wall -ansi

ep1.o: source/ep1.c
	gcc -c source/ep1.c -Wall -ansi

srtn.o: source/srtn.c
	gcc -c source/srtn.c -Wall -ansi

rr.o: source/rr.c
	gcc -c source/rr.c -Wall -ansi

ps.o: source/ps.c
	gcc -c source/ps.c -Wall -ansi

fcfs.o: source/fcfs.c
	gcc -c source/fcfs.c -Wall -ansi

edf.o: source/edf.c
	gcc -c source/edf.c -Wall -ansi

sjf.o: source/sjf.c
	gcc -c source/sjf.c -Wall -ansi

clean:
	rm -rf source/obj/*.o
	rm -rf source/*~
	rm -rf outputs/*.txt
	rm -rf a.out
	rm ep1sh
	rm ep1

all: ep1sh ep1
		 mv source/../*.o source/obj/
