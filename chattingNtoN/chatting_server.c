// 헤더파일 인클루드
#include "chatting.h"

// 매크로 선언
#define DEFAULT_ALLOCATED_FD_AMOUNT 6

// 함수원형 선언
void * epoll_thread_main(void * arg);

// 구조체 선언
typedef struct epoll_arg_t {
	int fd;
	struct epoll_event *ep_evts;
	struct epoll_event evt;
	int manager_fd;
} epoll_arg_t;

// 메인함수
int main(int argc, char *argv[])
{
	// 변수선언
	int socket_option_value = 1;
	int server_socket, server_as_client_socket;
	struct sockaddr_in server_address, server_as_client_address;
	
	int epfd;
	struct epoll_event *ep_events;
	struct epoll_event event;
	epoll_arg_t epoll_arg;
	
	pthread_t send_thread_ID, receive_thread_ID, epoll_thread_ID;
	void * thread_return;

	// 입력인자 검증
	if(argc!=3) {
		printf("Usage : %s <port> <user_name>\n", argv[0]);
		exit(1);
	}

	// user_name 초기화
  	sprintf(user_name, "[%s]", argv[2]);

  	// epoll 인스턴스 생성
	epfd = epoll_create ( EPOLL_SIZE );

	// 이벤트 발생 fd가 채워질 구조체의 버퍼생성 및 일어난 이벤트 정보들을 담는 구조체 포인터 
	ep_events = malloc ( sizeof ( struct epoll_event ) * EPOLL_SIZE );

	// 소켓생성  	
	server_socket=socket(PF_INET, SOCK_STREAM, 0);

	// 소켓 옵션 지정
	setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (void *)&socket_option_value, sizeof(socket_option_value));	//

	// 주소정보 초기화
	memset(&server_address, 0, sizeof(server_address));	//
	server_address.sin_family=AF_INET; 					//
	server_address.sin_addr.s_addr=htonl(INADDR_ANY);		//
	server_address.sin_port=htons(atoi(argv[1]));			//

	// 소켓에 IP주소와 포트번호 할당
	if(bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address))==-1)
		error_handling("bind() error\n");

	// 클라이언트의 접속 요청 대기 - 서버 소켓 리스닝
	if(listen(server_socket, 5)==-1)
		error_handling("listen() error");
	
	// 서버소켓 non-block mode 설정
	setnonblockingmode ( server_socket );

	// server_socket에 대해서 수신할 데이터가 존재하게 되는경우의 이벤트를 관찰하고 싶다고 명세를 작성한다.
	event.events = EPOLLIN;
	event.data.fd = server_socket;
	
	// 미리 작성한 명세(event)를 epoll 인스턴스의 파일디스크립터에 등록한다
	epoll_ctl ( epfd, EPOLL_CTL_ADD, server_socket, &event );

	// 쓰레드 생성시 인자로 넘겨줄 구조체를 초기화 한다
	epoll_arg.fd = epfd;
	epoll_arg.ep_evts = ep_events;
	epoll_arg.evt = event;
	epoll_arg.manager_fd = server_socket;

	// epoll 쓰레드 생성
	pthread_create ( &epoll_thread_ID, NULL, epoll_thread_main, ( void * ) &epoll_arg );

	// 서버가 서버 자신에게 접속을 요청
	server_as_client_socket = socket ( PF_INET, SOCK_STREAM, 0 );

	// 주소정보 초기화
	memset ( &server_as_client_address, 0, sizeof ( server_as_client_address ) );
	server_as_client_address.sin_family = AF_INET;
	server_as_client_address.sin_addr.s_addr = htonl(INADDR_ANY);
	server_as_client_address.sin_port = htons ( atoi ( argv[1] ) );
	if ( connect ( server_as_client_socket, ( struct sockaddr* ) &server_as_client_address, sizeof ( server_as_client_address ) ) == -1 )
		error_handling ( "server_as_client_socket() error" );

	// 서버 준비 완료 문구 출력
	print_server_is_ready();

	// 쓰레드 생성
	pthread_create(&send_thread_ID, NULL, send_msg_thread_main, (void*)&server_as_client_socket);
	pthread_create(&receive_thread_ID, NULL, receive_msg_thread_main, (void*)&server_as_client_socket);

	// 쓰레드 종료 대기
	pthread_join(epoll_thread_ID, &thread_return);
	pthread_join(send_thread_ID, &thread_return);
	pthread_join(receive_thread_ID, &thread_return);
	
	// 소켓 닫기
	close(server_as_client_socket);
	close(server_socket);

	return 0;
}
	
