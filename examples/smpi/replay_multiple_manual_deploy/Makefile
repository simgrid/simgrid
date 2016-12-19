CXX=clang++
CXXFLAGS=-Wall -Wextra -g -std=c++11
LDFLAGS=-lsimgrid

replay_mnd: replay_multiple.cpp
	$(CXX) -o $@ $< $(LDFLAGS) $(CXXFLAGS)

clean:
	rm -f replay_mnd
