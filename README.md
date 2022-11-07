# Index Allocated Disk Simulator

---

## Description

An Index Allocated Disk Written in C++

## Main Functions

1- listAll - lists all the disk content
2- fsFormat - formats the disk depending on the blocksize
3- CreateFile - create a new file and opens it.
4- OpenFile - opens a closed file.
5- CloseFile - closes an open file.
6- WriteToFile - writes data on the disk
7- DelFile - deletes a file
8- ReadFromFile - reads data from file
9- writer - takes input and block details and writes to that area
10- findEmptyBlock - finds the first empty block in BitVector
11- findExistingBlock - finds what block data is written to according to the index block and the block number.
12- toChar - converts int to char
13- toInt - converts char to int
14- ceilVal - divides and rounds up

## How to run

To compile:
in a linux/unix terminal run: `g++ indexSim.cpp -o indexSim`
a compiled file will be created with the name proxy.

To run:
in a linux/unix terminal run: `./indexSim`
