#include <stdio.h>
#include <string.h> 
#include <stdlib.h> 
#include <time.h> 
#include <stdbool.h>
#include <unistd.h>
#include <malloc.h>
#include <termio.h>
#include <poll.h>
#include <pthread.h>
#include <termios.h>
// test

#define MAPSIZE 30

int** mapdata;
int playerX = 1;    // 플레이어의 초기 X 좌표
int playerY = MAPSIZE-2;    // 플레이어의 초기 Y 좌표
int player_2X = MAPSIZE-2;
int player_2Y = 1;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *player1_handler(void * arg);

int getch() {
    int c;
    struct termios oldattr, newattr;

    tcgetattr(STDIN_FILENO, &oldattr);           // 현재 터미널 설정 읽음
    newattr = oldattr;
    newattr.c_lflag &= ~(ICANON | ECHO);         // CANONICAL과 ECHO 끔
    newattr.c_cc[VMIN] = 1;                      // 최소 입력 문자 수를 1로 설정
    newattr.c_cc[VTIME] = 0;                     // 최소 읽기 대기 시간을 0으로 설정
    tcsetattr(STDIN_FILENO, TCSANOW, &newattr);  // 터미널에 설정 입력
    c = getchar();                               // 키보드 입력 읽음
    tcflush(STDIN_FILENO, TCIFLUSH);
    fflush(stdin);
    usleep(10000);
    tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);  // 원래의 설정으로 복구
    return c;
}


int main()
{
    mapdata = (int**)malloc(sizeof(int*)*MAPSIZE);
    for(int i = 0; i<MAPSIZE;i++) {
        mapdata[i] = (int*)malloc(sizeof(int)*MAPSIZE);
    }
    int countp1 = 0, countp2 = 0;
    int user_choice;
    pthread_t player1, player2, print_map;
    printf("게임을 시작하시겠습니까?(시작하기: 1)");
    scanf("%d", &user_choice);
    if(user_choice == 1)
    {
        mapdata[playerY][playerX] = 2;
        mapdata[player_2Y][player_2X] = 3;

        pthread_create(&player1, NULL, player1_handler, NULL);
        
        time_t start = time(NULL);
        while((time(NULL) - start) < 20.0000001) {
            countp1 = 0, countp2 = 0;
            system("clear");
            for(int i = 0; i< MAPSIZE; i++) {
                for(int j = 0; j<MAPSIZE; j++) {
                    if(mapdata[i][j] == 2) countp1++;
                    else if(mapdata[i][j] == 3) countp2++;
                }
            }
            printf("\n");
            for (int y = 0; y < MAPSIZE; y++) {
                for (int x = 0; x < MAPSIZE; x++) {
                    if ((y == playerY) && (x == playerX)) {
                        printf("🐇");    
                    }
                    else if ((y == player_2Y) && (x == player_2X)) {
                        printf("🐢");    
                    }
                    else if (mapdata[y][x] == 1) {
                        printf("  ");  // 벽d
                    }
                    else if (mapdata[y][x] == 2) {
                        printf("🥕");
                    }
                    else if (mapdata[y][x] == 3) {
                        printf("💧");
                    }
                    else {
                        printf("  ");  // 빈 공간
                    }
                    printf(" ");
                }
                if(y == 0) printf("\t1번 플레이어:  %d  개", countp1);
                if(y == 1) printf("\t2번 플레이어:  %d  개", countp2);
                if(y == 3) printf("\t남 은 시 간 :  %ld 초", (20-(time(NULL) - start)));
                if(y == 6 && (time(NULL) - start) > 19.9999) {
                    if(countp1 > countp2) printf("\t1번 플레이어 승리");
                    else if(countp1 < countp2) printf("\t2번 플레이어 승리");
                    else printf("\t무승부");
                } 
                printf("\n");
            }
            usleep(5000);
        }
    }
    getchar();
    tcflush(STDIN_FILENO, TCIFLUSH);
    while(getchar() != '\n');
    for(int i = 0; i<MAPSIZE;i++) {
        free(mapdata[i]);
    }
    free(mapdata);
}

