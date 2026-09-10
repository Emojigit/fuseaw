.PHONY: all clean

CXX := g++
CXXFLAGS := -std=c++23 -O2 -Wall
FUSE_FLAGS := $(shell pkg-config fuse3 --cflags --libs)

SOURCES := fuseaw.cpp $(wildcard src/*.cpp)
OBJECTS := $(SOURCES:.cpp=.o)

all: fuseaw

fuseaw: $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $@ $(FUSE_FLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) fuseaw
