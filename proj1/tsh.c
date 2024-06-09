// ? 23.03.22 - 23.03.27 | 컴퓨터학부 | 2020076735 | 박민선 | 리다이렉션, 파이프 기능 추가
/*
 * Copyright(c) 2023 All rights reserved by Heekuck Oh.
 * 이 프로그램은 한양대학교 ERICA 컴퓨터학부 학생을 위한 교육용으로 제작되었다.
 * 한양대학교 ERICA 학생이 아닌 이는 프로그램을 수정하거나 배포할 수 없다.
 * 프로그램을 수정할 경우 날짜, 학과, 학번, 이름, 수정 내용을 기록한다.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_LINE 80             /* 명령어의 최대 길이 */
#define WRITE_END 1             /* 파이프 write side */
#define READ_END 0              /* 파이프 read side */

/*
 * cmdexec - 명령어를 파싱해서 실행한다.
 * 스페이스와 탭을 공백문자로 간주하고, 연속된 공백문자는 하나의 공백문자로 축소한다. 
 * 작은 따옴표나 큰 따옴표로 이루어진 문자열을 하나의 인자로 처리한다.
 * 기호 '<' 또는 '>'를 사용하여 표준 입출력을 파일로 바꾸거나,
 * 기호 '|'를 사용하여 파이프 명령을 실행하는 것도 여기에서 처리한다.
 */
static void cmdexec(char *cmd)
{
    char *argv[MAX_LINE/2+1];   /* 명령어 인자를 저장하기 위한 배열 */
    int argc = 0;               /* 인자의 개수 */
    char *p, *q;                /* 명령어를 파싱하기 위한 변수 */
    int fd, fd2;                /* 파일 처리를 위한 변수 */
    bool is_input = 0, is_output = 0;   /* 입력 리다이렉션 | 출력 리다이렉션 여부 */
    int input_argc, output_argc;        /* 리다이렉션 기호 입력 당시의 명령어 갯수 */

    /*
     * 명령어 앞부분 공백문자를 제거하고 인자를 하나씩 꺼내서 argv에 차례로 저장한다.
     * 작은 따옴표나 큰 따옴표로 이루어진 문자열을 하나의 인자로 처리한다.
     */
    p = cmd; p += strspn(p, " \t");
    do {
        /*
         * 공백문자, 큰 따옴표, 작은 따옴표, <, >, |가 있는지 검사한다.
         */ 
        q = strpbrk(p, " \t\'\"<>|");
        /*
         * 공백문자가 있거나 아무 것도 없으면 공백문자까지 또는 전체를 하나의 인자로 처리한다.
         */
        if (q == NULL || *q == ' ' || *q == '\t') {
            q = strsep(&p, " \t");
            if (*q) argv[argc++] = q;
        }
        /*
         * 입력 리다이렉션이 필요한 경우
         * 파일명을 입력받기 위해 입력 리다이렉션 표시를 남기고,
         * 현재 명령어 갯수를 저장한다.
         */
        else if (*q == '<') {
            // ? "grep int<tsh.c" 와 같이, 띄어쓰기 없이 명령어를 입력한 경우를 대비
            q = strsep(&p, "<");
            if (*q) argv[argc++] = q;
            
            input_argc = argc;
            is_input = true;
        }
        /*
         * 출력 리다이렉션이 필요한 경우
         * 파일명을 입력받기 위해 출력 리다이렉션 표시를 남기고,
         * 현재 명령어 갯수를 저장한다.
         */
        else if (*q == '>') {
            q = strsep(&p, ">");
            if (*q) argv[argc++] = q;
            
            output_argc = argc;
            is_output = true;
        }
        /*
        * 파이프를 만난 순간, 앞에 있던 명령어를 실행하여 그 결과를 뒤에 있는 명령어의 입력으로 전달해야 한다.
        * 따라서 자식 프로세스는 손자 프로세스를 생성하고, 손자 프로세스는 파이프 이전 명령어를 모두 실행한다.
        * 손자 프로세스가 명령어를 실행한 결과를 파이프로 전달한다. 자식 프로세스는 손자 프로세스가 전달한 출력을 파이프로 받는다.
        */
        else if (*q == '|') {
            q = strsep(&p, "|");
            
            if (*q) argv[argc++] = q;
            
            /*
            * 자식-손자를 연결할 파이프를 생성
            */
            int fd_pipe[2];
            if (pipe(fd_pipe) == -1) {
                printf("PIPE ERROR\n");
                return ;
            }

            pid_t pid;
            if ((pid = fork()) < 0) {
                printf("FORK ERROR\n");
                return ;
            }
            /*
             * 손자 프로세스는 WRITE_END 파이프를 출력과 연결시킨 후,
             * 현재까지의 명령어를 실행한다.
             */
            if (pid == 0) {
                close(fd_pipe[READ_END]);
                dup2(fd_pipe[WRITE_END], STDOUT_FILENO);
                close(fd_pipe[WRITE_END]);

                argv[argc] = NULL;
                execvp(argv[0], argv);
            }
            /*
             * 자식 프로세스는 READ_END 파이프를 입력과 연결시킨 후,
             * 파이프(|) 이후 명령어부터 저장하기 위해 argc 값을 초기화한다.
             */
            else {
                close(fd_pipe[WRITE_END]);
                dup2(fd_pipe[READ_END], STDIN_FILENO);
                close(fd_pipe[READ_END]);
                argc = 0;
            }
        }
        /*
         * 작은 따옴표가 있으면 그 위치까지 하나의 인자로 처리하고, 
         * 작은 따옴표 위치에서 두 번째 작은 따옴표 위치까지 다음 인자로 처리한다.
         * 두 번째 작은 따옴표가 없으면 나머지 전체를 인자로 처리한다.
         */
        else if (*q == '\'') {
            q = strsep(&p, "\'");
            if (*q) argv[argc++] = q;
            q = strsep(&p, "\'");
            if (*q) argv[argc++] = q;
        }
        /*
         * 큰 따옴표가 있으면 그 위치까지 하나의 인자로 처리하고, 
         * 큰 따옴표 위치에서 두 번째 큰 따옴표 위치까지 다음 인자로 처리한다.
         * 두 번째 큰 따옴표가 없으면 나머지 전체를 인자로 처리한다.
         */
        else {
            q = strsep(&p, "\"");
            if (*q) argv[argc++] = q;
            q = strsep(&p, "\"");
            if (*q) argv[argc++] = q;
        }
        
        /*
         * 명령어를 한번 파싱할 때마다 리다이렉션을 처리할 준비가 되었는지 확인하기.
         * 리다이렉션 기호를 찾은 시점을 기준으로 명령어 수가 1만큼 증가했다는 것은
         * 파일명이 argv에 입력되었다는 의미이므로 리다이렉션을 처리한다.
         */
        if (is_input && argc == input_argc + 1) {
            /* 파일을 처리함과 동시에 파일명은 명령어 배열에서 삭제한다. */
            if ((fd = open(argv[--argc], O_RDONLY, 0)) > 0) {
                /* 아직 출력 리다이렉션이 처리되지 않은 경우, output_argc 값도 1 감소시킨다. */
                if (is_output) output_argc--;
                dup2(fd, STDIN_FILENO);
                close(fd);
            } else {
                printf("file error\n");
                return ;
            }
            /* 리다이렉션을 모두 처리하면 관련 flag 값을 갱신한다. */
            is_input = false;
        }
        else if (is_output && argc == output_argc + 1) {
            if ((fd2 = open(argv[--argc], O_WRONLY | O_TRUNC | O_CREAT, 0644)) > 0) {
                dup2(fd2, STDOUT_FILENO);
                close(fd2);
            } else {
                printf("file error\n");
                return ;
            }
            /* 리다이렉션을 모두 처리하면 관련 flag 값을 갱신한다. */
            is_output = false;
        }
    } while (p);
    argv[argc] = NULL;
    /*
     * argv에 저장된 명령어를 실행한다.
     */
    if (argc > 0)
        execvp(argv[0], argv);
}

