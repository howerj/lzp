#ifndef LZP_H
#define LZP_H
#ifdef __cplusplus
extern "C" {
#endif
/* LZP Header only library. Consult Project Repo `readme.md` for more information */
#define LZP_PROJECT "LZP Compression CODEC"
#define LZP_AUTHOR  "Richard James Howe"
#define LZP_LICENSE "The Unlicense / Public Domain"
#define LZP_EMAIL   "howe.r.j.89@gmail.com"
#define LZP_REPO    "https://github.com/howerj/lzp"

#ifndef LZP_EXTERN
#define LZP_EXTERN extern /* applied to API function prototypes */
#endif

#ifndef LZP_API
#define LZP_API /* applied to all exported API functions */
#endif

#ifndef LZP_MODEL /* hash/predictor selector */
#define LZP_MODEL (0)
#endif

#ifndef LZP_MODEL_BITS /* 16 or 8 are sensible values */
#define LZP_MODEL_BITS (16)
#endif

#define LZP_MODEL_SIZE (1 << LZP_MODEL_BITS)

typedef struct {
	unsigned char model[LZP_MODEL_SIZE]; /* predictor model */
	int (*get)(void *in);           /* like getchar */
	int (*put)(void *out, int ch);  /* like putchar */
	unsigned short (*hash)(unsigned short hash, unsigned char b); /* predictor */
	void *in, *out; /* passed to `get` and `put` respectively */
	unsigned long icnt, ocnt; /* input and output byte count respectively */
} lzp_t;

LZP_EXTERN unsigned short lzp_hash(unsigned short h, unsigned char b);
LZP_EXTERN int lzp_encode(lzp_t *l);
LZP_EXTERN int lzp_decode(lzp_t *l);

#ifdef LZP_IMPLEMENTATION

#include <assert.h>

LZP_API unsigned short lzp_hash(unsigned short h, unsigned char b) { /* SOUNDEX might make an interesting model to try out */
	if (LZP_MODEL == 1) return b; /* N.B. The model size only needs to be 256 bytes in size for this hash */
	if (LZP_MODEL == 2) return (h << 8) | b;
	if (LZP_MODEL == 3) return 255 & ((h << 4) ^ b);
	return (h << 4) ^ b; /* default model */
}

LZP_API int lzp_encode(lzp_t *l) {
	assert(l && l->get && l->hash);
	assert(sizeof (unsigned char) == 1 && sizeof (unsigned short) == 2);
	unsigned char *model = l->model, buf[8 + 1/* 8 byte run + control byte */] = { 0, };
	unsigned short hash = 0;
	for (int c = 0;c >= 0;) {
		int mask = 0, i = 0, j = 1;
		for (i = 0; i < 8; i++) {
			if ((c = l->get(l->in)) < 0)
				break;
			l->icnt++;
			if (c == model[hash]) {
				mask |= 1 << i;
			} else {
				model[hash] = c;
				buf[j++] = c;
			}
			hash = l->hash(hash, c);
		}
		if (i > 0) {
			buf[0] = mask;
			for (int k = 0; k < j; k++) {
				if (l->put(l->out, buf[k]) < 0)
					return -1;
				l->ocnt++;
			}
		}
	}
	return 0;
}

LZP_API int lzp_decode(lzp_t *l) {
	assert(l && l->get && l->hash);
	assert(sizeof (unsigned char) == 1 && sizeof (unsigned short) == 2);
	unsigned char *model = l->model;
	unsigned short hash = 0;
	for (int mask = 0;(mask = l->get(l->in)) >= 0;)
		for (int i = 0, c = 0; i < 8; i++) {
			if (mask & (1 << i)) {
				c = model[hash];
			} else {
				if ((c = l->get(l->in)) < 0)
					break;
				l->icnt++;
				model[hash] = c;
			}
			if (l->put(l->out, c) != c)
				return -1;
			l->ocnt++;
			hash = l->hash(hash, c);
		}
	return 0;
}

#ifdef LZP_MAIN
#include <stdio.h>
#include <string.h>
#ifdef _WIN32 /* Used to unfuck file mode for "Win"dows. Text mode is for losers. */
#include <windows.h>
#include <io.h>
#include <fcntl.h>
static inline void lzp_binary(FILE *f) { _setmode(_fileno(f), _O_BINARY); }
#else
static inline void lzp_binary(FILE *f) { (void)(f); }
#endif

static int lzp_get(void *in) { return fgetc((FILE*)in); }
static int lzp_put(void *out, int ch) { return fputc(ch, (FILE*)out); }

int main(int argc, char **argv) {
	int r = 0;
	lzp_t lzp = { 
		.model = { 0, }, 
		.get = lzp_get, 
		.put = lzp_put, 
		.hash = lzp_hash, 
		.in = stdin, 
		.out = stdout, 
		.icnt = 0,
		.ocnt = 0,
	};
	char ibuf[8192], obuf[8192];
	lzp_binary(stdin);
	lzp_binary(stdout);
	if (setvbuf(stdin,  ibuf, _IOFBF, sizeof (ibuf))) return 1;
	if (setvbuf(stdout, obuf, _IOFBF, sizeof (obuf))) return 1;
	if (argc != 2) goto usage;
	if      (strcmp(argv[1], "-c") == 0) r = lzp_encode(&lzp);
	else if (strcmp(argv[1], "-d") == 0) r = lzp_decode(&lzp);
	else goto usage;
	if (fprintf(stderr, "in  bytes %ld\nout bytes %ld\nratio     %.3f%%\n", 
		lzp.icnt, lzp.ocnt, 100.* (double)lzp.ocnt / (double)lzp.icnt) < 0)
		r = -1;
	return r != 0 ? 2 : 0;
usage:
	(void)fprintf(stderr, 
		"Usage:   %s -[cd]\n"
		"Project: " LZP_PROJECT "\n" 
		"Author:  " LZP_AUTHOR  "\n" 
		"Email:   " LZP_EMAIL   "\n" 
		"Repo:    " LZP_REPO    "\n" 
		"License: " LZP_LICENSE "\n", argv[0]);
	return 1;
}
#endif
#endif
#ifdef __cplusplus
}
#endif
#endif

