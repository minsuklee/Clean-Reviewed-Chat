// 헤더파일 인클루드
#include "chatting.h"

// 메인함수
int main(int argc, char *argv[])
{
	// 변수선언
	int server_socket;
	struct sockaddr_in server_address;
	pthread_t send_thread_ID, receive_thread_ID;
	void * thread_return_value;

	// 입력인자 검증
	if(argc!=4) {
		printf("Usage : %s <IP> <port> <user_name>\n", argv[0]);
		exit(1);
	 }
	
	// user_name 초기화
	sprintf(user_name, "[%s]", argv[3]);

	// 소켓생성
	server_socket=socket(PF_INET, SOCK_STREAM, 0);

	// 주소정보 초기화
	memset(&server_address, 0, sizeof(server_address));
	server_address.sin_family=AF_INET;
	server_address.sin_addr.s_addr=inet_addr(argv[1]);
	server_address.sin_port=htons(atoi(argv[2]));
	  
	// 서버에 connect 요청
	if(connect(server_socket, (struct sockaddr*)&server_address, sizeof(server_address))==-1)
		error_handling("connect() error");
	
	print_connected_message();
	// 쓰레드 생성
	pthread_create(&send_thread_ID, NULL, send_msg_thread_main, (void*)&server_socket);
	pthread_create(&receive_thread_ID, NULL, receive_msg_thread_main, (void*)&server_socket);

	// 쓰레드 종료 대기
	pthread_join(send_thread_ID, &thread_return_value);
	pthread_join(receive_thread_ID, &thread_return_value);

	// 소켓 닫기
	close(server_socket);  
	return 0;
}