void * epoll_thread_main(void * arg)
{
	// 변수선언
	int socket_count = 0;
	int server_socket, client_socket;
	int received_msg_len, i, j;

	char buffer[BUFFER_SIZE];

	struct sockaddr_in client_address;
	socklen_t address_size;

	struct epoll_event *ep_events;
	struct epoll_event event;
	int epfd, event_cnt;

	// 파라미터로 넘어온 구조체를 인자를 이용하여 현 합수의 변수들을 초기화 한다
	epoll_arg_t epoll_arg = *((epoll_arg_t *) arg);
	server_socket = epoll_arg.manager_fd;
	ep_events = epoll_arg.ep_evts;
	event = epoll_arg.evt;
	epfd = epoll_arg.fd;

	// 서버 실행
	while(1)
	{
		// epoll 이벤트 대기 (클라이언트 접속 또는 메시지 전송 감지)
		event_cnt=epoll_wait(epfd, ep_events, EPOLL_SIZE, -1);
		if(event_cnt==-1)
		{
			puts("epoll_wait() error");
			break;
		}

		// epoll에서 return 된 이벤트 갯수(이벤트가 발생한 파일 디스크립터의 수) 만큼 반복
		for(i=0; i<event_cnt; i++)
		{
			// 이벤트가 발생한 소켓을 컨트롤 하기 편하게 sock 변수에 저장
			int sock = ep_events[i].data.fd;

			// 이벤트를 받은 소켓이 server_socket(연결요청) 이면
			// server_socket은 listen함수에 의해 등록된 file descriptor 임
			if(sock==server_socket)
			{
				address_size= sizeof(client_address);
				// 연결
				client_socket=accept(sock, (struct sockaddr*)&client_address, &address_size);
				if(client_socket < 0)
					break;

				// 새로 연결된 client_socket을 EPOLLIN 상황에서 이벤트가 발생하도록 등록
				setnonblockingmode(client_socket);
				event.events=EPOLLIN|EPOLLET;
				event.data.fd=client_socket;
				epoll_ctl(epfd, EPOLL_CTL_ADD, client_socket, &event);

				// 접속자 count 증가
				socket_count++;

				// 대화상대 유/무 출력
				if(client_socket > DEFAULT_ALLOCATED_FD_AMOUNT)
				{
					print_newConnection();
					printf("현재 접속자수 %d 명\n", client_socket - DEFAULT_ALLOCATED_FD_AMOUNT);
					printf("=======================================\n");
					fflush(stdout);
				}
			}
			// 이벤트를 받은 소켓이 server_socket(연결요청) 이 아니면
			else
			{
				while(1)
				{
					// 이벤트가 발생한 소켓을 컨트롤 하기 편하게 clnt_sock 변수에 저장
					client_socket = ep_events[i].data.fd;

					// 버퍼를 비워줌 (비워주지 않으면 이상한 문자들이 출력되는 이슈가 있음 )
					memset ( buffer, 0, BUFFER_SIZE );
					received_msg_len=read(client_socket, buffer, BUFFER_SIZE);

					// 종료 요청이 들어오면
					if(received_msg_len==0)
					{
						epoll_ctl(epfd, EPOLL_CTL_DEL, client_socket, NULL);
						close(client_socket);
						printf("closed client: %d \n", client_socket);
						socket_count--;
						break;
					}
					// read에 error가 발생했는데 Non-Blocking Mode가
					// 할 일이 없으면 대기 없이 바로 리턴한다는 특성때문에 발생한 에러에 대한 대응
					else if(received_msg_len<0)
					{
						if(errno==EAGAIN || errno == EWOULDBLOCK){
							break;
						}
					}
					// read에서 문제가 없는경우 접속중인 모든 클라이언트들에게 메시지 발송
					// 문제가 없을경우 read()의 반환값은 1이상의 정수
					else
					{	
						// DEFAULT_ALLOCATED_FD_AMOUNT 는 매크로에 의해 6으로 정의되어 있음
						// 파일디스크립터가 0~5번까지 이미 선점되어 있기 떄문임
						// 0 : 표준입력 , 1 : 표준 출력 , 2 : 표준에러, 3 : epfd, 4 : server_socket, 5 : server_as_client_socket
						// 따라서 6번부터가 진짜 클라이언트들임
						for (j=DEFAULT_ALLOCATED_FD_AMOUNT; j<socket_count+DEFAULT_ALLOCATED_FD_AMOUNT; j++) {
							if (client_socket != j) {
								write(j, buffer, BUFFER_SIZE);
							}
						}				
					}
				}
			}
		}
	}
	return NULL;
}