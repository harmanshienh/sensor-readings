CC = gcc
CFLAGS = -Wall
LDFLAGS = -lwiringPi
TARGET = uart_receiver
SRC = main.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

clean:
	rm -f $(TARGET)