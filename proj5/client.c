/*
 * Copyright(c) 2021-2023 All rights reserved by Heekuck Oh.
 * 이 프로그램은 한양대학교 ERICA 컴퓨터학부 학생을 위한 교육용으로 제작되었다.
 * 한양대학교 ERICA 학생이 아닌 이는 프로그램을 수정하거나 배포할 수 없다.
 * 프로그램을 수정할 경우 날짜, 학과, 학번, 이름, 수정 내용을 기록한다.
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <inttypes.h>
#include <stdatomic.h>
#include "pthread_pool.h"

#define NTASK 64
#define NLOOP 1024
#define RED "\e[0;31m"
#define CYAN "\e[0;36m"
#define RESET "\e[0m"

/*
 * 어떤 정수 x를 인자로 받아서 <x>를 화면에 출력한다.
 */
void number1(void *param)
{
    int num;
    
    num = *(int *)param;
    printf("<%d>", num);
}

/*
 * 어떤 정수 x를 인자로 받아서 [x]를 화면에 출력한다.
 */
void number2(void *param)
{
    int num;
    
    num = *(int *)param;
    printf("[%d]", num);
}

/*
 * 어떤 정수 x를 인자로 받아서 {x}를 화면에 출력한다.
 */
void number3(void *param)
{
    int num;
    
    num = *(int *)param;
    printf("{%d}", num);
}

/*
 * 1/16384 확률로 점을 화면에 출력한다.
 */
void dot(void *param)
{
    if ((rand() % 16384) == 0) {
        printf(".");
        fflush(stdout);
    }
}

/*
 * n이 소수인지 검사하는 매우 비효율적인 함수이다.
 */
bool isprime(uint64_t n)
{
    uint64_t p = 5;
    
    if (n == 2 || n == 3)
        return true;
    if (n == 1 || n % 2 == 0 || n % 3 == 0)
        return false;
    while (n >= p*p)
        if (n % p == 0 || n % (p + 2) == 0)
            return false;
        else
            p += 6;
    return true;
}

atomic_int count;
struct foo {
    int id;
    uint64_t num;
};

/*
 * n부터 n+100까지 수 가운데 소수가 있으면 출력하는 함수
 */
void find_primes(void *arg)
{
    int id = (*(struct foo *)arg).id;
    uint64_t n = (*(struct foo *)arg).num;

    for (int i = 0; i < 100; ++i, ++n)
        if (isprime(n)) {
            printf(CYAN"[T%d]"RESET"%"PRIu64"\n", id, n);
            count++;
        }
}

/*
 * 스레드 풀이 잘 구현되었는지 검증하기 위해 작성된 메인 함수이다.
 */
