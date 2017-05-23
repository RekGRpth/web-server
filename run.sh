#!/bin/sh

DEBUG=true
#DEBUG=false
RAGEL=true
#RAGEL=false
clear && \
clear && \
make clean && \
RAGEL=${RAGEL} DEBUG=${DEBUG} make && \
clear && \
clear && \
UV_THREADPOOL_SIZE=8 ./server
