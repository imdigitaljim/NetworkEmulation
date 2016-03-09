#Course: CNT5505 - Data Networks and Communications
#Semester: Spring 2016
#Name: James Bach, Becky Powell
#
# Compile using on linprog
#


CC=g++
CFLAGS=-pedantic -Wall -std=c++11

all: station bridge
	
bridge: bridgedriver.o bridge.o
	$(CC) $(CFLAGS) bridgedriver.o bridge.o connection.o -o bridge

station: stationdriver.o station.o
	$(CC) $(CFLAGS) stationdriver.o station.o connection.o -o station
	
stationdriver.o: stationdriver.cpp 
	$(CC) $(CFLAGS) -c stationdriver.cpp
	
bridgedriver.o: bridgedriver.cpp
	$(CC) $(CFLAGS) -c bridgedriver.cpp

connection.o: connection.cpp connection.h
	$(CC) $(CFLAGS) -c connection.cpp

bridge.o: bridge.h bridge.cpp connection.o
	$(CC) $(CFLAGS) -c bridge.cpp

station.o: station.h station.cpp connection.o
	$(CC) $(CFLAGS) -c station.cpp
	
clean:
	rm -f *.o bridge station .cs*

tstation: station
	station -route ifaces/ifaces.b rtables/rtable.b hosts

#%.o : %.cpp $(INCLUDES)
#$(CC) $(CFLAGS) $<