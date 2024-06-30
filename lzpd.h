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

#ifndef LZP_MODEL_BITS /* Models 1 and 3 only require 256 byte dictionaries */
#if (LZP_MODEL == 1) || (LZP_MODEL == 3)
#define LZP_MODEL_BITS (8)
#endif
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
LZP_EXTERN int lzp_encode(lzp_t *l); /* returns negative on failure */
LZP_EXTERN int lzp_decode(lzp_t *l); /* returns negative on failure */

#ifdef LZP_IMPLEMENTATION

#include <assert.h>

LZP_API unsigned short lzp_hash(unsigned short h, unsigned char b) { /* SOUNDEX might make an interesting model to try out, DJB2 does not work well */
	if (LZP_MODEL == 1) return b; /* N.B. The model size only needs to be 256 bytes in size for this hash */
	if (LZP_MODEL == 2) return (h << 8) | b;
	if (LZP_MODEL == 3) return 255 & ((h << 4) ^ b); /* Again, the model only needs to be 256 bytes */
	return (h << 4) ^ b; /* default model */
}

LZP_API int lzp_encode(lzp_t *l) {
	assert(l && l->get && l->hash);
	assert(sizeof (unsigned char) == 1 && sizeof (unsigned short) == 2);
	unsigned char *model = l->model, buf[8 + 1/* 8 byte run + control byte */] = { 0, };
	long hash = 0;
	for (int c = 0;c >= 0;) {
		int mask = 0, i = 0, j = 1;
		for (i = 0; i < 8; i++) {
			if ((c = l->get(l->in)) < 0)
				break;
			l->icnt++;
			assert(hash < LZP_MODEL_SIZE);
			if (c == model[hash]) { /* Match found, set control bit */
				mask |= 1 << i;
			} else { /* No match, update model */
				model[hash] = c;
				buf[j++] = c;
			}
			hash = l->hash(hash, c);
		}
		if (i > 0) { /* There might be a better way of encoding the control byte, popcnt(mask) is usually <= 4 */
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
	long hash = 0;
	for (int mask = 0;(mask = l->get(l->in)) >= 0;) {
		l->icnt++;
		for (int i = 0, c = 0; i < 8; i++) {
			assert(hash < LZP_MODEL_SIZE);
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
static int lzp_put_null(void *out, int ch) { (void)(out); return ch; }

static int lzp_model_save(lzp_t *l, FILE *out) {
	assert(l);
	assert(out);
	for (int i = 0; i < LZP_MODEL_SIZE; i++)
		if (fprintf(out, "%d\n", (int)(short)l->model[i]) < 0)
			return -1;
	return 0;
}

static int lzp_model_load(lzp_t *l, FILE *in) {
	assert(l);
	assert(in);
	for (int i = 0, d = 0; i < LZP_MODEL_SIZE; i++) {
		if (fscanf(in, "%d,", &d) < 0) /* optional comma */
			return -1;
		l->model[i] = d;
	}
	return 0;
}

int main(int argc, char **argv) {
	int r = 0;
	lzp_t lzp = { 
		.model = { 0, }, /* you could load in a custom model */
		.get   = lzp_get, 
		.put   = lzp_put, 
		.hash  = lzp_hash, 
		.in    = stdin, 
		.out   = stdout, 
		.icnt  = 0,
		.ocnt  = 0,
	};
	char ibuf[8192], obuf[8192];
	lzp_binary(stdin);
	lzp_binary(stdout);
	if (setvbuf(stdin,  ibuf, _IOFBF, sizeof (ibuf))) return 1;
	if (setvbuf(stdout, obuf, _IOFBF, sizeof (obuf))) return 1;
	if (argc == 3 && (!strcmp(argv[1], "-C") || !strcmp(argv[1], "-D"))) {
		FILE *f = fopen(argv[2], "rb");
		if (!f) { (void)fprintf(stderr, "Unable to open file '%s' for reading", argv[2]); return 1; }
		if (lzp_model_load(&lzp, f) < 0) r = -1;
		if (fclose(f) < 0) r = -1;
		if (!strcmp(argv[1], "-C")) r = lzp_encode(&lzp); else r = lzp_decode(&lzp);
	} else if (argc == 2) {
		if      (!strcmp(argv[1], "-c")) { r = lzp_encode(&lzp); }
		else if (!strcmp(argv[1], "-d")) { r = lzp_decode(&lzp); }
		else if (!strcmp(argv[1], "-m")) { lzp.put = lzp_put_null; r = lzp_encode(&lzp); if (r >= 0) r = lzp_model_save(&lzp, stdout); }
		else goto usage;
	} else {
		goto usage;
	}
	if (fprintf(stderr, "in  bytes %ld\nout bytes %ld\nratio     %.3f%%\n", 
		lzp.icnt, lzp.ocnt, 100. * (double)lzp.ocnt / (double)lzp.icnt) < 0)
		r = -1;
	if (fflush(stdout) < 0) r = -1; /* `obuf` does not persist after `main` exits, when fflush is called by the run time */
	return r != 0 ? 2 : 0;
usage:
	(void)fprintf(stderr, 
		"Usage:   %s -c|-d|-m|-C MODEL|-D MODEL\n"
		"Flags:   Model=%u/Bits=%u\n"
		"Project: " LZP_PROJECT "\n" 
		"Author:  " LZP_AUTHOR  "\n" 
		"Email:   " LZP_EMAIL   "\n" 
		"Repo:    " LZP_REPO    "\n" 
		"License: " LZP_LICENSE "\n", argv[0], LZP_MODEL, LZP_MODEL_BITS);
	return 1;
}
#endif
#endif
#ifdef __cplusplus
}
#endif
#endif

