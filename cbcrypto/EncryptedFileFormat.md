# Encrypted file format

Each file is built up starting with a file header followed by multiple chunks.

## File header

The file header consists of two parts. A fixed size part and a variable
size part. If the key id's is plain numbers we can make the file header
fixed size containing the id in network byte order.

### Fixed size

    | offset | length | description     |
    +--------+--------+-----------------+
    | 0      | 5      | magic: \0CEF\0  |
    | 5      | 1      | version         |
    | 6      | 1      | id len          |

    Total of 7 bytes

### Variable size

This is the number of bytes used for the key id

### Example

In the current implementation only supporting AES-256-GCM the header for the key
`self:1` should look like:

    Offset 0 | 00 43 45 46 00    | Magic: \0CEF\0
    Offset 5 | 00                | Version: 0
    Offset 6 | 06                | Id length 6
    Offset 7 | 73 65 6c 66 3a 31 | self:1

    Total 13 bytes

## Chunk

Each chunk contains a fixed four byte header containing the
size (in network byte order) of the data which is encoded
as:

    nonce ++ ciphertext ++ tag

In version 0 the size of the nonce is 12 bytes and the tag is 16
bytes.
