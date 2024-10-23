# OS Linux C Projects
This repository contains university laboratory assignments for the Operating Systems course of 2016.
The .c files are the exact solutions I submitted at the time, retrieved from the university's Moodle site. All submissions received a score of 100/100 points.
The original task descriptions, however, have been lost (they had been sent individually to each student and never stored on Moodle, and I sadly no longer have the e-mails that contained them), therefore I have added for each assignment a README file that attempts to approximate the task descriptions.
My task descriptions are likely nowhere near as comprehensive or precise as the originals were, but I hope they will do a sufficient job of explaining the assignment.

## Assignment 1 - Files and Directories (File System Module)
An assignment centred around traversing the file system, listing directory contents, and parsing files.
A special type of file called an SF file is defined, and the student must scan its specific structure and efficiently extract data from it.

## Assignment 2 - Threads and Processes (Processes, Threads and Synchronization Module)
An assignment centred around coordinating processes and threads so they execute in specific orders.
Implemented with `fork()`, pthread, and semaphores.

## Assignment 3 - Pipes and Shared Memory (Inter-Process Communication)
An assignment centred around communication between processes using named pipes and shared memory.
Unlike previous assignments, this assignment features a back-and-forth communication between program and tester, such that the tester can request a sequence of consecutive instructions.