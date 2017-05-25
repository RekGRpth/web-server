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
WEB_SERVER_THREAD_COUNT=0 ./server
