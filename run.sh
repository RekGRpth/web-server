#!/bin/sh

THREAD=0
#THREAD=1
#THREAD=5
#THREAD=10
DEBUG=true
#DEBUG=false
RAGEL=true
#RAGEL=false
clear && \
clear && \
make clean && \
RAGEL=${RAGEL} DEBUG=${DEBUG} THREAD=${THREAD} make && \
clear && \
clear && \
UV_THREADPOOL_SIZE=8 ./server
