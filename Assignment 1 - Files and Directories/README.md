# Assignment 1 - File System Module
This is the first assignment for the 2016 Operating Systems course.
This code in particular was submitted in early April of 2016 and scored full points.
The original task description (and helper tester program) has since been lost, but below is an approximate description of it.

## SF Files
Some of the tasks in the assignment below will require you to work with a special type of custom file called an SF file. A valid SF file is composed of a header section with a fixed format and a body section that contains multiple sections.

The header section of a valid SF file takes the following structure:
- MAGIC: 1 byte, must be 'z'
- HEADER_SIZE: 2 bytes, represents the length of the header, in bytes
- VERSION: 4 bytes, must be between 114 and 198 (inclusively)
- NUMBER_OF_SECTIONS: 2 bytes, represents the number of sections in the SF file's body, must be between 3 and 12 (inclusively)
- \[NUMBER_OF_SECTIONS\] section headers, each with the following structure:
    - SECTION_NAME: 18 bytes, represents the name of the section
    - SECTION_TYPE: 1 byte, must be between 21 and 52 (inclusively)
    - SECTION_OFFSET: 4 btyes, represents the offset in the SF file where the section starts
    - SECTION_SIZE: 4 bytes, represents the length of the section, in bytes

The body section of an SF file contains a number of sections (as described by the section headers above), as well as unspecified data outside the sections. It is possible for sections to overlap.

## Task Description
You are to write a program in C which will be executed on a Linux machine.
Your program will be compiled and called by the tester.py program with one or more options passed as command line arguments. For example:
`./a1 list path=/etc name_ends_with=zbf recursive`
Note that the options can be given in ANY ORDER (so, for example, the above command is equivalent with `./a1 name_ends_with=zbf recursive path=/etc list`).
Depending on the option received, your program will need to output different responses to STDOUT (which the tester will then check for correctness), then terminate.
If an error is encountered during processing, your program should output `ERROR`, followed by the cause of the error (e.g. `invalid directory path` or `invalid option: <OPTION>`). For example:
```
ERROR
invalid directory path
```
Otherwise, it should output `SUCCESS`, followed by the output corresponding to the option received.

### VARIANT
Format:
`./a1 variant`

If your program receives the option `variant`, your program should output your variant number (`11629`) on a new line.

### LIST
Format:
`./a1 list path=<PATH_TO_DIRECTORY> [recursive] [name_ends_with=<SUFFIX>] [permissions=<PERMISSIONS>]`

If your program receives the option `list`, it should list the contents of the directory at `PATH_TO_DIRECTORY`. Each entry in the directory should be listed with its complete partial path, on a new line. The order in which entries are listed is irrelevant.
For example, if `PATH_TO_DIRECTORY` is `etc/files`, and the directory structure is
```
etc
└── files
    ├── box.txt
    ├── aavvb.bat
    ├── subf
    │   └── subfile.txt
    └── zorp
```
your program should output
```
SUCCESS
etc/files/box.txt
etc/files/aavvb.bat
etc/files/subf
etc/files/zorp
```

Options:
- if your program receives the option `name_ends_with=<SUFFIX>`, only entries whose names end with `SUFFIX` should be listed
- if your program receives the option `permissions=<PERMISSIONS>`, only files whose permissions match `PERMISSIONS` (given in string format, e.g. `rwxr--r--`) should be listed
- if your program receives the option `recursive`, it should also recursively list contents of subdirectories in the given location (applying the same filtering rules)

### PARSE
Format:
`./a1 parse path=<PATH_TO_FILE>`

If your program receives the `parse` option, it should verify that the file at `PATH_TO_FILE` is a valid SF file (outputting an error if it is not), parse its header, and then output the version, number of sections, and, for each of its sections, its name, type and size.
Example of a successful output:
```
SUCCESS
version=114
nr_sections=4
section1: fgeohhereg, 22, 17
section2: feihephpoehth, 44, 2033
section3: pthe, 36, 10004
section4: nbebtowhgw, 50, 501
```

### EXTRACT
Format:
`./a1 extract path=<PATH_TO_SF_FILE> section=<SECTION_NR> line=<LINE_NR>`

If your program receives the `extract` option, it should open the file at `PATH_TO_SF_FILE`, verify that it is a valid SF file, then output the `LINE_NR`-th line from the `SECTION_NR`-th section in that file. New lines in the SF file sections are delimited with CR LF. For example, if the content of the section were
` | 'e' | 'x' | 't' | 'r' | 'a' | '\r' | '\n' | 't' | 'h' | 'i' | 's' | ' ' | 't' | 'e' | 'x' | 't' | '\r' | '\n' | ... `
then your program should output:
```
SUCCESS
this text
```

When implementing your solution, consider the fact that the line to be printed may be very long. (Solutions that use a naive approach of printing character by character may time out on some tests.)

### FINDALL
Format:
`./a1 findall path=<PATH_TO_DIRECTORY>`

If your program receives the `findall` option, it should recursively search the location at `PATH_TO_DIRECTORY` and its subdirectories and list all valid SF files that have no section greater than 1277 bytes in size. Each entry should contain the complete partial path and be printed on a new line, like for `list`. The order in which the files are displayed is irrelevant.