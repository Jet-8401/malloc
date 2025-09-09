#!/bin/bash

make re &&
clang -g -O0 -std=c11 full_tests.c -L. -lft_malloc -Wl,-rpath=.
# LD_PRELOAD=./libft_malloc.so ./a.out
# valgrind ./a.out
# ./a.out
