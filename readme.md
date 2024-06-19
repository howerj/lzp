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

