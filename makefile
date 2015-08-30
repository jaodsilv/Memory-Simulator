ep1sh: ep1sh.o ep1.o sjf.o fcfs.o rr.o srtn.o ps.o
	gcc -o ep1sh ep1sh.o ep1.o sjf.o fcfs.o rr.o srtn.o ps.o -lreadline -pthread -g

ep1sh.o: ep1sh.c
	gcc -c ep1sh.c -Wall -ansi

ep1.o: ep1.c
	gcc -c ep1.c -Wall -ansi

srtn.o: srtn.c
	gcc -c srtn.c -Wall -ansi

rr.o: rr.c
	gcc -c rr.c -Wall -ansi

ps.o: ps.c
	gcc -c ps.c -Wall -ansi

fcfs.o: fcfs.c
	gcc -c fcfs.c -Wall -ansi

sjf.o: sjf.c
	gcc -c sjf.c -Wall -ansi

clean:
	rm -rf *.o
	rm -rf *~
	rm -rf outputs/*.txt
	rm -rf a.out
	rm ep1sh
