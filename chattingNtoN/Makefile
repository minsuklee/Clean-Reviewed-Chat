# makefile for chatting program

cc = gcc
CFLAGS = -W -Wall

all: chatting_server chatting_client

chatting_server :  chatting_server.c -lpthread
chatting_client :  chatting_client.c -lpthread

clean: 
	rm chatting_server chatting_client