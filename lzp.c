/* Consult Project Repo `readme.md` for more information 
 * TODO: Turn into library, run time hash and model changes. */
#define LZP_PROJECT "LZP Compression CODEC"
#define LZP_AUTHOR  "Richard James Howe"
#define LZP_LICENSE "The Unlicense / Public Domain"
#define LZP_EMAIL   "howe.r.j.89@gmail.com"
#define LZP_REPO    "https://github.com/howerj/lzp"

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#ifndef LZP_MODEL
#define LZP_MODEL (0)
#endif

#if LZP_MODEL == 1
#define MODEL_BITS (8)
#define MODEL_SIZE  (1 << MODEL_BITS)
typedef uint8_t lzp_hash_t;
static lzp_hash_t lzp_model(lzp_hash_t h, uint8_t b) { (void)h; return b; }
#elif LZP_MODEL == 2
#define MODEL_BITS (16)
#define MODEL_SIZE  (1 << MODEL_BITS)
typedef uint16_t lzp_hash_t;
static lzp_hash_t lzp_model(lzp_hash_t h, uint8_t b) { return (h << 8) | b; }
#else /* default model */
#define MODEL_BITS (16)
#define MODEL_SIZE  (1 << MODEL_BITS)
typedef uint16_t lzp_hash_t;
static lzp_hash_t lzp_model(lzp_hash_t h, uint8_t b) { return (h << 4) ^ b; }
#endif

typedef struct {
	uint8_t model[MODEL_SIZE];
	int (*get)(void *in);
	int (*put)(void *out, int ch);
	void *in, *out;
} lzp_t;

int lzp_encode(lzp_t *l) {
	assert(l);
	assert(l->get);
	uint8_t *model = l->model, buf[8 + 1] = { 0, };
	lzp_hash_t hash = 0;
	for (int c = 0;c != EOF;) {
		int mask = 0, i = 0, j = 1;
		for (i = 0; i < 8; i++) {
			c = l->get(l->in);
			if (c == EOF)
				break;
			if (c == model[hash]) {
				mask |= 1 << i;
			} else {
				model[hash] = c;
				buf[j++] = c;
			}
			hash = lzp_model(hash, c);
		}
		if (i > 0) {
			buf[0] = mask;
			for (int k = 0; k < j; k++)
				if (l->put(l->out, buf[k]) < 0)
					return -1;
		}
	}
	return 0;
}

int lzp_decode(lzp_t *l) {
	assert(l);
	assert(l->get);
	uint8_t *model = l->model;
	lzp_hash_t hash = 0;
	for (int mask = 0;(mask = l->get(l->in)) != EOF;)
		for (int i = 0; i < 8; i++) {
			int c = 0;
			if (mask & (1 << i)) {
				c = model[hash];
			} else {
				c = l->get(l->in);
				if (c == EOF)
					break;
				model[hash] = c;
			}
			if (l->put(l->out, c) != c)
				return -1;
			hash = lzp_model(hash, c);
		}
	return 0;
}

static int get(void *in) { return fgetc(in); }
static int put(void *out, int ch) { return fputc(ch, out); }

#define UNUSED(X) ((void)(X))

#ifdef _WIN32 /* Used to unfuck file mode for "Win"dows. Text mode is for losers. */
#include <windows.h>
#include <io.h>
#include <fcntl.h>
static void binary(FILE *f) { _setmode(_fileno(f), _O_BINARY); }
#else
static inline void binary(FILE *f) { UNUSED(f); }
#endif

int main(int argc, char **argv) {
	int r = 0;
	lzp_t lzp = { .get = get, .put = put, .in = stdin, .out = stdout, };
	char ibuf[8192], obuf[8192];
	binary(stdin);
	binary(stdout);
	setvbuf(stdin,  ibuf, _IOFBF, sizeof (ibuf));
	setvbuf(stdout, obuf, _IOFBF, sizeof (obuf));
	if (argc != 2) goto usage;
	if      (strcmp(argv[1], "-c") == 0) r = lzp_encode(&lzp);
	else if (strcmp(argv[1], "-d") == 0) r = lzp_decode(&lzp);
	else goto usage;
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

