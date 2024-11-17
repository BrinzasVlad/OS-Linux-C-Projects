# Assignment 2 - Processes, Threads and Synchronization Module
This is the second assignment for the 2016 Operating Systems course.
This code in particular was submitted in early May of 2016 and scored full points.
The original task description (and helper tester program) has since been lost, but below is an approximate description of it.

## Task Description
You are to write a program in C which will be executed on a Linux machine.
Your program will be compiled and called by the tester.py program, which will then analyse its execution.
Throughout the course of this assignment, you will need to create and coordinate a number of processes and threads. Your program should include the `a2_helper.h` header file, which contains the `info(status, process_id, thread_id)` function. Your program should call this function as follows:
- when a new process starts executing, it should call `info(BEGIN, <process_id>, 0)`
- when a new thread starts executing, it should call `info(BEGIN, <parent_process_id>, <thread_id>)`
- when a process terminates, it should call `info(END, <process_id>, 0)`
- when a thread terminates, it should call `info(END, <parent_process_id>, <thread_id>)`
In any process, the main thread counts as thread id 0, and subthreads ids start from 1. The original process (the one that is launched when your program is run) has process id 1.

## Restrictions

### Process Tree
Your program must create the following process tree. The original process is P1.
```
P1
├── P2
│   ├── P3
│   ├── P4
│   │   └── P5
│   ├── P6
│   └── P7
└── P8
```
All processes must wait until all of their child processes have terminated before terminating themselves (so, e.g., P1 must wait for P2 and P8 before terminating).

### Sub-threads
- Process P6 must create four threads in total, T6.1 to T6.4
- Process P7 must create fourty-one threads in total, T7.1 to T7.41
- Process P8 must create four threads in total, T8.1 to T8.4

### Timing restrictions
- All processes must wait until all of their child processes have terminated before they can terminate
- All processes must wait until all of their subthreads have terminated before they can terminate
- T8.2 may not start executing before T6.4 terminates
- T6.4 may not start executing before T8.1 terminates
- T6.3 may not start executing before T6.2 starts executing
- T6.3 must terminate before T6.2 may terminate
- There may be AT MOST six P7 treads active at any given time
- T7.10 may only terminate when there are EXACTLY five other P7 threads active
(Note that you DO need T7.10 to terminate in order to complete the task.)
