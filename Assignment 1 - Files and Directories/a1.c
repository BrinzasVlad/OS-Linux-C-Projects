#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OPTION_BAD_OPTION -1
#define OPTION_VARIANT 0
#define OPTION_LIST 1
#define OPTION_PARSE 2
#define OPTION_EXTRACT 3
#define OPTION_FINDALL 4

#define MAX_PATH_LEN 256
#define BUF_SIZE 16384

struct sect_header {
    char sect_name[19]; //19 chars for
    unsigned char sect_type; //A "char", rather, since it's a 0-255 unsigned number really
    int sect_offset;
    int sect_size;
};

int get_option(int argc, char **argv) {
	/*
	* Being given the command line arguments, determines which
	* function the program should perform
	*/

	for (int i = 1; i < argc; ++i) {
		if (0 == strcmp(argv[i], "variant")) return OPTION_VARIANT;
		if (0 == strcmp(argv[i], "list")) return OPTION_LIST;
		if (0 == strcmp(argv[i], "parse")) return OPTION_PARSE;
		if (0 == strcmp(argv[i], "extract")) return OPTION_EXTRACT;
		if (0 == strcmp(argv[i], "findall")) return OPTION_FINDALL;
	}

	return OPTION_BAD_OPTION;
}

int is_small_SF_file(char *path) {
    /*
     * Tests whether the given file is a SF file or not
     * Return 1 if yes and 0 in all other cases.
     * SF files with any section over size 1277 are refused.
     */
    int fd = open(path, O_RDONLY);

    char magic;
    short header_size;
    int version;
    unsigned char no_of_sections; //Unsigned char as 1-byte int.

    struct sect_header header; //Header of verified section

    if (read(fd, &magic, 1) == 1 &&
        read(fd, &header_size, 2) == 2 &&
        read(fd, &version, 4) == 4 &&
        read(fd, &no_of_sections, 1) == 1) {
        //Read data and check that all read operations
        //have gone through properly

        //If reading goes well, check for file correctness
        if('z' != magic ||
           version < 114 || version > 198 ||
           no_of_sections < 3 || no_of_sections > 12) {

            return 0;
        }
    }
    else {
        return 0;
    }

    for (int i = 0; i < no_of_sections; ++i) {
        if (read(fd, header.sect_name, 18) == 18 &&
            read(fd, &(header.sect_type), 1) &&
            read(fd, &(header.sect_offset), 4) &&
            read(fd, &(header.sect_size), 4)) { //If everything is read okay

            //Test types and section size
            if (header.sect_type < 21 || header.sect_type > 52 ||
                header.sect_size > 1277) {
                return 0;
            }
        }
        else {
            return 0;
        }
    }

    //If everything else goes through, then we're good.
    return 1;
}

int test_ends_with(char *file_name, char *ending) {
    /*
     * Check that the file name ends with the given ending
     * Code idea guided by
     * https://stackoverflow.com/questions/10347689/how-can-i-check-whether-a-string-ends-with-csv-in-c
     */
     int name_len = strlen(file_name);
     int end_len = strlen(ending);

     if(0 == strcmp(file_name + name_len - end_len, ending))
        return 1;
     else
        return 0;
}

int test_permissions(struct stat *inode, char *permissions) {
    /*
     * Test that the file corresponding to the inode has the same permissions
     * as in the string permissions. The string is assumed to be correct.
     * Courtesy to the OS Lab (get-metadata.c) for the coding style
     */
     char file_perm[10];
     file_perm[9] = '\0';

     if (inode->st_mode & S_IRUSR) // Check owner's READ
        file_perm[0] = 'r';
	 else
        file_perm[0] = '-';

	if (inode->st_mode & S_IWUSR) // Check owner's WRITE
        file_perm[1] = 'w';
	else
        file_perm[1] = '-';

	if (inode->st_mode & S_IXUSR)		// check owner's EXECUTION
		file_perm[2] = 'x';
	else
		file_perm[2] = '-';

	if (inode->st_mode & S_IRGRP)		// check group's READ
		file_perm[3] = 'r';
	else
		file_perm[3] = '-';

	if (inode->st_mode & S_IWGRP)		// check group's WRITE
		file_perm[4] = 'w';
	else
		file_perm[4] = '-';

	if (inode->st_mode & S_IXGRP)		// check group's EXECUTION
		file_perm[5] = 'x';
	else
		file_perm[5] = '-';

	if (inode->st_mode & S_IROTH)		// check others' READ
		file_perm[6] = 'r';
	else
		file_perm[6] = '-';

	if (inode->st_mode & S_IWOTH)		// check others' WRITE
		file_perm[7] = 'w';
	else
		file_perm[7] = '-';

	if (inode->st_mode & S_IXOTH)		// check others' EXECUTION
		file_perm[8] = 'x';
	else
		file_perm[8] = '-';

    if(0 == strcmp(file_perm, permissions))
        return 1;
    else
        return 0;
}

