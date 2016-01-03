CC=g++
CXXFLAGS=-Wall -O3 -std=c++0x -Wextra -pedantic

all: wgs

wgs: dice.o scramble.o wgs_json.o solver.o analyze.o maker.o validate.o wgs.h
	$(CC) $(CXXFLAGS) dice.o scramble.o wgs_json.o solver.o analyze.o maker.o validate.o -o wgs -ljansson

analyze.o: analyze.cpp

dice.o: dice.cpp

scramble.o: scramble.cpp

wgs_json.o: wgs_json.cpp wgs.h

solver.o: solver.cpp wgs.h

maker.o: maker.cpp

validate.o: validate.cpp

clean:
	rm *.o wgs
