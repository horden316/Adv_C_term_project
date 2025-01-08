CC = gcc
CFLAGS = -Wall -g
OBJS = main.o filesystem.o command.o
TARGET = filesystem

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

main.o: main.c main.h filesystem.h
	$(CC) $(CFLAGS) -c main.c

filesystem.o: filesystem.c filesystem.h
	$(CC) $(CFLAGS) -c filesystem.c

command.o: command.c command.h
	$(CC) $(CFLAGS) -c command.c

clean:
	rm -f $(OBJS) $(TARGET) filesystem.img