void list_dir(char *path, int recursive, char* end_filter, char* perm_filter) {
    /*
     * Lists all the contents of the directory at the given path
     * that fit the given filters, if the filters are not null.
     * If recursive is not 0, recurses on any sub-folders.
     * The path is presumed to be valid, else nothing will be displayed
     * Directory traversal inspired from OS Lab (list-dir-contents.c)
     */

     DIR* dir;
     struct dirent *entry;
     struct stat inode;
     char entry_name[MAX_PATH_LEN];

     dir = opendir(path);
     if(NULL == dir) return; //This should never happen, but better safe than sorry.

     //Go through the contents
     while ((entry = readdir(dir)) != 0) {
        //Skip /. and /..
        if(0 == strcmp(entry->d_name, ".") || 0 == strcmp(entry->d_name, "..")) continue;

        //Build complete path
        /*LINUX*/    snprintf(entry_name, MAX_PATH_LEN, "%s/%s", path, entry->d_name);
        /*WINDOWS*/  //snprintf(entry_name, MAX_PATH_LEN, "%s\\%s", path, entry->d_name);

        //Get information - what type of file is it? Skip on fail.
        /*LINUX*/    if(0 != lstat(entry_name, &inode)) continue;
        /*WINDOWS*/  //if(0 != stat(entry_name, &inode)) continue;

        //Skip if requirements not met
        if(strlen(end_filter) > 0 && !test_ends_with(entry->d_name, end_filter)) continue;
        if(strlen(perm_filter) > 0 && !test_permissions(&inode, perm_filter)) continue;

        //Print and, if needed, recurse
        printf("%s\n", entry_name);
        if (S_ISDIR(inode.st_mode) && recursive)
            if(NULL != opendir(entry_name))
                list_dir(entry_name, recursive, end_filter, perm_filter);
     }

     closedir(dir);
}

void start_list_dir(int argc, char **argv) {
    /*
     * Wrapper function for list_dir that manages the nitty-gritty details
     */
    int recursive = 0;
    char dir_path[MAX_PATH_LEN];
    char end_filter[31]; //Max. 30 characters to look for at the end of filename
    char perm_filter[10]; //9 chars for rwx, plus /0

    strcpy(dir_path, ""); //Initialize to empty so we can tell if they're found or not
    strcpy(end_filter, "");
    strcpy(perm_filter, "");
    for (int i = 1; i < argc; ++i) {
        if (0 == strcmp(argv[i], "recursive"))
            recursive = 1;
        else if (argv[i] == strstr(argv[i], "path=")) // Command option that starts with "path="
            strcpy(dir_path, argv[i] + strlen("path="));
        else if (argv[i] == strstr(argv[i], "name_ends_with="))
            strncpy(end_filter, argv[i] + strlen("name_ends_with="), 30); //Copy no more than 30 characters
        else if (argv[i] == strstr(argv[i], "permissions="))
            strncpy(perm_filter, argv[i] + strlen("permissions="), 10);
        else if (0 == strcmp(argv[i], "list"))
            ; // Do nothing - we're only checking this case as a non-wrong-parameter case
        else {
            printf("ERROR\n");
            printf("invalid option: %s\n", argv[i]);
            return;
        }
    }

    if (0 == strlen(dir_path)) {
        printf("ERROR\n");
        printf("no path given\n");
        return;
    }
    if (NULL == opendir(dir_path)) {
        printf("ERROR\n");
        printf("invalid directory path\n");
        return;
    }
    if(0 != strlen(perm_filter)) { //Verify permissions only if given
        for (int i = 0; i < 3; ++i) {
            char perms[3] = "rwx";
            for (int j = 0; j < 3; ++j) {
                if (perm_filter[3*i + j] != perms[j] && perm_filter[3*i + j] != '-') {
                    printf("ERROR\n");
                    printf("wrong permission format\n");
                    return;
                }
            }
        }
    }

    printf("SUCCESS\n");
    list_dir(dir_path, recursive, end_filter, perm_filter);
}

