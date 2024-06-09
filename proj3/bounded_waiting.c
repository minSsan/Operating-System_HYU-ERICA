/*
 * 2023.05.09 | 컴퓨터학부 | 2020076735 | 박민선 | bounded_waiting 문제를 스핀락으로 해결
 * Copyright(c) 2021-2023 All rights reserved by Heekuck Oh.
 * 이 프로그램은 한양대학교 ERICA 컴퓨터학부 학생을 위한 교육용으로 제작되었다.
 * 한양대학교 ERICA 학생이 아닌 이는 프로그램을 수정하거나 배포할 수 없다.
 * 프로그램을 수정할 경우 날짜, 학과, 학번, 이름, 수정 내용을 기록한다.
 */
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <stdatomic.h>

#define N 8             /* 스레드 개수 */
#define RUNTIME 100000  /* 출력량을 제한하기 위한 실행시간 (마이크로초) */

/*
 * ANSI 컬러 코드: 출력을 쉽게 구분하기 위해서 사용한다.
 * 순서대로 BLK, RED, GRN, YEL, BLU, MAG, CYN, WHT, RESET을 의미한다.
 */
char *color[N+1] = {"\e[0;30m","\e[0;31m","\e[0;32m","\e[0;33m","\e[0;34m","\e[0;35m","\e[0;36m","\e[0;37m","\e[0m"};

/*
 * waiting[i]는 스레드 i가 임계구역에 들어가기 위해 기다리고 있음을 나타낸다.
 * alive 값이 false가 될 때까지 스레드 내의 루프가 무한히 반복된다.
 */
bool waiting[N];
bool alive = true;

/*
 * 임계구역에 누군가 있는지 확인하는 변수
 * lock이 true면 현재 임계구역에 누군가 있으므로 접근이 불가능함을 의미한다.
 * lock이 false면 현재 임계구역에 아무도 없기 때문에 접근이 가능함을 의미한다.
 */
atomic_bool lock = false;

/*
 * N 개의 스레드가 임계구역에 배타적으로 들어가기 위해 스핀락을 사용하여 동기화한다.
 */
void *worker(void *arg)
{
    int i = *(int *)arg;
    
    while (alive) {
        /*
         * 스레드 i가 critical section 접근을 원함
         */
        waiting[i] = true;
        /*
         * lock의 상태가 해제(false)된 상태인지 확인하기 위한 기댓값
         */
        bool expected = false;

        /*
         * 스레드 i가 대기상태이면서 lock이 해제 상태가 아닌 동안 계속 대기한다.
         * - waiting[i]가 false -> 스레드 i가 더이상 대기하지 않고 cs에 접근 가능함을 의미(다른 스레드에 의해 대기상태가 해제)
         * - lock이 false -> cs의 접근 권한이 허용되었음을 의미
         * 위 두 조건 중 하나라도 성립하면 임계구역에 접근 가능하다.
         */
        while (waiting[i] && !atomic_compare_exchange_weak(&lock, &expected, true)) {
            expected = false; // expected가 true로 변경 -> false로 재설정
        }
        
        waiting[i] = false;
                
        /*
         * 임계구역: 알파벳 문자를 한 줄에 40개씩 10줄 출력한다.
         */
        for (int k = 0; k < 400; ++k) {
            printf("%s%c%s", color[i], 'A'+i, color[N]);
            if ((k+1) % 40 == 0)
                printf("\n");
        }
        /*
         * 임계구역이 성공적으로 종료되었다.
         */
        
        /*
         * turn을 다음 순서의 스레드 번호로 변경한다.
         */
        int turn = (i + 1) % N;
        /*
         * 자신의 순번으로 다시 넘어오기 전까지 모든 스레드의 대기상태를 검사한다.
         * 만약 대기중인 스레드가 있다면, 해당 스레드에게 접근 권한을 넘겨주기 위함이다.
         */
        while ((turn != i) && !waiting[turn]) {
            turn = (turn + 1) % N;
        }
        
        /*
         * 자신의 순번으로 되돌아왔다는 것은 현재 대기중인 스레드가 없다는 뜻이다.
         * 따라서 권한을 곧바로 넘겨주지 않고, lock을 그냥 해제한다.
         */
        if (turn == i) lock = false;
        /*
         * 대기중인 스레드가 있다면, lock을 해제하지 않고 해당 스레드에게 cs접근 권한을 넘겨준다.
         * -> waiting을 false로 바꾸면 해당 스레드가 while문을 빠져나와 cs에 접근 가능하게 된다.
         */
        else waiting[turn] = false;
    }
    pthread_exit(NULL);
}

int main(void)
{
    pthread_t tid[N];
    int i, id[N];

    /*
     * N 개의 자식 스레드를 생성한다.
     */
    for (i = 0; i < N; ++i) {
        id[i] = i;
        pthread_create(tid+i, NULL, worker, id+i);
    }
    /*
     * 스레드가 출력하는 동안 RUNTIME 마이크로초 쉰다.
     * 이 시간으로 스레드의 출력량을 조절한다.
     */
    usleep(RUNTIME);
    /*
     * 스레드가 자연스럽게 무한 루프를 빠져나올 수 있게 한다.
     */
    alive = false;
    /*
     * 자식 스레드가 종료될 때까지 기다린다.
     */
    for (i = 0; i < N; ++i)
        pthread_join(tid[i], NULL);
    /*
     * 메인함수를 종료한다.
     */
    return 0;
}
