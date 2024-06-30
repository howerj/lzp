# LZP Compression Code

* Author: Richard James Howe
* License: The Unlicense / Public Domain
* Email: <mailto:howe.r.j.89@gmail.com>
* Repo: <https://github.com/howerj/lzp>

This repo contains an implementation of the LZP lossless compression 
routine, this routine is incredibly simple. More complex than Run Length
Encoding but simpler than LZSS (another simple CODEC with a better compression
ratio than LZP). The virtues of this CODEC are its simplicity and speed,
compression ratio is not one of them.

The library is presented is a [Head Only Library](https://en.wikipedia.org/wiki/Header-only).

The way this CODEC works is that it either outputs a literal byte or it 
outputs a byte from a model based off of previously seen characters in a
dictionary. 

The format is:

* An 8 bit control character
* 0-8 literals.

Which is repeated until the end of input.

If a bit in the control character is zero it means we need to output an encoded
literal, otherwise we will output a byte from the predictors model. The model
is incredibly simple, we keep a running hash of the data. If the output of that
running hash is the same as the next literal we want to output we place a 1 for 
that byte in the control character, otherwise we have to output the literal.

This scheme limits the maximum bytes incompressible data can expand to adding
one byte for every eight (112.5%), and limits the gains to output one byte for 
every eight bytes (12.5%).

The End Of File condition is not contained within the format (the format is not
self terminating and relies on out of band signalling to indicate the input
stream is finished).

1. Initialize the model and running hash to zero (once only).
2. Get 8 bytes from an input source and store in an input buffer `buf`
3. Set the control byte to zero.
4. For each byte `b` in `buf` if `b` is in `model[hash]` then bitwise or in
a `1` into a control byte for the bit in the control byte that represents
that byte in the input buffer. If it is not then or in a `0` and add that byte
`b` to an output buffer, also add the byte `b` to the model with 
`model[hash] = b`. Update the hash with `hash = hash_function(hash, b)`.
5. Output the control byte and then output all bytes (0-8 bytes) in the
output buffer.
6. If there is more input go to step 2, otherwise terminate.

To decode:

1. Initialize the model and running hash to zero (once only).
2. Read in a single control byte.
3. For each bit `bit` in the control byte if the bit is zero read in
another byte `b` and set `model[hash] = b`. Output byte `b`. If the
bit `bit` was one then output the byte `model[hash]`. In either case the hash
is updated with the new output byte, `c`, as in 
`hash = hash_function(hash, c)`.
4. If there is more input go to step 2, otherwise terminate.

Both routines can be described in under thirty lines of C code (at the
time of writing both are 27 lines, this may change).

The hash used is often a weak one and can be experimented with. The hash
`hash = (hash << 4) ^ next_byte` is commonly used, and it mixes in new
data with the old. 4 bits are discarded, 4 bits are exclusively old, 4 bits
exclusively new, and 4 bits are are mixture of both old and new bytes.

## API

The library is structured as a header only library, as mentioned, it should
compile cleanly as C++. There are three exported functions and one structure.

The functions are `lzp_encode`, `lzp_decode` and `lzp_hash`. 

	typedef struct {
		unsigned char model[LZP_MODEL_SIZE]; /* predictor model */
		int (*get)(void *in);           /* like getchar */
		int (*put)(void *out, int ch);  /* like putchar */
		unsigned short (*hash)(unsigned short hash, unsigned char b); /* predictor */
		void *in, *out; /* passed to `get` and `put` respectively */
		unsigned long icnt, ocnt; /* input and output byte count respectively */
	} lzp_t;

The structure requires more explanation than the functions, once the structure
has been set up it is trivial to call `lzp_encode` or `lzp_decode`. The
functions are:

	unsigned short lzp_hash(unsigned short h, unsigned char b);
	int lzp_encode(lzp_t *l);
	int lzp_decode(lzp_t *l);

`lzp_hash` is the default hash function, it can be used to populate the
`hash` field in `lzp_t`.

The function pointers in `lzp_t` called `get` and `put` are used to read and
write a single character respectively, `in` and `out` (which will usually be
FILE pointers) are passed to `get` and `put`. They are analogues of `fgetc` and
`fputc`. Custom functions can be written to read and write to arbitrary
locations including memory.

There are also some macros, which can be defined by the user (they are
surrounded by `#ifndef` clauses).

	#define LZP_EXTERN extern /* applied to API function prototypes */
	#define LZP_API /* applied to all exported API functions */
	#define LZP_MODEL (0)
	#define LZP_MODEL_BITS (16)

And a derived macro which you should not change:

	#define LZP_MODEL_SIZE (1 << LZP_MODEL_BITS)

If you use an N-bit hash (up to 16-bits) then you can and should reduce the 
size of the model by setting `LZP_MODEL_BITS`. This is done automatically for
the built in models, if they are used.

There are comments about LZP that hint that using a "better" hash function 
(one that is better in the sense that it mixes its input better) will produce
better compression, from (minimal) testing this has been shown to not be the
case. It is often better to use a weak hash or even an identity function.

## DICTIONARY PRELOAD

It is possible to preload the dictionary with a model, this may improve
the compression ration, especially if the workload statistics are known in
advance. This can be done by populating the model table, the same values
should be used both for compression and decompression.

## BUGS AND LIMITATIONS

The input and output byte length counts are `unsigned long` values, which may
be 32-bit or 64-bit depending on your platform and compiler, if reading more
than 4GiB of data on a platform with 32-bit `long` types then this will
overflow.

## RETURN VALUE

The (example) program returns zero on success and non-zero on failure.

