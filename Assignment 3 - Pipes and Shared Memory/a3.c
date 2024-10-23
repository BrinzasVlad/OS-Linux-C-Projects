#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define SHM_KEY 17099
#define SHM_RIGHTS 0644

char* formatString(char* string) {
    ///Takes as input a normal, \0-terminated string
    ///Returns a string in our format, with length preceding the letters and no \0
    ///String must be deallocated externally!
    int length = strlen(string);
    char* result = (char*) malloc((length + 1) * sizeof(char)); //sizeof(char) is 1, but just in case
    result[0] = (char) length; //Store length as first value
    for(int i = 0; i < length; ++i) result[i+1] = string[i]; //Copy the actual array contents, bypassing \0
    return result;
}

char* unformatString(char* string) {
    ///Takes as input a string in our format, with no terminator
    ///Returns a normal string, terminated in \0
    ///String must be deallocated externally!
    int length = (int) (string[0]);
    char* result = (char*) malloc((length + 1) * sizeof(char)); //Again, sizeof(char)
    result[length] = '\0'; //Make sure we have terminator
    for(int i = 0; i < length; ++i) result[i] = string[i+1]; //Copy the actual string
    return result;
}

int main()
{
    char task[50];

    char *function = NULL;
    char *string_arg = NULL; //Initialise variables with default formats
    char string_arg_in[200];
    int int_arg = -1337;
    unsigned int uint_arg = 0;
    unsigned int uint_arg_2 = 0;

    int shm_id;
    void* shm = (void*) -1;
    int shm_size;

    void* mem = (void*) -1;
    int mem_size;

    unlink("RESP_PIPE_11629"); //Just in case.
    if(0 != mkfifo("RESP_PIPE_11629", 0777)) {
        printf("ERROR\n");
        printf("cannot create the response pipe");
        exit(1);
    }
    int req_pipe = open("REQ_PIPE_11629", O_RDONLY);
    int resp_pipe = open("RESP_PIPE_11629", O_WRONLY); //Says nothing about printing an error if we cannot open this!

    if(0 > req_pipe) {
        printf("ERROR\n");
        printf("cannot open the request pipe");
        close(resp_pipe);
        unlink("RESP_PIPE_11629");
        exit(1);
    }

    function = formatString("CONNECT");
    write(resp_pipe, function, 8); //Write "CONNECT"
    free(function);

    while(1) {
        //Execute in loop until asked to exit
        read(req_pipe, task, 1); //Read number of bytes
        read(req_pipe, task + 1, (int)task[0]); //Read the rest of the bytes, too

        //Do a switch on the command
        //Set arguments; set to NULL and -1337 respectively for no-arg
        function = unformatString(task);

        if(0 == strcmp(function, "PING")) { //PING
            string_arg = formatString("PONG");
            int_arg = 11629;
        }


        else if(0 == strcmp(function, "CREATE_SHM")) { //CREATE SHARED MEMORY
            read(req_pipe, &int_arg, sizeof(int)); //Shm size

            shm_size = int_arg;
            shm_id = shmget(SHM_KEY, shm_size, IPC_CREAT | SHM_RIGHTS);
            shm = shmat(shm_id, NULL, 0);

            int_arg = -1337; //Release
            if((void*)-1 == shm) string_arg = formatString("ERROR");
            else string_arg = formatString("SUCCESS");
        }


        else if(0 == strcmp(function, "WRITE_TO_SHM")) { //WRITE TO SHARED MEMORY
            read(req_pipe, &int_arg, sizeof(int)); //Offset
            read(req_pipe, &uint_arg, sizeof(unsigned int)); //Uint value

            int offset = int_arg;
            if(offset < 0 || offset + sizeof(unsigned int) > shm_size) {
                string_arg = formatString("ERROR");
            }
            else {
                *((unsigned int*) ((char*) shm + offset)) = uint_arg; //We first cast to char* so we can move offset bytes forward. Then, we cast to unsigned int* in order to actually write.
                string_arg = formatString("SUCCESS");
            }

            int_arg = -1337; //Release
            uint_arg = 0;
        }


        else if(0 == strcmp(function, "MAP_FILE")) { //MAP FILE IN MEMORY
            read(req_pipe, string_arg_in, 1); //Length of the filename
            read(req_pipe, string_arg_in + 1, (int) string_arg_in[0]); //Actual name of the filename
            char* filename = unformatString(string_arg_in); //Nice name of the filaname

            struct stat buf;
            if(0 == stat(filename, &buf)) {
                //All good, keep going.
                mem_size = buf.st_size;
                int file_desc = open(filename, O_RDONLY);
                if(-1 != file_desc) {
                    mem = mmap(NULL, mem_size, PROT_READ, MAP_PRIVATE, file_desc, 0); //Map the whole file, from 0 to mem_size - 1
                    if((void*)-1 != mem) {
                        string_arg = formatString("SUCCESS");
                    }
                    else {
                        //File was opened, but we cannot map it for some reason.
                        string_arg = formatString("ERROR");
                    }
                    close(file_desc);
                }
                else {
                    //File exists, but we may not open it.
                    string_arg = formatString("ERROR");
                }
            }
            else {
                //No such file exists, or we really can't do anything with it.
                string_arg = formatString("ERROR");
            }

            free(filename); //Free memory used.
        }


        else if(0 == strcmp(function, "READ_FROM_FILE_OFFSET")) { //READ FROM FILE FROM OFFSET
            read(req_pipe, &int_arg, sizeof(int)); //Offset
            read(req_pipe, &uint_arg, sizeof(unsigned int)); //Bytes

            if((void*)-1 == mem || //No memory-mapped file
                (void*)-1 == shm || //No shared memory to write results to
                int_arg < 0 || int_arg + uint_arg > mem_size) //Wrong offset / too many bytes
            {
                string_arg = formatString("ERROR");
            }
            else {
                for(int i = 0; i < uint_arg; ++i) {
                    ((char*)shm)[i] = ((char*)mem)[i + int_arg]; //I hope this is not bothersome and works fast
                }
                string_arg = formatString("SUCCESS");
            }

            int_arg = -1337; //Release
            uint_arg = 0;
        }


        else if(0 == strcmp(function, "READ_FROM_FILE_SECTION")) { //READ FROM FILE FROM SECTION
            read(req_pipe, &uint_arg_2, sizeof(unsigned int)); //Section
            read(req_pipe, &int_arg, sizeof(int)); //Offset
            read(req_pipe, &uint_arg, sizeof(unsigned int)); //No. bytes

            if((void*)-1 == mem || //No memory-mapped file
                (void*)-1 == shm || //No shared memory to write results to
                mem_size < 8) //File is so small it cannot possibly an SF file
            {
                //We cannot work at all without shared memory and memory mapped file
                string_arg = formatString("ERROR");
            }
            else {
                //The file is memory-mapped, check that it is a SF file; note that we only check the type for the section we are to read.
                char magic = *(char*)mem;
                int version = *(int*) ((char*)mem + 3); //Again, char* for traversal, int* for actual data.
                //if(4 > sizeof(int)) version = version | 0xffffffff; //Truncate to 8 x 4 = 32 bits, or 4 bytes; uncomment this line if you think that will be relevant
                short no_sections =  (short)( *((char*)mem + 7) ); //Take the one byte, THEN cast to short (once we're past the pointer matters)
                if(magic != 'z' || //Wrong magic
                    version < 114 ||  version > 198 || //Wrong version
                    no_sections < 5 || no_sections > 14 || //Wrong number of sections
                    uint_arg_2 > no_sections || uint_arg_2 < 0) //Wrong given section
                {
                    //Not a valid SF file
                    string_arg = formatString("ERROR");
                }
                else {
                    //It's an SF file and the section exists; find the section offset and read
                    char* sect_header = (char*)mem + 8 /*the file header*/ + 27 * (uint_arg_2 - 1) /*the N-1 sections we skip*/;

                    if(sect_header + 17 - (char*)mem /*distance in file, section header included*/ > mem_size) {
                        //The file runs out of size before the section header ends
                        string_arg = formatString("ERROR");
                    }
                    else {
                        //We can actually read the whole header
                        short sect_type = (short) (*(sect_header + 18));
                        int sect_offset = *(int*)(sect_header + 19);
                        int sect_size  = *(int*)(sect_header + 23);
                        if(21 != sect_type && 52 != sect_type) {
                            //Wrong section type
                            string_arg = formatString("ERROR");
                        }
                        else {
                        //Test that we're still in the file, then read.
                        if(sect_offset + uint_arg > mem_size || sect_offset < 0 || uint_arg > sect_size) {
                            //More data was asked for than we can provide
                            string_arg = formatString("ERROR");
                        }
                            else {
                                //Finally, all is good, now just copy!
                                int total_offset = sect_offset + int_arg;
                                for(int i = 0; i < uint_arg; ++i) {
                                    ((char*)shm)[i] = ((char*)mem)[i + total_offset];
                                }
                                string_arg = formatString("SUCCESS");
                            }
                        }
                    }
                }

            }

            int_arg = -1337; //Release
            uint_arg = 0;
            uint_arg_2 = 0;
        }


        else if(0 == strcmp(function, "READ_FROM_LOGICAL_SPACE_OFFSET")) { //READ FROM LOGICAL SPACE FROM OFFSET
            read(req_pipe, &int_arg, sizeof(int)); //Logical offset
            read(req_pipe, &uint_arg, sizeof(unsigned int)); //No. bytes

            int full_4096_chunks = int_arg / 4096; //Number of logical-address chunks' worth of data that we don't need.
            int remainder = int_arg % 4096; //Offset in the remaining piece.
            int section_number = 0; //Number of the section currently analysed.

            if((void*)-1 == mem || //No memory-mapped file
                (void*)-1 == shm || //No shared memory to write results to
                mem_size < 8) //File is so small it cannot possibly an SF file
            {
                //We cannot work at all without shared memory and memory mapped file
                string_arg = formatString("ERROR");
            }
            else {
                //The file is memory-mapped, check that it is a SF file; note that we only check the type for the section we are to read.
                char magic = *(char*)mem;
                int version = *(int*) ((char*)mem + 3); //Again, char* for traversal, int* for actual data.
                //if(4 > sizeof(int)) version = version | 0xffffffff; //Truncate to 8 x 4 = 32 bits, or 4 bytes; uncomment this line if you think that will be relevant
                short no_sections =  (short)( *((char*)mem + 7) ); //Take the one byte, THEN cast to short (once we're past the pointer matters)
                if(magic != 'z' || //Wrong magic
                    version < 114 ||  version > 198 || //Wrong version
                    no_sections < 5 || no_sections > 14 || //Wrong number of sections
                    uint_arg_2 > no_sections || uint_arg_2 < 0) //Wrong given section
                {
                    //Not a valid SF file
                    string_arg = formatString("ERROR");
                }
                else {
                    //It is a genuine SF file, as far as the header is considered.
                    //Now, we move through the section headers in order and see which one we're in.
                    while(full_4096_chunks >= 0) {
                        //While we still have sections to go over
                        char* sect_header = (char*)mem + 8 /*header size*/ + section_number * 27 /*sections to skip*/;

                        if(sect_header + 17 - (char*)mem /*distance in file, section header included*/ > mem_size) {
                            //The file runs out of section headers before we can complete the task
                            string_arg = formatString("ERROR");
                            break;
                        }

                        short sect_type = (short) (*(sect_header + 18));
                        int sect_offset = *(int*)(sect_header + 19);
                        int sect_size  = *(int*)(sect_header + 23);

                        if(21 != sect_type && 52 != sect_type) {
                            //Wrong section type; stop and announce the error
                            string_arg = formatString("ERROR");
                            break;
                        }
                            
                        int sect_chunks = (0 == sect_size % 4096) ? sect_size / 4096 : (sect_size / 4096) + 1; //Number of 4096-byte chunks that this section would occupy in the logical address space
                        if(full_4096_chunks >= sect_chunks) {
                            //This is a section that we skip
                            full_4096_chunks -= sect_chunks;
                            section_number++;
                        }
                        else {
                            //This is the target section; get our data from it (or fail) and we're done
                            int total_offset = sect_offset + remainder + 4096 * full_4096_chunks;
                            //From where the section starts, we skip the number of remaining chunks, if the section is so large, plus the remaining offset from the logical address

                            if(total_offset < 0 || total_offset + uint_arg > mem_size || total_offset + uint_arg > sect_offset + sect_size) {
                                //The data we are requsted is beyond where we can reach
                                string_arg = formatString("ERROR");
                            }
                            else {
                                //We can actually copy the data
                                for(int i = 0; i < uint_arg; ++i) {
                                    ((char*)shm)[i] = ((char*)mem)[total_offset + i];
                                }
                                string_arg = formatString("SUCCESS");
                            }
                            break;
                        }
                    }
                }

            }

            int_arg = -1337; //Release
            uint_arg = 0;
        }


        else if(0 == strcmp(function, "EXIT")) { //EXIT
            close(req_pipe);
            close(resp_pipe);
            unlink("RESP_PIPE_11629");
            if((void*)-1 != shm) {
                shmdt(shm);
                shmctl(shm_id, IPC_RMID, NULL);
            }
            if((void*)-1 != mem) munmap(mem, mem_size); //This actually happens automatically at exit, but eh.
            exit(0);
        }


        else {
            printf("We should not be here!!!\n");
            close(req_pipe);
            close(resp_pipe);
            unlink("RESP_PIPE_11629");
            if((void*)-1 != shm) {
                shmdt(shm);
                shmctl(shm_id, IPC_RMID, NULL);
            }
            if((void*)-1 != mem) munmap(mem, mem_size);
            exit(1);
        }

        //Write the actual results
        write(resp_pipe, task, 1 + (int)task[0]); //Command name
        if(NULL != string_arg) write(resp_pipe, string_arg, 1 + (int)string_arg[0]);
        if(-1337 != int_arg) write(resp_pipe, &int_arg, sizeof(int));

        //Free/reset the various variables
        free(function); function = NULL;
        free(string_arg); string_arg = NULL;
        int_arg = -1337;
        uint_arg = 0;
        uint_arg_2 = 0;
    }
}
