/*
 * 2023.06.03 - 2023.06.06 | 컴퓨터학부 | 2020076735 | 박민선 | POSIX를 이용한 스레드풀 구현
 * Copyright(c) 2021-2023 All rights reserved by Heekuck Oh.
 * 이 프로그램은 한양대학교 ERICA 컴퓨터학부 학생을 위한 교육용으로 제작되었다.
 * 한양대학교 ERICA 학생이 아닌 이는 프로그램을 수정하거나 배포할 수 없다.
 * 프로그램을 수정할 경우 날짜, 학과, 학번, 이름, 수정 내용을 기록한다.
 */
#include "pthread_pool.h"
#include <stdlib.h>
/*
 * 스레드 풀: 작업을 제출하는 공간. 이 작업은 대기열에서 기다리다가 실행됨.
 * 사용자가 작업을 스레드풀에 제출하면 풀 안에 있는 스레드가 그 작업을 실행한다.
 * 제출된 작업은 대기열에서 기다리다 한가한 스레드에 의해 선택되어 실행된다. (스레드 -> 작업 선택, 선택 불가능 한 상황에서는 작업이 대기열에서 대기)
 * 대기열에 더 수행할 작업이 없으면 스레드는 새 작업이 들어와 깨워줄 때까지 대기상태 (스레드 -> 작업 선택, 작업이 없으면 작업이 생겨서 깨워줄 때까지 대기)
 */

/*
 * 풀에 있는 일꾼(일벌) 스레드가 수행할 함수이다.
 * FIFO 대기열에서 기다리고 있는 작업을 하나씩 꺼내서 실행한다.
 * 대기열에 작업이 없으면 새 작업이 들어올 때까지 기다린다.
 * 이 과정을 스레드풀이 종료될 때까지 반복한다.
 */
static void *worker(void *param)
{
    pthread_pool_t *pool = (pthread_pool_t *) param;
    // ? 스레드 풀이 실행 상태일 동안 반복
    while (pool->running) {
        pthread_mutex_lock(&(pool->mutex));

        // ? 스레드풀이 실행상태 이면서 대기열이 존재하지 않는 동안
        while (pool->running && pool->q_len == 0)
            pthread_cond_wait(&(pool->full), &(pool->mutex));
        
        // ? shutdown에서 스레드풀이 종료되면 더이상 작업을 대기하지 않고 종료시킨다.
        if (!pool->running) {
            pthread_mutex_unlock(&(pool->mutex));
            break;
        }
        
        // ? 대기열의 front에 위치한 작업을 불러온다.
        task_t *task_q = pool->q; // 대기열
        task_t task = task_q[pool->q_front]; // 대기열의 front에 위치한 작업

        // ? front의 위치, 대기열 길이를 갱신한다.
        pool->q_front = (pool->q_front + 1) % (pool->q_size);
        pool->q_len--;

        // ? 대기 중인 작업이 대기열에 들어갈 수 있도록 신호를 보낸다.
        pthread_cond_signal(&(pool->empty));
        pthread_mutex_unlock(&(pool->mutex));
        
        // * 불러온 작업을 실행한다
        task.function(task.param); // 작업 실행
    }
    pthread_exit(NULL);
}

/*
 * 스레드풀을 생성한다. bee_size는 일꾼(일벌) 스레드의 개수이고, queue_size는 대기열의 용량이다.
 * bee_size는 POOL_MAXBSIZE를, queue_size는 POOL_MAXQSIZE를 넘을 수 없다.
 * 일꾼 스레드와 대기열에 필요한 공간을 할당하고 변수를 초기화한다.
 * 일꾼 스레드의 동기화를 위해 사용할 상호배타 락과 조건변수도 초기화한다.
 * 마지막 단계에서는 일꾼 스레드를 생성하여 각 스레드가 worker() 함수를 실행하게 한다.
 * 대기열로 사용할 원형 버퍼의 용량이 일꾼 스레드의 수보다 작으면 효율을 극대화할 수 없다.
 * 이런 경우 사용자가 요청한 queue_size를 bee_size로 상향 조정한다.
 * 성공하면 POOL_SUCCESS를, 실패하면 POOL_FAIL을 리턴한다.
 */
int pthread_pool_init(pthread_pool_t *pool, size_t bee_size, size_t queue_size)
{
    // ? 스레드 수와 버퍼 크기가 최대 용량을 넘을 경우 POOL_FAIL을 리턴 후 종료
    if (bee_size > POOL_MAXBSIZE || queue_size > POOL_MAXQSIZE)
        return POOL_FAIL;
    
    // ? 원형 버퍼가 스레드 수보다 작으면 버퍼 크기를 스레드 수로 상향조정
    if (queue_size < bee_size)
        queue_size = bee_size;
    
    // ? bee_size, q_size 값 할당
    pool->bee_size = bee_size;
    pool->q_size = queue_size;
    // ? q, bee 배열 초기화 및 할당
    pool->q = calloc(queue_size, sizeof(task_t)); // 스택 영역은 함수가 종료되면서 값이 소멸되기 때문에 힙 영역에 할당한다.
    pool->bee = calloc(bee_size, sizeof(pthread_t));
    
    // ? q_front, q_len 값을 0으로 초기화
    pool->q_front = 0;
    pool->q_len = 0;

    // ? 뮤텍스 초기화
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);
    pool->mutex = mutex;

    // ? 조건변수 초기화
    pthread_cond_t full, empty;
    pthread_cond_init(&full, NULL);
    pthread_cond_init(&empty, NULL);
    pool->full = full;
    pool->empty = empty;

    // ? 스레드풀 실행 상태를 true로 설정
    pool->running = true;

    // ? 일꾼 스레드 생성 -> worker 실행하도록
    for (int i = 0; i < bee_size; ++i) {
        pthread_create(pool->bee+i, NULL, worker, pool);
    }
    return POOL_SUCCESS;
}

