CC = gcc

CFLAGS = -Wall -g

TARGET = it87_dump

SRCS = it87_dump.c

OBJS = $(SRCS:.c=.o)

all: $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

run: $(TARGET)
	sudo ./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean run
