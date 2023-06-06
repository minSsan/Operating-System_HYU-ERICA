/*
 * Copyright(c) 2021-2023 All rights reserved by Heekuck Oh.
 * 이 프로그램은 한양대학교 ERICA 컴퓨터학부 학생을 위한 교육용으로 제작되었다.
 * 한양대학교 ERICA 학생이 아닌 이는 프로그램을 수정하거나 배포할 수 없다.
 * 프로그램을 수정할 경우 날짜, 학과, 학번, 이름, 수정 내용을 기록한다.
 * 2023.04.05 | 컴퓨터학부 | 2020076735 | 박민선 | check_rows, check_columns, check_subgrid, check_sudoku 함수 내용 추가
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>

/*
 * 기본 스도쿠 퍼즐
 */
int sudoku[9][9] = {{6,3,9,8,4,1,2,7,5},{7,2,4,9,5,3,1,6,8},{1,8,5,7,2,6,3,9,4},{2,5,6,1,3,7,4,8,9},{4,9,1,5,8,2,6,3,7},{8,7,3,4,6,9,5,2,1},{5,4,2,3,9,8,7,1,6},{3,1,8,6,7,5,9,4,2},{9,6,7,2,1,4,8,5,3}};

/*
 * valid[0][0], valid[0][1], ..., valid[0][8]: 각 행이 올바르면 true, 아니면 false
 * valid[1][0], valid[1][1], ..., valid[1][8]: 각 열이 올바르면 true, 아니면 false
 * valid[2][0], valid[2][1], ..., valid[2][8]: 각 3x3 그리드가 올바르면 true, 아니면 false
 */
bool valid[3][9];

/*
 * 스도쿠 퍼즐의 각 행이 올바른지 검사한다.
 * 행 번호는 0부터 시작하며, i번 행이 올바르면 valid[0][i]에 true를 기록한다.
 */
void *check_rows(void *arg)
{
    /*
     * 0번 행부터 8번 행까지 순차적으로 검사한다.
     * 각 행은 또 다시 0번 ~ 8번 열로 구성되어 있으므로, 2중 for문을 사용하여 각 값에 접근한다.
     */
    for (int row = 0; row < 9; ++row) {
        /*
         * isVisited 배열은 인덱스에 해당하는 숫자가 현재 행에서 등장했는지 체크한다.
         * 단, 자연수 그대로를 인덱스로 사용하여 0번 인덱스는 사용하지 않는다.
         */
        bool isVisited[10] = {0};
        
        for (int column = 0; column < 9; ++column) {
            int current = sudoku[row][column];
            
            /*
             * 행 안에서 값이 중복하여 등장하는 경우
             * 해당 행이 올바르지 않음을 표시하고 해당 행의 검사를 중단한다.
             */
            if (isVisited[current]) {
                valid[0][row] = false;
                break; // 다음 행을 검사한다.
            }
            
            isVisited[current] = true;
            /*
             * 마지막 열에 대한 검사까지 성공적으로 마쳤다면, 행이 올바르게 구성되었음을 기록한다.
             */
            if (column == 8) valid[0][row] = true;
        }
    }
    pthread_exit(NULL);
}

/*
 * 스도쿠 퍼즐의 각 열이 올바른지 검사한다.
 * 열 번호는 0부터 시작하며, j번 열이 올바르면 valid[1][j]에 true를 기록한다.
 */
void *check_columns(void *arg)
{
    /*
     * 0번 열부터 8번 열까지 순차적으로 탐색한다.
     * 각 열은 또 다시 0번 ~ 8번 행으로 구성되어 있으므로, 2중 for문을 사용하여 각 값에 접근한다.
     */
    for (int column = 0; column < 9; ++column) {
        /*
         * isVisited 배열은 인덱스에 해당하는 숫자가 현재 열에서 등장했는지 체크한다.
         * 단, 자연수 그대로를 인덱스로 사용하여 0번 인덱스는 사용하지 않는다.
         */
        bool isVisited[10] = {0};
        
        for (int row = 0; row < 9; ++row) {
            int current = sudoku[row][column];
            
            /*
             * 열 안에서 값이 중복하여 등장하는 경우
             * 해당 열이 올바르지 않음을 표시하고 해당 열의 검사를 중단한다.
             */
            if (isVisited[current]) {
                valid[1][column] = false;
                break; // 다음 열을 검사한다.
            }
            
            isVisited[current] = true;
            /*
             * 마지막 행에 대한 검사까지 성공적으로 마쳤다면, 열이 올바르게 구성되었음을 기록한다.
             */
            if (row == 8) valid[1][column] = true;
        }
    }
    pthread_exit(NULL);
}

/*
 * 스도쿠 퍼즐의 각 3x3 서브그리드가 올바른지 검사한다.
 * 3x3 서브그리드 번호(k)는 0부터 시작하며, 왼쪽에서 오른쪽, 위에서 아래로 증가한다. (0 ~ 8)
 * k번 서브그리드가 올바르면 valid[2][k]에 true를 기록한다.
 */
