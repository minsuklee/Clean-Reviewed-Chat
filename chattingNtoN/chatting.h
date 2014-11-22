// 헤더파일 인클루드
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/epoll.h>

// 매크로 선언
#define BUFFER_SIZE 1024
#define USER_NAME_SIZE 20
#define EPOLL_SIZE 50

// 함수원형 선언
void * send_msg_thread_main(void * arg);
void * receive_msg_thread_main(void * arg);
void error_handling(char * msg);
void setnonblockingmode(int fd);
void print_connected_message();
void print_server_is_ready();
void print_newConnection();

// 전역변수 선언
char user_name[USER_NAME_SIZE]="[DEFAULT]";

// server, client 공통적으로 사용되는 thread
// 메시지 발송 처리
void * send_msg_thread_main(void * arg)
{
	int socket=*((int*)arg);
	char messageToSend[BUFFER_SIZE];
	char messageWithName[USER_NAME_SIZE + BUFFER_SIZE];
	while(1) 
	{
		fgets(messageToSend, BUFFER_SIZE, stdin);
		if(!strcmp(messageToSend,"q\n")||!strcmp(messageToSend,"Q\n")) 
		{
			close(socket);
			exit(0);
		}
		sprintf(messageWithName,"%s %s", user_name, messageToSend);
		write(socket, messageWithName, strlen(messageWithName));
	}
	return NULL;
}

// server, client 공통적으로 사용되는 thread
// 메시지 수신 처리
void * receive_msg_thread_main(void * arg)
{
	int socket=*((int*)arg);
	char received_message[USER_NAME_SIZE + BUFFER_SIZE];
	int received_message_length;
	while(1)
	{
		received_message_length=read(socket, received_message, USER_NAME_SIZE+BUFFER_SIZE-1);
		if(received_message_length==-1) 
			return (void*)-1;
		received_message[received_message_length]=0;
		fputs(received_message, stdout);
	}
	return NULL;
}

// 표준 에러 스트림에 오류 출력
void error_handling(char *msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}

// Non Block mode 설정
void setnonblockingmode(int fd)
{
	int flag=fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flag|O_NONBLOCK);
}

// 사용자 안내 문구
void print_server_is_ready()
{
	fputs("\n=============== welcome ===============\n", stdout);
	fputs("   Waiting for the client connection\n", stdout);
	fputs("=======================================\n", stdout);
	fflush(stdout);
}

// 사용자 안내 문구
void print_newConnection()
{
	fputs("\n=========== new connection ============\n", stdout);
	fputs("   Now You have someone to talk to\n", stdout);
	fputs("=======================================\n", stdout);
	fflush(stdout);
}

// 사용자 안내 문구
void print_connected_message()
{
	fputs("\n=========== connected ============\n", stdout);
	fputs("       connecting success\n", stdout);
	fputs("==================================\n", stdout);
}