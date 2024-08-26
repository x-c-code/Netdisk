SRCS := $(wildcard *.c)
SRCS_CRYPT := $(wildcard ./bcrypt/*.c)
CC := gcc
COM_P := -Wall -g -pthread -lcrypt -lssl -lcrypto -lmysqlclient -ll8w8jwt

netdisk_server: $(SRCS) $(SRCS_CRYPT)
	$(CC) $^ -o $@ $(COM_P) 

clean:
	$(RM) netdisk_server

rebuild: clean netdisk_server