/*
 * 기능이 간단한 유닉스 셸인 tsh (tiny shell)의 메인 함수이다.
 * tsh은 프로세스 생성과 파이프를 통한 프로세스간 통신을 학습하기 위한 것으로
 * 백그라운드 실행, 파이프 명령, 표준 입출력 리다이렉션 일부만 지원한다.
 */
int main(void)
{
    char cmd[MAX_LINE+1];       /* 명령어를 저장하기 위한 버퍼 */
    int len;                    /* 입력된 명령어의 길이 */
    pid_t pid;                  /* 자식 프로세스 아이디 */
    int background;             /* 백그라운드 실행 유무 */
    
    /*
     * 종료 명령인 "exit"이 입력될 때까지 루프를 무한 반복한다.
     */
    while (true) {
        /*
         * 좀비 (자식)프로세스가 있으면 제거한다.
         */
        pid = waitpid(-1, NULL, WNOHANG);
        if (pid > 0)
            printf("[%d] + done\n", pid);
        /*
         * 셸 프롬프트를 출력한다. 지연 출력을 방지하기 위해 출력버퍼를 강제로 비운다.
         */
        printf("tsh> "); fflush(stdout);
        /*
         * 표준 입력장치로부터 최대 MAX_LINE까지 명령어를 입력 받는다.
         * 입력된 명령어 끝에 있는 새줄문자를 널문자로 바꿔 C 문자열로 만든다.
         * 입력된 값이 없으면 새 명령어를 받기 위해 루프의 처음으로 간다.
         */
        len = read(STDIN_FILENO, cmd, MAX_LINE);
        if (len == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        cmd[--len] = '\0';
        if (len == 0)
            continue;
        /*
         * 종료 명령이면 루프를 빠져나간다.
         */
        if(!strcasecmp(cmd, "exit"))
            break;
        /*
         * 백그라운드 명령인지 확인하고, '&' 기호를 삭제한다.
         */
        char *p = strchr(cmd, '&');
        if (p != NULL) {
            background = 1;
            *p = '\0';
        }
        else
            background = 0;
        /*
         * 자식 프로세스를 생성하여 입력된 명령어를 실행하게 한다.
         */
        if ((pid = fork()) == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        /*
         * 자식 프로세스는 명령어를 실행하고 종료한다.
         */
        else if (pid == 0) {
            cmdexec(cmd);
            exit(EXIT_SUCCESS);
        }
        /*
         * 포그라운드 실행이면 부모 프로세스는 자식이 끝날 때까지 기다린다.
         * 백그라운드 실행이면 기다리지 않고 다음 명령어를 입력받기 위해 루프의 처음으로 간다.
         */
        else if (!background)
            waitpid(pid, NULL, 0);
    }
    return 0;
}