void parse(char *path) {
    /*
     * Verifies the validity of a SF file and outputs data about it
     * The path is considered to be pointing to a valid, openable file.
     */
    int fd = open(path, O_RDONLY);

    char magic;
    short header_size; //Not quite sure what this is even used for!
    int version;
    unsigned char no_of_sections; //Although a "char", a char is a 1-byte int, so...

    struct sect_header *headers; //An unknown-size array of section headers

    if (read(fd, &magic, 1) == 1) {
        if('z' != magic) {
            printf("ERROR\n");
            printf("wrong magic\n");
            return;
        }
    }
    else {
        printf("ERROR\n");
        printf("magic not found\n");
        return;
    }

    if (read(fd, &header_size, 2) == 2)
        ;
    else {
        printf("ERROR\n");
        printf("header size not found\n");
        return;
    }

    if (read(fd, &version, 4) == 4) {
        if (version < 114 || version > 198) {
            printf("ERROR\n");
            printf("wrong version\n");
            return;
        }
    }
    else {
        printf("ERROR\n");
        printf("version not found\n");
        return;
    }

    if (read(fd, &no_of_sections, 1) == 1) {
        if (no_of_sections < 3 || no_of_sections > 12) {
            printf("ERROR\n");
            printf("wrong sect_nr");
            return;
        }
    }
    else {
        printf("ERROR\n");
        printf("sect_nr not found\n");
        return;
    }

    headers = malloc(no_of_sections * sizeof(struct sect_header)); //malloc() warning!

    for (int i = 0; i < no_of_sections; ++i) {
        if (read(fd, headers[i].sect_name, 18) == 18 &&
            read(fd, &(headers[i].sect_type), 1) &&
            read(fd, &(headers[i].sect_offset), 4) &&
            read(fd, &(headers[i].sect_size), 4)) { //In short, if everything reads okay for this section

            headers[i].sect_name[18] = '\0'; //Add terminator, juuust in case

            if (headers[i].sect_type < 21 || headers[i].sect_type > 52) {
                printf("ERROR\n");
                printf("wrong sect_types\n");
                free(headers);
                return;
            }
        }
        else {
            printf("ERROR\n");
            printf("sections not complete\n");
            free(headers);
            return;
        }
    }

    //At this point, everything has been successfully read.
    printf("SUCCESS\n");
    printf("version=%d\n", version);
    printf("nr_sections=%d\n", no_of_sections);
    for (int i = 0; i < no_of_sections; ++i) {
        printf("section%d: %s %d %d\n", i+1, headers[i].sect_name, headers[i].sect_type, headers[i].sect_size);
    }
    free(headers);
}

void start_parse(int argc, char **argv) {
    /*
     * Wrapper function for parse that manages the nitty-gritty details
     */
    char path[MAX_PATH_LEN];
    struct stat inode;

    strcpy(path, ""); //Initialize to empty so we can tell if found
    for (int i = 1; i < argc; ++i) {
        if (argv[i] == strstr(argv[i], "path=")) // Command option that starts with "path="
            strcpy(path, argv[i] + strlen("path="));
        else if (0 == strcmp(argv[i], "parse"))
            ; // Do nothing - we're only checking this case as a non-wrong-parameter case
        else {
            printf("ERROR\n");
            printf("invalid option: %s\n", argv[i]);
            return;
        }
    }

    //Check for possible mishaps before parsing
    if (0 == strlen(path)) {
        printf("ERROR\n");
        printf("no path given\n");
        return;
    }
    if (0 != stat(path, &inode)) {
        printf("ERROR\n");
        printf("invalid path\n");
        return;
    }
    if (-1 == open(path, O_RDONLY)) {
        printf("ERROR\n");
        printf("read permission denied\n");
        return;
    }

    parse(path);
}