void *check_subgrid(void *arg)
{
    /*
     * 전달받은 arg를 int형의 k 변수로 변환한다.
     * k를 통해 시작 행 번호, 시작 열 번호를 계산한다.
     * 1 ~ 9까지의 숫자가 그리드 내에 등장했는지 확인하기 위해 isVisited 배열을 사용한다.
     */
    int k = *(int*)arg;
    int row_start = (k / 3) * 3;
    int col_start = (k % 3) * 3;
    bool isVisited[10] = {0};
    
    /*
     * k번째 3 X 3 서브 그리드를 검사한다.
     */
    for (int i = row_start; i < row_start + 3; ++i) {
        for (int j = col_start; j < col_start + 3; ++j) {
            int current = sudoku[i][j];
            
            /*
             * 숫자가 중복해서 등장한 경우,
             * k번 서브그리드에 대해 false 표시를 남기고 스레드를 종료한다.
             */
            if (isVisited[current]) {
                valid[2][k] = false;
                pthread_exit(NULL);
            }
            isVisited[current] = true;
        }
    }
    /*
     * 숫자가 중복해서 등장하지 않으면 for문이 정상적으로 종료되므로,
     * k번 서브그리드에 대해 true 표시를 남긴다.
     */
    valid[2][k] = true;
    pthread_exit(NULL);
}

/*
 * 스도쿠 퍼즐이 올바르게 구성되어 있는지 11개의 스레드를 생성하여 검증한다.
 * 한 스레드는 각 행이 올바른지 검사하고, 다른 한 스레드는 각 열이 올바른지 검사한다.
 * 9개의 3x3 서브그리드에 대한 검증은 9개의 스레드를 생성하여 동시에 검사한다.
 */
void check_sudoku(void)
{
    int i, j;
    
    /*
     * 검증하기 전에 먼저 스도쿠 퍼즐의 값을 출력한다.
     */
    for (i = 0; i < 9; ++i) {
        for (j = 0; j < 9; ++j)
            printf("%2d", sudoku[i][j]);
        printf("\n");
    }
    printf("---\n");
    /*
     * 스레드를 생성하여 각 행을 검사하는 check_rows() 함수를 실행한다.
     */
    pthread_t row_thread;
    pthread_create(&row_thread, NULL, check_rows, NULL);
    /*
     * 스레드를 생성하여 각 열을 검사하는 check_columns() 함수를 실행한다.
     */
    pthread_t col_thread;
    pthread_create(&col_thread, NULL, check_columns, NULL);
    /*
     * 9개의 스레드를 생성하여 각 3x3 서브그리드를 검사하는 check_subgrid() 함수를 실행한다.
     * 3x3 서브그리드의 위치를 식별할 수 있는 값을 함수의 인자로 넘긴다.
     */
    pthread_t subgrid_threads[9];
    int arg[9];
    for (i = 0; i < 9; ++i) {
        arg[i] = i;
    }

    for (int k = 0; k < 9; ++k) {
        pthread_create(subgrid_threads+k, NULL, check_subgrid, arg+k);
    }
    /*
     * 11개의 스레드가 종료할 때까지 기다린다.
     */
    pthread_join(row_thread, NULL); // 행 검사 스레드
    pthread_join(col_thread, NULL); // 열 검사 스레드
    for (i = 0; i < 9; ++i) {       // 서브 그리드 검사 스레드
        pthread_join(subgrid_threads[i], NULL);
    }

    /*
     * 각 행에 대한 검증 결과를 출력한다.
     */
    printf("ROWS: ");
    for (i = 0; i < 9; ++i)
        printf(valid[0][i] ? "(%d,YES)" : "(%d,NO)", i);
    printf("\n");
    /*
     * 각 열에 대한 검증 결과를 출력한다.
     */
    printf("COLS: ");
    for (i = 0; i < 9; ++i)
        printf(valid[1][i] ? "(%d,YES)" : "(%d,NO)", i);
    printf("\n");
    /*
     * 각 3x3 서브그리드에 대한 검증 결과를 출력한다.
     */
    printf("GRID: ");
    for (i = 0; i < 9; ++i)
        printf(valid[2][i] ? "(%d,YES)" : "(%d,NO)", i);
    printf("\n---\n");
}

/*
 * alive 값이 true이면 각 스레드는 무한 루프를 돌며 반복해서 일을 하고,
 * alive 값이 false가 되면 무한 루프를 빠져나와 스레드를 종료한다.
 */
bool alive = true;

/*
 * 스도쿠 퍼즐의 값을 3x3 서브그리드 내에서 무작위로 섞는 함수이다.
 */
