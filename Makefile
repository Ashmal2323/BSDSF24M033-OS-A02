CC     = gcc
CFLAGS = -Wall -Wextra -g

TARGET = bin/ls
SRC    = src/ls-v1.0.0.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET) obj/*.o

.PHONY: all clean
