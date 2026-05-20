CC     = gcc
CFLAGS = -Wall -Wextra -Iinclude -O2 -Wno-restrict
LIBS   = -lcurl
SRC    = src/main.c src/parser.c src/resolver.c src/builder.c \
         src/error.c src/sdk.c src/pipeline.c src/deps.c
TARGET = bundle

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LIBS)

clean:
	rm -f $(TARGET)
