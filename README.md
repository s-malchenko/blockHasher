# blockHasher
Console utility for block hashing files.

### What it does
This utility reads a block of data from input file and calculates its MD5 signature. New line with each calculated signature is appended to the output file as text. Sources for MD5 hashing were taken here: https://github.com/ulwanski/md5.

### Settings
1. Block size can be customized, default is 1 MB.
1. Single-threaded or multi-threaded mode is available. Maximum threads number can be customized, default is 4.
1. This program always measures the time of its work and prints it to the console.

You can call blockHasher without any parameters to read a short manual:
```
Usage: blockHasher <file to hash> <output file>
       [-b <block size in bytes, default is 1 MB>]
       [-m [threads count, default is 4]
```
Example:
```
blockHasher input.zip signature.txt -m
```
will proceed file input.zip by blocks of 1 MB in maximum of 4 threads.
