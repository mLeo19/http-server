CC = clang
CFLAGS = -Wall -Wextra -g -fsanitize=address

TARGET = server
SRC = src/main.c src/request.c src/files.c src/mime.c

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)