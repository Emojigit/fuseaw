.PHONY: fuseaw

fuseaw:
	g++ -std=c++23 fuseaw.cpp src/*.cpp -o fuseaw $$(pkg-config fuse3 --cflags --libs)
