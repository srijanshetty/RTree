CC=g++ -std=c++11
CFLAGS=-Wall -c
DEBUG=-g

.PHONY: all build restore setup-files clean-all clean-files

# Call the build routine
all: tree.out

# Start fresh
fresh: clean-files tree.out

# Load the first set of points inserted
restore: setup-files tree.out

# Build the tree
tree.out: rtree.o
	$(CC) $(DEBUG) rtree.o -o tree.out

rtree.o: rtree.cpp config.h
	$(CC) $(CFLAGS) $(DEBUG) config.h rtree.cpp

# rtree.cpp: rtree.config configure
	# ./configure

setup-files: clean-files
	rm -f .tree.session
	rm -f leaves/* objects/*
	tar xvzf data.tar.gz

clean-all: clean-files
	rm *.o *.out *.gch

clean-files:
	rm -f .tree.session
	rm -f leaves/* objects/*
	touch leaves/DUMMY objects/DUMMY