void extract(char* path, int section, int line) {
    /*
     * Opens the SF file at the given path, and prints
     * the line given by line, from the section given
     * by section, or otherwise prints what went wrong.
     * The path is assumed to yield an openable file.
     *
     * This function has much in common with parse, since
     * it, too, must fail for an invalid SF file.
     */
    int fd = open(path, O_RDONLY);

    char magic;
    short header_size;
    int version;
    unsigned char no_of_sections; //Unsigned char as 1-byte int.

    struct sect_header header; //Header of our target section

    char buffer[BUF_SIZE + 1]; //16kB of memory, but most systems today can easily manage this
    int cur_line = 1; //The line we've gotten around to
    int bytes_read = 0; //How many bytes of the section we've processed so far
    char *cur_token; //Start of current line or part of line

    if (read(fd, &magic, 1) == 1 &&
        read(fd, &header_size, 2) == 2 &&
        read(fd, &version, 4) == 4 &&
        read(fd, &no_of_sections, 1) == 1) {
        //Read data and check that all read operations
        //have gone through properly

        //If reading goes well, check for file correctness
        if('z' != magic ||
           version < 114 || version > 198 ||
           no_of_sections < 3 || no_of_sections > 12) {

            printf("ERROR\n");
            printf("invalid file\n");
            return;
        }
        if(section > no_of_sections) {
            printf("ERROR\n");
            printf("invalid section\n");
            return;
        }
    }
    else {
        //If something wasn't read properly,
        //then the file format is incorrect.
        printf("ERROR\n");
        printf("invalid file\n");
        return;
    }

    for (int i = 0; i < section; ++i) {
        if (read(fd, header.sect_name, 18) == 18 &&
            read(fd, &(header.sect_type), 1) &&
            read(fd, &(header.sect_offset), 4) &&
            read(fd, &(header.sect_size), 4)) { //If everything is read okay

            header.sect_name[18] = '\0'; //Add terminator

            if (header.sect_type < 21 || header.sect_type > 52) {
                printf("ERROR\n");
                printf("invalid file\n");
                return;
            }
        }
        else {
            printf("ERROR\n");
            printf("invalid file\n");
            return;
        }
    }

    //At this point, our header has data about
    //the section we want, or we're already out.

    lseek(fd, header.sect_offset, SEEK_SET); //Move to target section
    int success_printed = 0; //Auxiliary to display the "SUCCESS" message only once
    while(bytes_read < header.sect_size && read(fd, buffer, BUF_SIZE) > 0) {
        //Read ~16k characters at once up to the section (or file) limit
        //For all but the largest lines, this will finish the job in one shot
        if (header.sect_size - bytes_read < BUF_SIZE) //This is the final read
            buffer[header.sect_size - bytes_read] = '\0'; //Cut buffer short with '\0'
        else
            buffer[BUF_SIZE] = '\0'; //Make sure we have a terminator

        cur_token = strtok(buffer, "\r"); //0x0D = \r, 0x0A = \n
        while(NULL != cur_token) {
            //Manage newlines
            if (cur_token[0] == '\n') {
                //We find a 0A after the 0D, so at the start of a new line
                //Else it means we are in a new buffer after a \r in the previous
                cur_token++;
                bytes_read++;
                cur_line++;
            }

            //Process the token
            if (cur_line < line) {
                //Nothing to do, just skip the bytes
                bytes_read += strlen(cur_token);
            }
            else if (cur_line > line) {
                //We've gone beyond our target line, so stop
                return;
            }
            else {
                //Print our current chunk
                if(!success_printed) {
                    printf("SUCCESS\n");
                    success_printed = 1;
                }
                printf("%s", cur_token);
                bytes_read += strlen(cur_token);
                //We can't stop for sure yet, since the current line
                //might stretch past the current buffer chunk
            }

            //Move to next strtok
            cur_token = strtok(NULL, "\r");
        }
    }

    //If we didn't print "SUCCESS" yet, then there was a line error
    if(!success_printed) {
        printf("ERROR\n");
        printf("invalid line\n");
        return;
    }
}

