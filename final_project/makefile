CC = gcc
CFLAGS = -Wall -g
OBJS = main.o filesystem.o
TARGET = filesystem

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

main.o: main.c main.h filesystem.h
	$(CC) $(CFLAGS) -c main.c

filesystem.o: filesystem.c filesystem.h
	$(CC) $(CFLAGS) -c filesystem.c

clean:
	rm -f $(OBJS) $(TARGET) filesystem.img