void *shuffle_sudoku(void *arg)
{
    int tmp;
    int grid;
    int row1, row2;
    int col1, col2;
    
    srand(time(NULL));
    while (alive) {
        /*
         * 0부터 8번 사이의 서브그리드 하나를 무작위로 선택한다.
         */
        grid = rand() % 9;
        /*
         * 해당 서브그리드의 좌측 상단 행열 좌표를 계산한다.
         */
        row1 = row2 = (grid/3)*3;
        col1 = col2 = (grid%3)*3;
        /*
         * 해당 서브그리드 내에 있는 임의의 두 위치를 무작위로 선택한다.
         */
        row1 += rand() % 3; col1 += rand() % 3;
        row2 += rand() % 3; col2 += rand() % 3;
        /*
         * 두 위치에 있는 값을 맞바꾼다.
         */
        tmp = sudoku[row1][col1];
        sudoku[row1][col1] = sudoku[row2][col2];
        sudoku[row2][col2] = tmp;
    }
    pthread_exit(NULL);
}

/*
 * 메인 함수는 위에서 작성한 함수가 올바르게 동작하는지 검사하기 위한 것으로 수정하면 안된다.
 */
int main(void)
{
    int i, tmp;
    pthread_t tid;
    struct timespec req;
    
    printf("********** BASIC TEST **********\n");
    /*
     * 기본 스도쿠 퍼즐을 출력하고 검증한다.
     */
    check_sudoku();
    for (i = 0; i < 9; ++i)
        if (valid[0][i] == false || valid[1][i] == false || valid[2][i] == false) {
            printf("ERROR: 스도쿠 검증오류!\n");
            return 1;
        }
    /*
     * 기본 퍼즐에서 세 개를 맞바꾸고 검증해본다.
     */
    tmp = sudoku[0][0]; sudoku[0][0] = sudoku[1][1]; sudoku[1][1] = tmp;
    tmp = sudoku[5][3]; sudoku[5][3] = sudoku[4][5]; sudoku[4][5] = tmp;
    tmp = sudoku[7][7]; sudoku[7][7] = sudoku[8][8]; sudoku[8][8] = tmp;
    check_sudoku();
    for (i = 0; i < 9; ++i)
        if ((i == 2 || i == 3 || i == 6) && valid[0][i] == false) {
            printf("ERROR: 행 검증오류!\n");
            return 1;
        }
        else if ((i != 2 && i != 3 && i != 6) && valid[0][i] == true) {
            printf("ERROR: 행 검증오류!\n");
            return 1;
        }
    for (i = 0; i < 9; ++i)
        if ((i == 2 || i == 4 || i == 6) && valid[1][i] == false) {
            printf("ERROR: 열 검증오류!\n");
            return 1;
        }
        else if ((i != 2 && i != 4 && i != 6) && valid[1][i] == true) {
            printf("ERROR: 열 검증오류!\n");
            return 1;
        }
    for (i = 0; i < 9; ++i)
        if (valid[2][i] == false) {
            printf("ERROR: 부분격자 검증오류!\n");
            return 1;
        }

    printf("********** RANDOM TEST **********\n");
    /*
     * 기본 스도쿠 퍼즐로 다시 바꾼 다음, shuffle_sudoku 스레드를 생성하여 퍼즐을 섞는다.
     */
    tmp = sudoku[0][0]; sudoku[0][0] = sudoku[1][1]; sudoku[1][1] = tmp;
    tmp = sudoku[5][3]; sudoku[5][3] = sudoku[4][5]; sudoku[4][5] = tmp;
    tmp = sudoku[7][7]; sudoku[7][7] = sudoku[8][8]; sudoku[8][8] = tmp;
    if (pthread_create(&tid, NULL, shuffle_sudoku, NULL) != 0) {
        fprintf(stderr, "pthread_create error: shuffle_sudoku\n");
        return -1;
    }
    /*
     * 무작위로 섞는 중인 스도쿠 퍼즐을 5번 검증해본다.
     * 섞는 중에 검증하는 것이므로 결과가 올바르지 않을 수 있다.
     * 충분히 섞을 수 있는 시간을 주기 위해 다시 검증하기 전에 1 usec 쉰다.
     */
    req.tv_sec = 0;
    req.tv_nsec = 1000;
    for (i = 0; i < 5; ++i) {
        check_sudoku();
        nanosleep(&req, NULL);
    }
    /*
     * shuffle_sudoku 스레드가 종료될 때까지 기다린다.
     */
    alive = 0;
    pthread_join(tid, NULL);
    /*
     * shuffle_sudoku 스레드 종료 후 다시 한 번 스도쿠 퍼즐을 검증해본다.
     * 섞는 과정이 끝났기 때문에 퍼즐 출력과 검증 결과가 일치해야 한다.
     */
    check_sudoku();
    for (i = 0; i < 9; ++i)
        if (valid[0][i] == true)
            printf("빙고! %d번 행이 맞았습니다.\n", i);
    for (i = 0; i < 9; ++i)
        if (valid[1][i] == true)
            printf("빙고! %d번 열이 맞았습니다.\n", i);
    for (i = 0; i < 9; ++i)
        if (valid[2][i] == false) {
            printf("ERROR: 부분격자 검증오류!\n");
            return 1;
        }
    
    return 0;
}
