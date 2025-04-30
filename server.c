#include <stdio.h>    
#include <stdlib.h>   
#include <string.h>    
#include <unistd.h>    
#include <pthread.h>       // 쓰레드 
#include <arpa/inet.h>     // IP 주소 처리 
#include <netinet/in.h>    // sockaddr_in 
#include <time.h>

#define MAX_CLIENTS 100     // 최대 동시접속자
#define BUF_SIZE 1024      // 메시지 사이즈
#define ID_SIZE 32         // ID 길이

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int server_port = 0;
int running = 1; 
int is_server = 0; 
int server_socket;
int client_count = 0;
char client_ids[MAX_CLIENTS][ID_SIZE] = {0};
int client_sockets[MAX_CLIENTS]; 
char my_id[ID_SIZE];

void run_server(int port);
void send_to_all(char *msg, int except_fd);
void* client_handler(void* arg);
void* game_control_p1(void* arg);
void* game_control_p2(void* arg);

int main(int argc, char *argv[], void* arg) {
    int client_sock = *(int *)arg;

    if (argc < 2) {                          // 2개도 안되는 쌩뚱맞은거 걸르고 끝냄 사용법 알림 // 2개부터는 됨 2개는 콘픽 열때 4개는 서버모드 6개는 클라이언트 모드
        printf("사용법: \n");
        printf("방장: %s -s -p <port>\n", argv[0]);
        exit(1);
    }

    if (strcmp(argv[1], "-s") == 0) {        // 일단 2개가 넘으니 [1] 자리에 -s가 있으면 서버모드이다
        int port = 0;                                  
        for (int i = 2; i < argc; i++) {     //   i=2 부터 총 들어온인자 전까지 1씩더해 -p위치 찾음    // 서버는 3개이고 클라는 4개 이므로 
            if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {         // i에 -p 찾았고 올라간 i 보다 1 더큰게 전체 인자수보다 작으면(i+1에 포트번호넣을거니)
                port = atoi(argv[i + 1]);                            // -p위치인 i에서 1더한 값에 포트번호 넣음
            }
        }
        if (port == 0) {                                              
            fprintf(stderr, "포트번호를 입력하세요.\n");
            exit(1);
        }
        run_server(port);
    } 
}

void run_server(int port) {
    server_port = port;                             // 전역변수에 포트번호 저장
    struct sockaddr_in serv_addr, cli_addr;         // 서버 및 클라이언트의 주소정보를 담을 구조체
    socklen_t cli_len = sizeof(cli_addr);           // 클라이언트 주소 길이   // os가 자동으로 채워줌
    pthread_t input_tid, recv_tid, t_id;            // input_tid = server_input_thread / recv_tid = server_recv_thread / t_id = client_handler
    
    server_socket = socket(AF_INET, SOCK_STREAM, 0);       // 소켓
    if (server_socket == -1) error_exit("socket()");
    int bufSize = 1024*10;

    memset(&serv_addr, 0, sizeof(serv_addr));              // 주소정보 설정
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);
    socklen_t len = sizeof(bufSize);

    setsockopt(server_socket, SOL_SOCKET, SO_SNDBUF, &bufSize, sizeof(bufSize));
    getsockopt(server_socket, SOL_SOCKET, SO_SNDBUF, &bufSize, &len);

    // pthread_create(&input_tid, NULL, server_input_thread, NULL);              // 서버(방장)의 채팅 입력을 받아서 처리 입력 전용 스레드  // 종료, 강퇴, 일반메시지 (스레드 id저장변수, 기본속성, 스레드시작함수포인터, 인자x)
    // pthread_create(&recv_tid, NULL, server_recv_thread, NULL);                // 서버의 메시지 출력전용 스레드 

    if (bind(server_socket, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)       // 바인딩
        error_exit("bind()");
    if (listen(server_socket, MAX_CLIENTS) == -1)                                         // 요청대기
        error_exit("listen()");
    while (1) {           
        int client_sock = accept(server_socket, (struct sockaddr*)&cli_addr, &cli_len);   // accept는 클라 접속요청때까지 대기 / 접속요청하면 클라 소켓 생성후 반환환
        pthread_create(&t_id, NULL, client_handler, (void*)&client_sock);
    }
    close(server_socket);
}

