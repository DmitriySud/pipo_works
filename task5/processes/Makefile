include ../../Makefile.common

all: build run

build:
	$(CXX) factorial.cpp $(SOURCES)/../../common/big_integer.cpp $(CXXFLAGS) -o factorial -I$(SOURCES)/../..

run: 
	./factorial 4
