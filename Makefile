#SRC_EXT=c
CC= gcc -Wall
#HDR_EXT=h
SRC=main.c mes_semaphores.c
#SRC=$(shell ls | grep -E '*\.$(SRC_EXT)')
#HDR=$(shell ls | grep -E '*\.$(HDR_EXT)')
HDR=mes_semaphores.h
BIN=main

$(BIN): $(SRC) $(HDR) 
	$(CC) -Wextra -std=gnu99 $(SRC) -lm -o $(BIN)

clean:
	rm -f $(BIN) *.o
