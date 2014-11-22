// 헤더파일 인클루드
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define BUFFER_SIZE 4096
#define PORT 9000
#define LISTENER_QUEUE_SIZE 1024

// 함수 원형 선언
void error_handling(char *msg);
void print_usage();

// 메인함수
int main(int argc, char **argv)
{ 
  // 변수선언
  int listener_fd, connected_fd, read_msg_len, fd;
  // FILE* fp; 
  struct sockaddr_in server_address, client_address;
  socklen_t client_addr_len;
  
  char HTTPheader[] = "HTTP/1.0 200 OK\r\nDATE: Wed, 12 Mar 2014 00:14:10 GMT\r\n\r\n";
  char message[BUFFER_SIZE];
  char buffer[BUFFER_SIZE];

  // 문자열 parse 관련
  char  *end_of_file_location;
  char  *start_of_file_location;
  char  *file_location_ptr;
  char  file_locations[100];
  
  // 입력인자 검증
  if (argc != 1){
    printf("Usage : %s\n", argv[0]);
    exit(1);
  }

  // 소켓생성
  listener_fd=socket(AF_INET,SOCK_STREAM,0);
  if(listener_fd == -1)
    error_handling("socket() error");
  
  // 주소정보 초기화
  memset(&server_address,0,sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr=htonl(INADDR_ANY);
  server_address.sin_port=htons(PORT);
  
  // 소켓에 IP주소와 포트번호 할당
  if(bind(listener_fd,(struct sockaddr *)&server_address,sizeof(server_address)) == -1)
    error_handling("bind() error");
  
  // 사용자 안내문구 출력
  print_usage();

  // 클라이언트의 접속 요청 대기 - 서버 소켓 리스닝
  if(listen(listener_fd, LISTENER_QUEUE_SIZE) == -1)
    error_handling("listen() error");

  while(1)
  {
    // 연결
    client_addr_len=sizeof(client_address);
    connected_fd = accept(listener_fd,(struct sockaddr *)&client_address,&client_addr_len);
    
    // 예외처리
    if(connected_fd == -1)
      error_handling("accept() error");

    if (connected_fd < 4) 
      continue;

    // request 받음
    read_msg_len = recv(connected_fd,message,BUFFER_SIZE,0);
    message[read_msg_len] = 0;

    // request 문자열parse
    end_of_file_location = strstr(message, " HTTP");
    start_of_file_location = strstr(message, "/");
    
    memset(file_locations, 0, sizeof(file_locations));
    file_location_ptr = strncpy(file_locations, start_of_file_location, end_of_file_location-(start_of_file_location));
    file_location_ptr = strtok (file_locations, " ");

    // default Page parse
    if(!strcmp(file_locations, "/"))
    {
      file_location_ptr = "/index.html";
      // strcat(message,"index.html");
    }

    // 확인차 console에 출력
    printf( "%s\n", file_location_ptr);

    // header response
    send(connected_fd,HTTPheader,strlen(HTTPheader),0);

    // body response
    if((fd = (int)fopen(++file_location_ptr, "r")))
    { 
      // request 요청에 대한 파일이 존재하면 해당 파일을 보내주고
      while ((read_msg_len = fread(buffer, 1, BUFSIZ, (FILE *)fd)) > 0)
      {
        send(connected_fd,buffer,read_msg_len,0);
      }
    }
    else
    {
      // request 요청에 대한 파일이 존재하지 않으면 404 파일을 보내준다
      fd = open("error404.html", O_RDONLY);
      while ((read_msg_len = read(fd, buffer, BUFSIZ)) > 0)
      {
        send(connected_fd,buffer,read_msg_len,0);
      }
    }
    // 소켓닫기
    close(fd);
    close(connected_fd);
  }
}

// 표준 에러 스트림에 오류 출력
void error_handling(char *msg)
{
  fputs(msg, stderr);
  fputc('\n', stderr);
  exit(1);
}

void print_usage()
{
  printf("==================================================\n");
  printf("브라우저를 열어 주소창에 localhost:9000번을 입력하세요~\n");
  printf("localhost:9000입력시 내부 로직에 의해\n");
  printf("default page인 index.html 파일을 자동으로 불러옵니다\n");
  printf("localhost:9000/index.html 입력시 index.html파일을 불러옵니다\n");

  printf("서버에 존재하는 html파일은 error404.html, index.html 입니다\n");
  printf("이외의 주소 입력시 404 error page를 불러옵니다\n");
  printf("==================================================\n");
} 