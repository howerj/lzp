MODEL=0
FILE=lzp.h
CFLAGS=-Wall -Wextra -std=c99 -pedantic -O3 -fwrapv -DLZP_MODEL=${MODEL}

.PHONY: all default run test clean

all default: lzp

test run: lzp
	./lzp -c < ${FILE} > ${FILE}.lzp
	./lzp -d < ${FILE}.lzp > ${FILE}.orig
	cmp ${FILE} ${FILE}.orig

lzp: lzp.c lzp.h

lzp.1: readme.md

clean:
	git clean -dffx
