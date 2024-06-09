#
# Copyright(c) 2020-2023. All rights reserved by Heekuck Oh.
# 이 파일은 한양대학교 ERICA 컴퓨터학부 학생을 위해 만들었다.
#
CC = gcc
CFLAGS = -Wall -O
CLIBS =
#
OS := $(shell uname -s)
ifeq ($(OS), Linux)
#	CLIBS += -lbsd
endif
ifeq ($(OS), Darwin)
#	CLIBS +=
endif
#
all: tsh.o
	$(CC) -o tsh tsh.o $(CLIBS)

tsh.o: tsh.c
	$(CC) $(CFLAGS) -c tsh.c

clean:
	rm -rf *.o
	rm -rf tsh
	
