CC_C = arm-linux-gnueabihf-gcc
CFLAGS = -Wall -g -std=c99 -D _POSIX_C_SOURCE=200809L -Werror -pthread 
OUTDIR = $(HOME)/cmpt433/public/myApps

all: sorter noworky
sorter:
	$(CC_C) $(CFLAGS) main.c sorter.c iSquaredC.c pot.c udpHandler.c -lm  -o ${OUTDIR}/sorter
noworky:
	$(CC_C) $(CFLAGS) noworky.c -o ${OUTDIR}/noworky
	
