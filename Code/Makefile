# TARGET = name of executable you want to create 
TARGET=pastrydht
# If you are using gcc to compile then make CC=gcc
CC=g++
# list of headerfiles space seperated
HEADERS=md5.h RoutingManager.h Pastry.h
#flags 
FLAGS=-Wall -pthread 
# you can add flags like this: FLAGS += -std=c++0x

#list of object files that you want to create[space seperated].
OBJS=md5.o RoutingManager.o Pastry.o

$(TARGET) : $(OBJS) $(HEADERS)
	$(CC) $(FLAGS) -o $@ $^
%.o : %.c 
	$(CC) $(FLAGS) -c -o $@ $^

# clean the build, removes all .o files and executable named in TARGET. To execute run make clean from command line
clean : 
	rm *.o $(TARGET)
