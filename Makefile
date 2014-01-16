CXX=g++
CXXFLAGS=-W -Wall -Wextra -Werror -pedantic -std=c++11 -ggdb
LDFLAGS=-lPocoNetSSL -lPocoNet -lPocoFoundation -ljsoncpp

reningsverk: main.o terminalui.o reningsverk.o issue.o initiative.o suggestion.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@

%.o: %.c++ *.h
	$(CXX) $(CXXFLAGS) $< -c -o $@
