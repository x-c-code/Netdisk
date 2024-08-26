SRCS := $(wildcard *.c)
CC := gcc
COM_P := -Wall -g -pthread -lcrypt -lssl -lcrypto -ll8w8jwt

client: $(SRCS) 
	$(CC) $^ -o $@ $(COM_P) 

clean:
	$(RM) client

rebuild: clean client
