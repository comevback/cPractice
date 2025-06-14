CC = gcc
CFLAGS = -Wall -O2 -lpthread
SRC = pfind.c threadpool.c
OUT = pfind

$(OUT): $(SRC)
	$(CC) $(SRC) -o $(OUT) $(CFLAGS)

clean:
	rm -f $(OUT)