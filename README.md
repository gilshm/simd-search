AVX Search
=========
Proof of concept for fast search in plain-text files using Intel's Advanced Vector Extensions (AVX) SIMD. As for now, code only supports one characters, and only prints the number of lines with a match (like running *grep -c*).
Performance is x2 compared to GNU Grep when data doesn't reside in the different caches, which is a reasonable assumption when dealing with Big Data for example. When caches are "hot" performance is somewhat the same (I thought the AVX implementation should be faster in both scenarios).

Usage
-----
	avx-search -i [input_file] -m [match]
Only one character is supported. Input file must be a multiplication of the buffer size (1MB default).

Compile
-------
	gcc -O3 -std=c99 -mbmi -mavx2 -o avx-search avx-search.c
Tested with gcc 4.8.4

