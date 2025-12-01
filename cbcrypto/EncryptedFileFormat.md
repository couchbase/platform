# Encrypted file format

Each file is built up starting with a file header followed by multiple chunks.

## File header

The file header has the following layout:

    | offset | length | description                    |
    +--------+--------+--------------------------------+
    | 0      | 21     | magic: \0Couchbase Encrypted\0 |
    | 21     | 1      | version                        |
    | 22     | 1      | compression                    |
    | 23     | 1      | key derivation                 |
    | 24     | 3      | unused (should be set to 0)    |
    | 27     | 1      | id len                         |
    | 28     | 36     | id bytes                       |
    | 64     | 16     | salt (uuid)                    |

    Total of 80 bytes

Compression may be one of the following:

    | 0 | No compression     |
    | 1 | Snappy             |
    | 2 | ZLIB               |
    | 3 | GZip               |
    | 4 | ZSTD               |
    | 5 | bzip2              |

In version 1, key derivation is composed of two 4-bit parts. The least
significant bits determine the key derivation method and may be one of
the following:

    | 0 | No key derivation: use key as-is                      |
    | 1 | Key-based key derivation: KBKDF HMAC/SHA2-256/Counter |
    | 2 | Password-based key derivation: PBKDF2 SHA2-256        |

When password-based key derivation is used, the most significant 4 bits
encode the number of PBKDF2 iterations as follows:

    Iterations = 1024 * 2^EncodedValue

E.g., 0x32 is password-based key derivation with 8192 iterations.

No key derivation is used in version 0.

The identifier may be up to 36 bytes, and should be written in printable
characters which may be used directly in logging or as an argument to
the command line tool dump-deks to look up the actual key. The length
identifier is used to have a single rule for the length rather than
requiring a key identifier shorter than 36 bytes to add a NUL byte
to terminate the id.

When password-based key derivation is used, the key id is recommended
to always be "password", to simplify key lookup.

### Key derivation parameters

In the following, SaltString is the `salt (uuid)` in the file header,
encoded as the canonical text representation of a UUID.

#### Key-based

    Label: "Couchbase File Encryption"
    Context: "Couchbase Encrypted File/" + SaltString

#### Password-based

    PBKDF2 salt: "Couchbase Encrypted File/" + SaltString

### Examples

Below is an example of a file header using Snappy compression and the
key `75c10c81-c8de-4c49-901a-7dd79562b3f6` (v0):

    00000000: 0043 6f75 6368 6261 7365 2045 6e63 7279  .Couchbase Encry
    00000010: 7074 6564 0000 0100 0000 0024 3735 6331  pted.......$75c1
    00000020: 3063 3831 2d63 3864 652d 3463 3439 2d39  0c81-c8de-4c49-9
    00000030: 3031 612d 3764 6437 3935 3632 6233 6636  01a-7dd79562b3f6
    00000040: 3f12 8796 3b63 420a 9f2a d865 ec86 322f  ?...;cB..*.e..2/

Below is an example of a file header using password-based key derivation
and no compression (v1):

    00000000: 0043 6f75 6368 6261 7365 2045 6e63 7279  .Couchbase Encry
    00000010: 7074 6564 0001 0072 0000 0008 7061 7373  pted...r....pass
    00000020: 776f 7264 0000 0000 0000 0000 0000 0000  word............
    00000030: 0000 0000 0000 0000 0000 0000 0000 0000  ................
    00000040: 8fc0 ce3d e73e 4f9d 95aa c739 2c15 905d  ...=.>O....9,..]

## Chunk

Each chunk contains a fixed four byte header containing the
size (in network byte order) of the data which is encoded
as:

    Nonce + Ciphertext + Tag

In versions 0-1 the size of the nonce is 12 bytes and the tag is 16
bytes.

Even if 32 bit length field allows for really large chunks (4GB) one
should choose a reasonable chunk size keeping in mind that the process
decrypting the chunk should keep the entire chunk in memory.

Each chunk use the content of the header and file offset as associated
data (AD) generated on the following format:

    FileHeader + Offset

The offset is appended as a 64-bit integer in network byte order.
