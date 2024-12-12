# Encrypted file format

Each file is built up starting with a file header followed by multiple chunks.

## File header

The file header consist of a 64 bytes header with the following layout

    | offset | length | description                    |
    +--------+--------+--------------------------------+
    | 0      | 21     | magic: \0Couchbase Encrypted\0 |
    | 21     | 1      | version                        |
    | 22     | 1      | compression                    |
    | 24     | 4      | unused (should be set to 0)    |
    | 27     | 1      | id len                         |
    | 28     | 36     | id bytes                       |
    | 64     | 16     | salt (uuid)                    |

    Total of 80 bytes

compression may be one of the following

    | 0 | No compression     |
    | 1 | Snappy             |
    | 2 | ZLIB               |
    | 3 | GZip               |
    | 4 | ZSTD               |
    | 5 | bzip2              |

The identifier may be up to 36 bytes, and should be written in printable
characters which may be used directly in logging or as an argument to
the command line tool dump-deks to look up the actual key. The length
identifier is used to have a single rule for the length rather than
requiring a key identifier shorter than 36 bytes to add a NUL byte
to terminate the id.

### Example

Below is an example for a file header with containing the key
`75c10c81-c8de-4c49-901a-7dd79562b3f6` using Snappy compression

    00000000: 0043 6f75 6368 6261 7365 2045 6e63 7279  .Couchbase Encry
    00000010: 7074 6564 0000 0100 0000 0024 3735 6331  pted.......$75c1
    00000020: 3063 3831 2d63 3864 652d 3463 3439 2d39  0c81-c8de-4c49-9
    00000030: 3031 612d 3764 6437 3935 3632 6233 6636  01a-7dd79562b3f6
    00000040: 3f12 8796 3b63 420a 9f2a d865 ec86 322f  ?...;cB..*.e..2/

## Chunk

Each chunk contains a fixed four byte header containing the
size (in network byte order) of the data which is encoded
as:

    nonce ++ ciphertext ++ tag

In version 0 the size of the nonce is 12 bytes and the tag is 16
bytes.

Even if 32 bit length field allows for really large chunks (4GB) one
should choose a reasonable chunk size keeping in mind that the process
decrypting the chunk should keep the entire chunk in memory.

Each chunk use the content of the header and file offset as associated
data (AD) generated on the following format:

    <header>offset

The offset is added as a 64 bit integer in network byte order.
