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

connection.o: connection.cpp connection.h packet.h
	$(CC) $(CFLAGS) -c connection.cpp

bridge.o: bridge.h bridge.cpp connection.o
	$(CC) $(CFLAGS) -c bridge.cpp

station.o: station.h station.cpp connection.o
	$(CC) $(CFLAGS) -c station.cpp
	
clean:
	rm -f *.o bridge station .cs*

btstation: station
	station -no ifaces/ifaces.b rtables/rtable.b hosts
	
atstation: station
	station -no ifaces/ifaces.a rtables/rtable.a hosts

ctstation: station
	station -no ifaces/ifaces.c rtables/rtable.c hosts
dtstation: station
	station -no ifaces/ifaces.d rtables/rtable.d hosts
etstation: station 	
	station -no ifaces/ifaces.e rtables/rtable.e hosts
	
rtastation: station
	station -route ifaces/ifaces.r2 rtables/rtable.r2 hosts
rtbstation: station
	station -route ifaces/ifaces.r1 rtables/rtable.r1 hosts
rtcstation: station
	station -route ifaces/ifaces.r3 rtables/rtable.r3 hosts
	

#%.o : %.cpp $(INCLUDES)
#$(CC) $(CFLAGS) $<