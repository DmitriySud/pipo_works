include ../Makefile.common

all: build run

build:
	$(CXX) useless.cpp $(CXXFLAGS) -o useless -I$(SOURCES)/..

run: 
	./useless records.txt
