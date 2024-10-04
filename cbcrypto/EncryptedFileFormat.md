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

    Total of 64 bytes

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
`50143181-2803-40df-af7e-510f01ae6f7f`

    0x00000000	0x00 0x43 0x6f 0x75 0x63 0x68 0x62 0x61     .Couchba
    0x00000008	0x73 0x65 0x20 0x45 0x6e 0x63 0x72 0x79     se Encry
    0x00000010	0x70 0x74 0x65 0x64 0x20 0x46 0x69 0x6c     pted Fil
    0x00000018	0x65 0x00 0x00 0x24 0x35 0x30 0x31 0x34     e..$5014
    0x00000020	0x33 0x31 0x38 0x31 0x2d 0x32 0x38 0x30     3181-280
    0x00000028	0x33 0x2d 0x34 0x30 0x64 0x66 0x2d 0x61     3-40df-a
    0x00000030	0x66 0x37 0x65 0x2d 0x35 0x31 0x30 0x66     f7e-510f
    0x00000038	0x30 0x31 0x61 0x65 0x36 0x66 0x37 0x66     01ae6f7f

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

Each chunk use the filename (without path) and file offset as associated
data (AD) generated on the following format:

    filename:offset

Example:

    myfile.cef:123456