void* client_handler(void* arg) {
    pthread_t game_p1, game_p2;
    int client_sock = *(int *)arg;                       // pthread_create()는 어떤 타입이는 넘길수 있도록 void*인자를 써서 뭐든 그렇게 받은후 함수 안에서 형변환해서 사용 
    char buf[BUF_SIZE];                                   // 클라가 보낸 메시지 저장
    char id[ID_SIZE];                                     // 클라 아이디 저장장
    char now[6];                                          // HH:MM:SS + '\0'
    time_(now);                                           //현재시간

    read(client_sock, id, ID_SIZE);                       //  id를 읽는다 write 순서에 맞춰 read 도함 클라의 1 write가 id
    
    pthread_mutex_lock(&mutex);

    for (int i = 0; i < client_count; i++) {              // 동일 id 중복 검사사
        if (strcmp(client_ids[i], id) == 0) {
            char *msg = "[공지] 동일한 ID가 존재합니다.\n";   
            write(client_sock, msg, strlen(msg));            // 특정 클라에게 보냄
            close(client_sock);
            pthread_mutex_unlock(&mutex);
            return NULL;
        }
    }

    strcpy(client_ids[client_count], id);
                     
    client_sockets[client_count++] = client_sock;
    
    char join_msg[BUF_SIZE];

    sprintf(join_msg, "[공지] %s 님이 입장했습니다. 현재 접속자 수: %d명\n", id, client_count+1);     // 입장 메시지 및 접속자 수 한 줄 출력
    pthread_mutex_unlock(&mutex);  

    send_to_all(join_msg, -1);

    while(running) {
        memset(buf, 0, sizeof(buf));
        read(client_sock, buf, BUF_SIZE -1);

        if(strncmp(buf, "/b", 2) == 0 || strncmp(buf, "/B", 2) == 0) {
            char tmp_buf[BUF_SIZE];
            strcpy(tmp_buf, buf); 
            char* token;
            char tmp[BUF_SIZE] = {0};
            char target_id[BUF_SIZE] = {0};
            token = strtok(tmp_buf, " ");
            strcpy(tmp, token);
            token = strtok(NULL, " ");
            strcpy(target_id, token);

            int privatesock = -1;

            for (int i = 0; i < client_count; i++) {
                if (strcmp(client_ids[i], target_id) == 0) {  // 클라이언트 ID와 일치하는지 비교
                    privatesock = i;  // 일치하는 소켓 번호 반환
                }
            }
            if (privatesock < 0) {
                printf("[알림] 해당 참가자를 찾을 수 없습니다.\n");
            } else {
                pthread_mutex_lock(&mutex);
                sprintf(buf, "[%s]님의 게임신청 수락하시려면 1을 눌러주세요\n", id);
                write(client_sockets[privatesock], buf, strlen(buf));
                read(client_sock, buf, BUF_SIZE -1);
                if(strcmp(buf, "1") == 0) {
                    sprintf(buf, "game start");
                    write(client_sockets[privatesock], buf, strlen(buf));
                    write(client_sock, buf, strlen(buf));
                    pthread_create(&game_p1, NULL, game_control_p1, (void*)&client_sock);
                    pthread_create(&game_p2, NULL, game_control_p2, (void*)&client_sockets[privatesock]);
                }
                pthread_mutex_unlock(&mutex);
            }
        }
    }
}

void send_to_all(char *msg, int except_fd) {                 // 클라이언트 들에게 write 서버에 알림(요청) / except_fd : 자기자신 제외 용
    pthread_mutex_lock(&mutex);                              // 뮤텍스   클라이언트 소켓과 클라이언트 카운트는 여러 쓰레드에서 사용하므로 뮤텍스 필요
    for (int i = 0; i < client_count; i++) {                 
        if (client_sockets[i] != except_fd) {                // 클라이언트 소켓 배열 돌며 write로 전달 except_fd는제외
            write(client_sockets[i], msg, strlen(msg));
        }
    }
    pthread_mutex_unlock(&mutex);
}

void* game_control_p1(void* arg) {
    
}
void* game_control_p2(void* arg) {
    
}