CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -O2
SRC = src/main.c src/parser.c src/resolver.c src/builder.c src/error.c src/sdk.c src/pipeline.c
TARGET = bundle

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
