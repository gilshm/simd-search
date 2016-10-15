AVX Search
=========
Proof of concept for fast search in plain-text files using Intel's Advanced Vector Extensions (AVX). As for now, code only supports one or two characters, and only prints the number of lines with a match (like running *grep -c*).

Usage
-----
	avx-search -i [input_file] -m [match]
Only one and two match characters are supported.

Compile
-------
	gcc-6 -O3 -std=c99 -mbmi -mavx2 -o avx-search avx-search.c
Tested only with gcc 6.2.0