int main(void)
{
    int i, num[NTASK];
    pthread_pool_t pool1, pool2, pool3, pool4;
    struct timeval start, end;
    double elapsed;

    gettimeofday(&start, NULL);
    /*
     * 스레드풀의 한계를 검사한다.
     */
    printf("--- 스레드풀 파라미터 한계 검증 ---\n");
    if (pthread_pool_init(&pool1, 65536, 16) == POOL_FAIL)
        printf("pthreaad_pool_init(): 일꾼 스레드 최대 수 초과.....PASSED\n");
    else {
        printf("Error: 일꾼 스레드 갯수 검사 오류\n");
        return -1;
    }
    if (pthread_pool_init(&pool1, 16, 65536) == POOL_FAIL)
        printf("pthreaad_pool_init(): 대기열 최대 용량 초과.....PASSED\n");
    else {
        printf("Error: 대기열 용량 검사 오류\n");
        return -1;
    }
    
    /*
     * 스레드 풀이 정상적으로 초기화되고 종료되는지 검사한다.
     */
    printf("--- 스레드풀 최기화와 종료 검증 ---\n");
    if (pthread_pool_init(&pool1, 64, 64)) {
        printf("Error: 스레드풀 초기화 오류\n");
        return -1;
    }
    printf("pthread_pool_init(): 완료......PASSED\n");
    if (pthread_pool_shutdown(&pool1, POOL_DISCARD)) {
        printf("Error: 스레드풀 종료 오류\n");
        return -1;
    }
    printf("pthread_pool_shutdown(): 완료......PASSED\n");
    if (pthread_pool_init(&pool1, 64, 64)) {
        printf("Error: 스레드풀 초기화 오류\n");
        return -1;
    }
    printf("pthread_pool_init(): 완료......PASSED\n");
    if (pthread_pool_shutdown(&pool1, POOL_COMPLETE)) {
        printf("Error: 스레드풀 종료 오류\n");
        return -1;
    }
    printf("pthread_pool_shutdown(): 완료......PASSED\n");

    /*
     * 3개의 일꾼 스레드와 대기열의 크기가 10개인 스레드 풀을 가동한다.
     */
    printf("--- 스레드풀 기본 동작 검증 ---\n");
    pthread_pool_init(&pool1, 3, 10);
    /*
     * 함수 numberX()를 실행하기 위해 NTASK 갯수 만큼 스레드 풀에 요청한다.
     * 대기열은 길이가 10개로 짧아서 기다리지 않으면 처리할 수 없는 요청이 발생할 수 있다. 
     * 대기열이 차서 기다리지 않고 거절된 작업을 빨간색으로 출력한다.
     */
    for (i = 0; i < NTASK; ++i) {
        num[i] = i;
        if (pthread_pool_submit(&pool1, number1, num+i, POOL_NOWAIT))
            printf(RED"<%d>"RESET, i);
    }
    /*
     * POOL_WAIT 플래그를 사용하여 위 과정을 반복한다.
     * 대기열에 빈 자리가 날 때까지 기다리기 때문에 작업 요청은 항상 성공한다.
     */
    for (i = 0; i < NTASK; ++i) {
        num[i] = i;
        if (pthread_pool_submit(&pool1, number2, num+i, POOL_WAIT)) {
            printf("Error: POOL_WAIT 옵션 오류\n");
            return -1;
        }
    }
    /*
     * 스레드풀을 종료한다. 대기열에 실행이 안 된 작업이 남아 있어도 종료한다.
     */
    pthread_pool_shutdown(&pool1, POOL_DISCARD);
    printf("......PASSED\n");
    
    /*
     * 8개의 일꾼 스레드와 대기열의 크기가 16개인 스레드 풀을 가동한다.
     * 매우 느린 방식으로 소수를 찾는 작업 16개를 스레드풀에 요청한다.
     * 스레드풀을 종료하면 일부 작업은 진행 중이고 나머지 작업은 대기열에 있다.
     * 종료 옵션이 POOL_DISCARD이므로 현재 진행 중인 작업만 마치고 종료한다.
     */
    printf("--- 스레드풀 종료 방식 검증 ---\n");
    struct foo arg[16];
    pthread_pool_init(&pool1, 8, 16);
    count = 0;
    for (i = 0; i < 16; ++i) {
        arg[i].id = i;
        arg[i].num = 0x0fffffff00000000 + i*100;
        pthread_pool_submit(&pool1, find_primes, arg+i, POOL_WAIT);
    }
    pthread_pool_shutdown(&pool1, POOL_DISCARD);
    printf("소수 %d개를 찾았다.\n", count);
    if (count < 18)
        printf("일부 일꾼 스레드가 구동되기 전에 풀이 종료되었을 가능성이 높다. 오류는 아니다.\n");
    printf("스레드가 출력한 소수의 개수가 일치하는지 아래 값과 확인한다......PASSED\n");
    printf("T0(2), T1(3), T2(1), T3(1), T4(5), T5(3), T6(1), T7(2)\n");
    
    /*
     * 앞과 같은 작업을 요청하지만 종료 옵션을 POOL_COMPLETE로 바꾼다.
     * 현재 진행 중인 작업은 물론 대기열에 남아 있는 나머지 작업도 모두 마치고 종료한다.
     */
    pthread_pool_init(&pool1, 8, 16);
    count = 0;
    for (i = 0; i < 16; ++i) {
        arg[i].id = i;
        arg[i].num = 0x0fffffff00000000 + i*100;
        pthread_pool_submit(&pool1, find_primes, arg+i, POOL_WAIT);
    }
    pthread_pool_shutdown(&pool1, POOL_COMPLETE);
    if (count != 31) {
        printf("Error: POOL_COMPLETE 옵션 오류\n");
        return -1;
    }
    printf("소수 31개를 모두 찾았다.\n스레드가 출력한 소수의 개수가 일치하는지 아래 값과 확인한다......PASSED\n");
    printf("T0(2), T1(3), T2(1), T3(1), T4(5), T5(3), T6(1), T7(2)\n");
    printf("T8(2), T9(0), T10(1), T11(3), T12(1), T13(3), T14(0), T15(3)\n");

    /*
     * 스레드풀이 잘 작동하는지 무작위로 생성해서 검증한다.
     */
    printf("--- 무작위 검증 ---\n");
    srand(time(NULL));
    pthread_pool_init(&pool1, 3, 10);
    for (i = 0; i < NLOOP; ++i) {
        /*
         * 스레드풀 세 개를 무작위로 생성한다.
         */
        pthread_pool_init(&pool2, rand()%POOL_MAXBSIZE+1, rand()%POOL_MAXQSIZE+1);
        pthread_pool_init(&pool3, rand()%POOL_MAXBSIZE+1, rand()%POOL_MAXQSIZE+1);
        pthread_pool_init(&pool4, rand()%POOL_MAXBSIZE+1, rand()%POOL_MAXQSIZE+1);
        /*
         * 세 개에 스레드풀에 점을 출력하는 작업을 요청한다. flag의 선택은 무작위로 한다.
         */
        for (int j = 0; j < 2*NLOOP; ++j) {
            pthread_pool_submit(&pool2, dot, NULL, rand()%2?POOL_WAIT:POOL_NOWAIT);
            pthread_pool_submit(&pool3, dot, NULL, rand()%2?POOL_WAIT:POOL_NOWAIT);
            pthread_pool_submit(&pool4, dot, NULL, rand()%2?POOL_WAIT:POOL_NOWAIT);
        }
        /*
         * 앞에서 만들었던 스레드풀 1번은 지속적으로 사용한다.
         * 나머지 스레드풀을 종료한다. 종료 옵션은 무작위로 선택한다.
         */
        pthread_pool_submit(&pool1, number3, &i, POOL_NOWAIT);
        pthread_pool_shutdown(&pool2, rand()%2?POOL_COMPLETE:POOL_DISCARD);
        pthread_pool_shutdown(&pool3, rand()%2?POOL_COMPLETE:POOL_DISCARD);
        pthread_pool_shutdown(&pool4, rand()%2?POOL_COMPLETE:POOL_DISCARD);
    }
    /*
     * 마지막으로 스레드풀 1번을 종료하고 마친다.
     */
    pthread_pool_shutdown(&pool1, POOL_COMPLETE);
    printf("......PASSED\n");
    /*
     * 총 실행시간을 계산한다.
     */
    gettimeofday(&end, NULL);
    elapsed = (double)(end.tv_sec - start.tv_sec)+(double)(end.tv_usec - start.tv_usec)*1e-6;
    printf("총 실행시간: %.4f초\n", elapsed);

    return 0;
}
