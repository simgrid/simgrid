CXX=clang++
CXXFLAGS=-Wall -Wextra -g -std=c++11
LDFLAGS=-lsimgrid

all: replay_mnd_usual

replay_mnd_hack: replay_multiple_hack.cpp
	$(CXX) -o $@ $< $(LDFLAGS) $(CXXFLAGS)

replay_mnd_usual: replay_multiple_usual.cpp
	$(CXX) -o $@ $< $(LDFLAGS) $(CXXFLAGS)

clean:
	rm -f replay_mnd
