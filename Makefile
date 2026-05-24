CC = clang
CFLAGS = -Wall -Wextra -g -fsanitize=address -pthread

TARGET = server
SRC = src/main.c \
      src/server.c \
      src/request.c \
      src/response.c \
      src/files.c \
      src/mime.c \
      src/handlers.c \
	src/logger.c \
      src/threadpool.c

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)