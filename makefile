MODEL=0
CFLAGS=-Wall -Wextra -std=c99 -pedantic -O2 -fwrapv -DLZP_MODEL=${MODEL}

.PHONY: all default run test clean

all default: lzp

test run: lzp
	./lzp -c < lzp.c > lzp.c.lzp
	./lzp -d < lzp.c.lzp > lzp.c.orig
	cmp lzp.c lzp.c.orig

clean:
	git clean -dffx
