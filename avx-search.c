/* gcc-6 -O3 -std=c99 -mbmi -mavx2 -o avx-search avx-search.c */

#include <immintrin.h>
#include <bmiintrin.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <fcntl.h>
#include <malloc.h>
#include <unistd.h>
#include <getopt.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>

#define BUF_SIZE (1024*1024)
#define VEC_SIZE 32
#define MEM_ALIGNMENT 32
#define BYTE_SIZE 8

void print_usage() {
	printf("-i --input\n");
	printf("-m --match\n");
}

int main(int argc, char *argv[]) {
	/* Command Line Arguments */

	char input[1024];
	char match[4];
	uint32_t match_length=0;
	int opt, long_index;

	input[0]='\0';

	static struct option long_options[] = {
		{"input",	required_argument, 0, 'i'},
		{"match",   required_argument, 0, 'm'}
	};

	while((opt = getopt_long(argc, argv, "i:m:", long_options, &long_index)) != -1) {
		switch(opt) {
			case 'i':
				strcpy(input, optarg);			
				break;
			case 'm':
				match_length = strlen(optarg);
				strcpy(match, optarg);
				break;
			default:
				print_usage();
		}
	}

	if(input[0]=='\0' || match_length==0 || match_length>2) {
		print_usage();
		return 1;
	}

	/* Search Algorithm */

	uint64_t counter=0;
	bool inc_counter = false;
	int ret_read;
	int fd = open(input, O_RDONLY);
	char* buf = (char*)memalign(MEM_ALIGNMENT, BUF_SIZE*sizeof(char));
	__m256i* mask = (__m256i*)memalign(MEM_ALIGNMENT, match_length*sizeof(__m256i));
	//char* aligned_buf = (char*)memalign(MEM_ALIGNMENT, VEC_SIZE*sizeof(char)); /* 256B buffer */
	//__m256i* aligned_buf = (__m256i*)memalign(MEM_ALIGNMENT, VEC_SIZE*sizeof(char)); /* 256B buffer */


	for(int i=0; i<match_length; i++) {
		for(int j=0; j<VEC_SIZE; j++) {
			*(((char*)mask)+j+i*VEC_SIZE) = match[i];
		}
	}

	__m256i mask1 = _mm256_set_epi8('h', 'h', 'h', 'h', 'h', 'h', 'h', 'h', 
									'h', 'h', 'h', 'h', 'h', 'h', 'h', 'h',
									'h', 'h', 'h', 'h', 'h', 'h', 'h', 'h',
									'h', 'h', 'h', 'h', 'h', 'h', 'h', 'h');
	
	__m256i mask2 = _mm256_set_epi8('a', 'a', 'a', 'a', 'a', 'a', 'a', 'a', 
									'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a',
									'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a',
									'a', 'a', 'a', 'a', 'a', 'a', 'a', 'a');
	
	__m256i mask_newline = _mm256_set_epi8('\n', '\n', '\n', '\n', '\n', '\n', '\n', '\n', 
										   '\n', '\n', '\n', '\n', '\n', '\n', '\n', '\n',
									       '\n', '\n', '\n', '\n', '\n', '\n', '\n', '\n',
									       '\n', '\n', '\n', '\n', '\n', '\n', '\n', '\n');


	while((ret_read = read(fd, buf, BUF_SIZE)) > 0) {
		__m256i *p = (__m256i*)buf;

		uint32_t x_next=-1;		
		
		for(int i=0; i<BUF_SIZE/VEC_SIZE; i++) {
			__m256i target = _mm256_load_si256(&p[i]);
			__m256i result = _mm256_cmpeq_epi8(target, mask[0]);
			__m256i result2 = _mm256_cmpeq_epi8(target, mask[1]);
			__m256i result_newline = _mm256_cmpeq_epi8(target, mask_newline);

			uint64_t *v, *v_nl, *v2;
			uint64_t x, x_nl, x2;

			for(int i=0; i<4; i++) {
				bool new_vec = true;	
				
				v = (uint64_t*)(&result)+i;
				v2 = (uint64_t*)(&result2)+i;
				v_nl = (uint64_t*)(&result_newline)+i;
				
				while(1==1) {
					x = _tzcnt_u64(*v)/BYTE_SIZE;
					x_nl = _tzcnt_u64(*v_nl)/BYTE_SIZE;

					if(x_next!=-1 && inc_counter==false && match_length==2) {
						//printf("%u\n", *((char*)v2)+x_next);
						//printf("%u\n", *((char*)v2+1));
						if(*((char*)v2+x_next) == -1) {
							counter++;
							inc_counter=true;
						}
					}

					if(x==8) { /* Character was not found */
						if(x_nl!=8) inc_counter=false; /* If there is a newline we can start counting again */
						if(new_vec) x_next=-1;

						break;
					}

					//printf("%" PRIu64 " ", x);
					if(inc_counter==false) {
						x_next = (x+1)%BYTE_SIZE;

						if(match_length==1) {
							counter++;
							inc_counter=true;
						}
					}

					if(x_nl<x && x_nl!=8) {
						inc_counter=false;
						*v_nl=*v_nl & ~((uint64_t)255<<(x_nl*8));
					}
	
					//if(x==7) break; /* Character was found in the last position, no need to do another iteration */
	
					*v=*v & ~((uint64_t)255<<(x*8));

					new_vec = false;
					//for(int j=0; j<8; j++)
					//	*v = _blsr_u64(*v);
				}
					//printf("%" PRIu64 "\n", counter);
				//printf("\n");
			}
			//printf("\n");
/*
			char* f = (char*)&result;
			char* f2 = (char*)&result_newline;
			for(int i=0; i<32; i++) {

				if(f[i]==-1 && inc_counter==false) {
					counter++;
					inc_counter = true;
				}

				if(f2[i]==-1) {
					inc_counter = false;
				}
			}
*/
		}
	}

	printf("%" PRIu64 "\n", counter);

/*

	__m256i target = _mm256_set_epi8('l', 'i', 'g', 3, 4, 5, 6, 7,
									 8, 9, 10, 11, 12, 13, 14, 15,
								     16, 17, 18, 19, 20, 21, 22, 23,
									 24, 25, 26, 27, 28, 29, 30, 31);

	__m256i mask1 = _mm256_set_epi8('g', 'g', 'g', 1, 1, 1, 1, 1,
				     			    1, 1, 1, 1, 1, 1, 1, 1,
								    1, 1, 1, 1, 1, 1, 1, 1,
								    1, 1, 1, 1, 1, 1, 1, 1);
	
	__m256i mask2 = _mm256_set_epi8('i', 'i', 'i', 1, 1, 1, 1, 1,
				     			    1, 1, 1, 1, 1, 1, 1, 1,
								    1, 1, 1, 1, 1, 1, 1, 1,
								    1, 1, 1, 1, 1, 1, 1, 1);
	
	__m256i mask3 = _mm256_set_epi8('l', 'l', 'l', 1, 1, 1, 1, 1,
				     			    1, 1, 1, 1, 1, 1, 1, 1,
								    1, 1, 1, 1, 1, 1, 1, 1,
								    1, 1, 1, 1, 1, 1, 1, 1);

	//__m256i result = _mm256_and_si256(target, mask);

	__m256i result = _mm256_cmpeq_epi8(target, mask1);

	char* f = (char*)&result;
	for(int i=0; i<32; i++) {
		if(f[i]==-1)
			printf("%d\n", i);
	}

	result = _mm256_cmpeq_epi8(target, mask2);
	for(int i=0; i<32; i++) {
		if(f[i]==-1)
			printf("%d\n", i);
	}


	result = _mm256_cmpeq_epi8(target, mask3);
	for(int i=0; i<32; i++) {
		if(f[i]==-1)
			printf("%d\n", i);
	}


	//printf("whitespace %d\n", ' ');
*/

	return 0;
}