/*
 * 스레드풀에서 실행시킬 함수와 인자의 주소를 넘겨주며 작업을 요청한다.
 * 스레드풀의 대기열이 꽉 찬 상황에서 flag이 POOL_NOWAIT이면 즉시 POOL_FULL을 리턴한다.
 * POOL_WAIT이면 대기열에 빈 자리가 나올 때까지 기다렸다가 넣고 나온다.
 * 작업 요청이 성공하면 POOL_SUCCESS를 리턴한다.
 */
int pthread_pool_submit(pthread_pool_t *pool, void (*f)(void *p), void *p, int flag)
{
    /*
    pool:  스레드풀
    f:     작업을 수행할 함수 포인터
    p:     f 함수에 넘겨주는 인자 포인터
    flag:  대기열이 꽉 찼을 때 기다릴 지 여부 (POOL_WAIT / POOL_NOWAIT)
    */
    // ? 대기열을 조회하기 전 스레드 풀의 mutex에 락을 건다
    pthread_mutex_lock(&(pool->mutex));

    // ? 대기열이 꽉 찬 경우
    if (pool->q_len == pool->q_size) {
        // ? 대기하지 않는다면 POOL_FULL 리턴하여 종료
        if (flag == POOL_NOWAIT) {
            pthread_mutex_unlock(&(pool->mutex));
            return POOL_FULL;
        }
        // ? 대기를 원한다면 shutdown에서 스레드 풀을 종료하기 전까지, 대기열이 빌 동안 대기한다.
        while (pool->running && pool->q_len == pool->q_size)
            pthread_cond_wait(&(pool->empty), &(pool->mutex));
    }
    
    // ? 대기열이 빌 때까지 대기하는 도중에 스레드 풀이 종료된 경우, 대기열에 넣지 않고 종료시킨다.
    if (!pool->running) {
        pthread_mutex_unlock(&(pool->mutex));
        return POOL_FULL;
    }

    // ? 작업 생성 후 스레드풀의 대기열에 작업을 추가한다.
    task_t task;
    task.function = f;
    task.param = p;
    int input_index = (pool->q_front + pool->q_len) % (pool->q_size);
    pool->q[input_index] = task;
    pool->q_len++; // 대기열에 들어온 작업의 수를 1 증가시킨다.
    
    pthread_cond_signal(&(pool->full)); // 작업 생성을 기다리는 일꾼 스레드를 깨운다.
    pthread_mutex_unlock(&(pool->mutex));
    return POOL_SUCCESS;
}

/*
 * 스레드풀을 종료한다. 일꾼 스레드가 현재 작업 중이면 그 작업을 마치게 한다.
 * how의 값이 POOL_COMPLETE이면 대기열에 남아 있는 모든 작업을 마치고 종료한다.
 * POOL_DISCARD이면 대기열에 새 작업이 남아 있어도 더 이상 수행하지 않고 종료한다.
 * 부모 스레드는 종료된 일꾼 스레드와 조인한 후에 스레드풀에 할당된 자원을 반납한다.
 * 스레드를 종료시키기 위해 철회를 생각할 수 있으나 바람직하지 않다.
 * 락을 소유한 스레드를 중간에 철회하면 교착상태가 발생하기 쉽기 때문이다.
 * 종료가 완료되면 POOL_SUCCESS를 리턴한다.
 */
int pthread_pool_shutdown(pthread_pool_t *pool, int how)
{
    pthread_mutex_lock(&(pool->mutex));

    // ? POOL_COMPLETE인 경우
    if (how == POOL_COMPLETE) {
        // ? 대기열에 남아있는 모든 작업을 마칠 때까지 대기한다.
        while (pool->q_len > 0) {
            pthread_mutex_unlock(&(pool->mutex));
            pthread_mutex_lock(&(pool->mutex));
        }
    }
    
    // ? 조건변수에서 대기중인 스레드, 작업을 진행중인 스레드가 더이상 작업을 이어가지 않도록 running을 false로 설정한다.
    pool->running = false;

    // ? 대기열이 비어있는 상태에서 shutdown이 호출되면, full cond에서 작업 할당을 기다리고 있는 일꾼 스레드가 존재할 수 있다.
    pthread_cond_broadcast(&(pool->full));
    pthread_cond_broadcast(&(pool->empty));

    pthread_mutex_unlock(&(pool->mutex));

    // ? 모든 일꾼 스레드가 종료될 때까지 대기한다.
    for (int i = 0; i < pool->bee_size; ++i) {
        pthread_join(*(pool->bee+i), NULL);
    }

    pthread_cond_destroy(&(pool->empty));
    pthread_cond_destroy(&(pool->full));
    pthread_mutex_destroy(&(pool->mutex));
    free(pool->bee);
    free(pool->q);

    return POOL_SUCCESS;
}
