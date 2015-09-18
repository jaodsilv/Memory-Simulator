ep2: ep2.o
	g++ -o ep2 ep2.o -lm -pthread -g --pedantic -std=gnu++11

ep1.o: source/ep1.cpp
	g++ -c source/ep1.cpp -Wall --pedantic -std=gnu++11

clean:
	rm -rf source/obj/*.o
	rm -rf source/*~
	rm -rf outputs/*.txt
	rm -rf a.out
	rm ep2

all: ep2
	mv ./*.o source/obj/
