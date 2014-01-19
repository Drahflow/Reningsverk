# tested with g++-4.7 and g++-4.8
CXX=g++
CXXFLAGS=-W -Wall -Wextra -Werror -pedantic -std=c++11 -ggdb
LDFLAGS=-lPocoNetSSL -lPocoNet -lPocoFoundation -ljsoncpp

reningsverk: main.o terminalui.o reningsverk.o issue.o initiative.o \
	     suggestion.o localstore.o tempfile.o area.o draft.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@

%.o: %.c++ *.h
	$(CXX) $(CXXFLAGS) $< -c -o $@
