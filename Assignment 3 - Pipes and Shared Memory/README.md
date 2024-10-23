# Assignment 3 - Inter-Process Communication
This is the third assignment for the 2016 Operating Systems course.
This code in particular was submitted in late May of 2016 and scored full points.
The original task description (and helper tester program) has since been lost, but below is an approximate description of it.

## Pipe Message Format
Some of the tasks in the assignment below will require you to send or receive messages through a named pipe. Messages can have one of two formats:
- signed or unsigned integer: will be sent directly, as 4-byte values
- character string: will first send a 1-byte value representing the length of the string, followed by the characters composing the string
For example, the string `test message` would be sent as:
```
| 12 | 't' | 'e' | 's' | 't' | ' ' | 'm' | 'e' | 's' | 's' | 'a' | 'g' | 'e' |
```
All messages sent and received in this assignment will follow this format.

## Task Description
You are to write a program in C which will be executed on a Linux machine.
Your program will be compiled and called by the tester.py program, and will then communicate with it via pipes.
The tester will send to your program a sequence of instructions and await response from your program after each one.
Your program should not terminate until it receives the `EXIT` instruction.

When receiving an instruction, your program should respond with the same instruction (e.g. if you received the message `PING`, you should respond with `PING`), followed by the specific response for that instruction.

### Pipe Setup
Your program should create and open for writing a named pipe with the name `RESP_PIPE_11629`. We will refer to this as the response pipe.
Your program should also open for reading a named pipe with the name `REQ_PIPE_11629`. This pipe will be created automatically by the tester program. We will refer to this as the request pipe.
Once the connection is established, your program should send the message `CONNECT` through the response pipe.

### PING
If your program receives the instruction `PING` through the request pipe, it should respond with the message `PONG` on the response pipe, followed by your variant number (`11629`).

### CREATE_SHM
If your program receives the instruction `CREATE_SHM` on the request pipe, the instruction will be followed by an integer message `SHM_SIZE`.
Your program should then create an shared memory region with key `17099`, size equal to `SHM_SIZE`, and permissions `0644` and attach it to its memory space.
If creating the shared memory region was successful, your program should send the message `SUCCESS` on the response pipe. Otherwise, you should send the message `ERROR`.

### WRITE_TO_SHM
If your program receives the instruction `WRITE_TO_SHM` on the request pipe, the instruction will be followed by an integer message `OFFSET` and an unsigned integer message `VALUE`.
Your program should write the value `VALUE` into the shared memory region at the offset `OFFSET`.
If the value would be written outside of the shared memory region or no shared memory region has been created yet, your program should send the message `ERROR` on the response pipe. Otherwise, you should send the message `SUCCESS` once the value has been written to shared memory.

### MAP_FILE
If your program receives the instruction `MAP_FILE` on the request pipe, the instruction will be followed by a character string message `FILENAME`.
Your program should map the file pointed to by `FILENAME` to its memory.
If mapping the file to memory was successful, your program should send the message `SUCCESS` on the response pipe. Otherwise, you should send the message `ERROR`.

### READ_FROM_FILE_OFFSET
If your program receives the instruction `READ_FROM_FILE_OFFSET` on the request pipe, the instruction will be followed by an integer message `OFFSET` and an unsigned integer message `NR_OF_BYTES`.
Your program should read a region of size `NR_OF_BYTES` at offset `OFFSET` in the memory-mapped file, and copy it to the beginning of the shared memory region.
If the bytes to be read are located (partially) outside the memory-mapped file, or if no memory-mapped file or shared memory region exists, your program should send the message `ERROR` on the response pipe. Otherwise, you should send the message `SUCCESS` once the bytes have been copied to shared memory.

### READ_FROM_FILE_SECTION
If your program receives the instruction `READ_FROM_FILE_SECTION` on the request pipe, the instruction will be followed by an unsigned integer message `SECTION_NR`, an integer message `OFFSET`, and an unsigned integer message `NR_OF_BYTES`.
If the memory-mapped file is a valid SF file (for a refresher on SF files, see Assignment 1), your program should read a region of size `NR_OF_BYTES` at offset `OFFSET` in the section indicated by `SECTION_NR`, and copy it to the beginning of the shared memory region.
If the memory-mapped is not a valid SF file, or the bytes to be read are located (partially) outside the memory-mapped file, or if no memory-mapped file or shared memory region exists, your program should send the message `ERROR` on the response pipe. Otherwise, you should send the message `SUCCESS` once the bytes have been copied to shared memory.

### READ_FROM_LOGICAL_SPACE_OFFSET
If your program receives the instruction `READ_FROM_FILE_SECTION` on the request pipe, the instruction will be followed by an integer message `LOGICAL_OFFSET` and an unsigned integer message `NR_OF_BYTES`.
If the memory-mapped file is a valid SF file, your program should read a region of size `NR_OF_BYTES` at offset `LOGICAL_OFFSET` in the LOGICAL ADDRESS SPACE of the SF file and copy it to the beginning of the shared memory region.
The logical address space of an SF file is formed as follows:
- each section is located in the logical memory space in the order their section headers are encountered in the SF file header
- each section must start at a logical offset that is a multiple of 4096
- sections in the logical address space are non-overlapping
For example, this is how the following sections would be mapped to logical address space:
```
section1: size=3 bytes, logical_space_offset=0
section2: size=1942 bytes, logical_space_offset=4096
section3: size=4097 bytes, logical_space_offset=8192
section4: size=17 bytes, logical_space_offset=16384
```
If the memory-mapped is not a valid SF file, or the bytes to be read are located (partially) outside the memory-mapped file, or if no memory-mapped file or shared memory region exists, your program should send the message `ERROR` on the response pipe. Otherwise, you should send the message `SUCCESS` once the bytes have been copied to shared memory.

### EXIT
If your program receives the instruction `EXIT` on the request pipe, it should close the request and response pipes, unlink the response pipe created at the beginning, detach and unlink any shared memory region, unmap any memory-mapped file, and terminate.