void* player1_handler(void *arg) {


    char move;

    for (int y = 0; y < MAPSIZE; y++) {
        for (int x = 0; x < MAPSIZE; x++) {
            if (y == 0 || y == MAPSIZE - 1 || x == 0 || x == MAPSIZE - 1) {
                mapdata[y][x] = 1; // 테두리는 모두 벽으로
            }     
            else {
                mapdata[y][x] = 0;  // 빈 공간
            }
        }
        printf("\n");
    }


    while(1) {
        move = getch();
        usleep(10000);
        // 토끼
        if (move == 'a')                              // 1번을 입력했을때 
        {
            if ((mapdata[playerX-1][playerY] == 0) || (mapdata[playerX-1][playerY] == 2) || (mapdata[playerX-1][playerY] == 3)){        // map[playerX][playerY] ==  0 일때 playerX 를 -- 하여 좌표수정
                mapdata[playerY][playerX] = 2;
                playerX --;
                mapdata[playerY][playerX] = 2;
            }
        }
        else if (move == 'd')
        {
            if((mapdata[playerX+1][playerY] == 0) || (mapdata[playerX+1][playerY] == 2) || (mapdata[playerX+1][playerY] == 3)){
                mapdata[playerY][playerX] = 2;
                playerX ++;
                mapdata[playerY][playerX] = 2;
            }
        }
        else if (move == 'w')
        {
            if((mapdata[playerX][playerY-1] == 0) || (mapdata[playerX][playerY-1] == 2) || (mapdata[playerX][playerY-1] == 3)){
                mapdata[playerY][playerX] = 2;
                playerY --;
                mapdata[playerY][playerX] = 2;
            }
        }
        else if (move == 's')
        {
            if((mapdata[playerX][playerY+1] == 0) || (mapdata[playerX][playerY+1] == 2) || (mapdata[playerX][playerY+1] == 3)){
                mapdata[playerY][playerX] = 2;
                playerY ++;
                mapdata[playerY][playerX] = 2;
            }
        }
        // 거북이
        else if (move == 'D')     //왼                         // 1번을 입력했을때 
        {
            if ((mapdata[player_2X-1][player_2Y] == 0) || (mapdata[player_2X-1][player_2Y] == 2) || (mapdata[player_2X-1][player_2Y] == 3)){        // map[playerX][playerY] ==  0 일때 playerX 를 -- 하여 좌표수정
                mapdata[player_2Y][player_2X] = 3;
                player_2X --;
                mapdata[player_2Y][player_2X] = 3;
            }
        }
        else if (move == 'C')       //오
        {
            if((mapdata[player_2X+1][player_2Y] == 0) || (mapdata[player_2X+1][player_2Y] == 2) || (mapdata[player_2X+1][player_2Y] == 3)){
                mapdata[player_2Y][player_2X] = 3;
                player_2X ++;
                mapdata[player_2Y][player_2X] = 3;
            }
        }
        else if (move == 'A')   // 위
        {
            if((mapdata[player_2X][player_2Y-1] == 0) || (mapdata[player_2X][player_2Y-1] == 2) || (mapdata[player_2X][player_2Y-1] == 3)){
                mapdata[player_2Y][player_2X] = 3;
                player_2Y --;
                mapdata[player_2Y][player_2X] = 3;
            }
        }
        else if (move == 'B') // 아래
        {
            if((mapdata[player_2X][player_2Y+1] == 0) || (mapdata[player_2X][player_2Y+1] == 2) || (mapdata[player_2X][player_2Y+1] == 3)){
                mapdata[player_2Y][player_2X] = 3;
                player_2Y ++;
                mapdata[player_2Y][player_2X] = 3;
            }
        }
        if(mapdata[playerY][playerX] == mapdata[player_2Y][player_2X]) {
            mapdata[playerY][playerX] = 0;
        }


        tcflush(STDIN_FILENO, TCIFLUSH);
    }

}