void start_extract(int argc, char **argv) {
    /*
     * Wrapper function for parse that manages the nitty-gritty details
     */
    char path[MAX_PATH_LEN];
    int section = -1;
    int line = -1;
    struct stat inode;

    strcpy(path, ""); //Initialize to empty so we can tell if found
    for (int i = 1; i < argc; ++i) {
        if (argv[i] == strstr(argv[i], "path=")) // Command option that starts with "path="
            strcpy(path, argv[i] + strlen("path="));
        else if (argv[i] == strstr(argv[i], "section="))
            sscanf(argv[i], "section=%d", &section);
        else if (argv[i] == strstr(argv[i], "line="))
            sscanf(argv[i], "line=%d", &line);
        else if (0 == strcmp(argv[i], "extract"))
            ; // Do nothing - we're only checking this case as a non-wrong-parameter case
        else {
            printf("ERROR\n");
            printf("invalid option: %s\n", argv[i]);
            return;
        }
    }

    //Check for possible mishaps before parsing
    if (0 == strlen(path)) {
        printf("ERROR\n");
        printf("no path given\n");
        return;
    }
    if (-1 == section){
        printf("ERROR\n");
        printf("no section given\n");
        return;
    }
    if (-1 == line) {
        printf("ERROR\n");
        printf("no line given\n");
        return;
    }
    if (0 != stat(path, &inode)) {
        printf("ERROR\n");
        printf("invalid path\n");
        return;
    }
    if (-1 == open(path, O_RDONLY)) {
        printf("ERROR\n");
        printf("read permission denied");
        return;
    }

    extract(path, section, line);
}

void findall (char *path) {
    /*
     * Lists all the SF files in target directory and its subfolders
     * only those SF files that have no section larger than
     * 1277 are to be displayed.
     * The path is assumed valid, else nothing will display
     */

     DIR* dir;
     struct dirent *entry;
     struct stat inode;
     char entry_name[MAX_PATH_LEN];

     dir = opendir(path);
     if(NULL == dir) return; //This should never happen, but better safe than sorry.

     //Go through the contents
     while ((entry = readdir(dir)) != 0) {
        //Skip /. and /..
        if(0 == strcmp(entry->d_name, ".") || 0 == strcmp(entry->d_name, "..")) continue;

        //Build complete path
        /*LINUX*/    snprintf(entry_name, MAX_PATH_LEN, "%s/%s", path, entry->d_name);
        /*WINDOWS*/  //snprintf(entry_name, MAX_PATH_LEN, "%s\\%s", path, entry->d_name);

        //Get information - what type of file is it? Skip on fail.
        /*LINUX*/    if(0 != lstat(entry_name, &inode)) continue;
        /*WINDOWS*/  //if(0 != stat(entry_name, &inode)) continue;

        //Print if valid SF file
        if(is_small_SF_file(entry_name))
            printf("%s\n", entry_name);

        //If directory, recurse
        if (S_ISDIR(inode.st_mode))
            if(NULL != opendir(entry_name))
                findall(entry_name);
     }

     closedir(dir);
}

void start_findall(int argc, char **argv) {
    /*
     * This is a wrapper function to set up for findall() and deal with the details.
     */
    char path[MAX_PATH_LEN];
    struct stat inode;

    strcpy(path, ""); //Initialize to empty so we can tell if found
    for (int i = 1; i < argc; ++i) {
        if (argv[i] == strstr(argv[i], "path=")) // Command option that starts with "path="
            strcpy(path, argv[i] + strlen("path="));
        else if (0 == strcmp(argv[i], "findall"))
            ; // Do nothing - we're only checking this case as a non-wrong-parameter case
        else {
            printf("ERROR\n");
            printf("invalid option: %s\n", argv[i]);
            return;
        }
    }

    //Check for possible mishaps before parsing
    if (0 == strlen(path)) {
        printf("ERROR\n");
        printf("no path given\n");
        return;
    }
    if (0 != stat(path, &inode)) {
        printf("ERROR\n");
        printf("invalid directory path\n");
        return;
    }

    printf("SUCCESS\n");
    findall(path);
}

int main(int argc, char **argv) {
    int option = get_option(argc, argv);

	/*
	 * TODO LIST
	 *
	 * Uncomment parts of test_permissions that are refused on Windows
	 * Swap stat() and other such functions in list_dir() to their Linux versions
	 */

	switch (option) {
	case OPTION_BAD_OPTION:
		printf("ERROR\n");
		printf("no option selected\n");
		break;
	case OPTION_VARIANT:
		printf("11629\n");
		break;
	case OPTION_LIST:
	    start_list_dir(argc, argv);
		break;
    case OPTION_PARSE:
        start_parse(argc, argv);
        break;
    case OPTION_EXTRACT:
        start_extract(argc, argv);
        break;
    case OPTION_FINDALL:
        start_findall(argc, argv);
        break;
	default:
		printf("A strange option has been encountered.\n");
		printf("If you see this message, please report to the developer.\n");
		break;
	}

	return 0;
}
