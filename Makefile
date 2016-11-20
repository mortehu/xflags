CXXFLAGS = -std=c++11 -g

all: test

test.o: test.cc cflags.h

test: cflags_before.o test.o cflags.o cflags_after.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) cflags_before.o test.o cflags.o cflags_after.o $(OUTPUT_OPTION)
