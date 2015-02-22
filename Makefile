CC=g++ -std=c++11
CFLAGS=-Wall -c
DEBUG=-g

.PHONY: all build restore config setup-files clean-all clean-files

# Call the build routine
all: build

# Start fresh
build: clean-files tree.out

# Load the first set of points inserted
restore: setup-files tree.out

# Build the tree
tree.out: config rtree.o
	$(CC) $(DEBUG) rtree.o -o tree.out

rtree.o: rtree.cpp
	$(CC) $(CFLAGS) $(DEBUG) config.h rtree.cpp

# Configure the parameters
config: configure
	./configure

setup-files: clean-files
	# rm -f .tree.session
	# rm -f leaves/* objects/*
	# tar xvf data.tar

clean-all: clean-files
	rm *.o *.out

clean-files:
	rm -f .tree.session
	rm -f leaves/* objects/*
	touch leaves/DUMMY objects/DUMMY
