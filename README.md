SIMD Search
===========
Proof of concept for SIMD fast search in plain-text files, implemented using Intel's Advanced Vector Extensions (AVX). As for now, code only supports one characters, and only prints the number of lines with a match (like running *grep -c*).

Usage
-----
	avx-search -i [input_file] -m [match]
Only one character is supported. Input file must be a multiplication of the buffer size (1MB default).

Compile
-------
	gcc -O3 -std=c99 -mbmi -mavx2 -o avx-search avx-search.c
Tested with gcc 4.8.4

