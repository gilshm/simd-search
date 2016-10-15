/* gcc-6 -O3 -std=c99 -mbmi -mavx2 -o avx1 avx1.c */

#include <immintrin.h>
#include <bmiintrin.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <fcntl.h>
#include <malloc.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/stat.h>

#define BUF_SIZE (1024*1024)
#define VEC_SIZE 32
#define MEM_ALIGNMENT 32
#define BYTE_SIZE 8

int main() {

	uint64_t counter=0;
	bool inc_counter = false;
	int ret_read;
	int fd = open("/mnt/p3700/gilsho/INPUT_FILE_10GB", O_RDONLY);
	char* buf = (char*)memalign(MEM_ALIGNMENT, BUF_SIZE*sizeof(char)); 
	__m256i* aligned_buf = (__m256i*)memalign(MEM_ALIGNMENT, VEC_SIZE*sizeof(char)); /* 256B buffer */


	__m256i mask1 = _mm256_set_epi8('h', 'h', 'h', 'h', 'h', 'h', 'h', 'h', 
									'h', 'h', 'h', 'h', 'h', 'h', 'h', 'h',
									'h', 'h', 'h', 'h', 'h', 'h', 'h', 'h',
									'h', 'h', 'h', 'h', 'h', 'h', 'h', 'h');
	
	__m256i mask2 = _mm256_set_epi8('z', 'z', 'z', 'z', 'z', 'z', 'z', 'z', 
									'z', 'z', 'z', 'z', 'z', 'z', 'z', 'z',
									'z', 'z', 'z', 'z', 'z', 'z', 'z', 'z',
									'z', 'z', 'z', 'z', 'z', 'z', 'z', 'z');
	
	__m256i mask_newline = _mm256_set_epi8('\n', '\n', '\n', '\n', '\n', '\n', '\n', '\n', 
										   '\n', '\n', '\n', '\n', '\n', '\n', '\n', '\n',
									       '\n', '\n', '\n', '\n', '\n', '\n', '\n', '\n',
									       '\n', '\n', '\n', '\n', '\n', '\n', '\n', '\n');


	while((ret_read = read(fd, buf, BUF_SIZE)) > 0) {
		__m256i* p = (__m256i*)buf;

		for(int i=0; i<BUF_SIZE/VEC_SIZE; i++) {
			__m256i target = _mm256_load_si256(&p[i]);
			__m256i result = _mm256_cmpeq_epi8(target, mask1);
			__m256i result_newline = _mm256_cmpeq_epi8(target, mask_newline);

			uint64_t *v, *v_nl;
			uint64_t x, x_nl;
			
			for(int i=0; i<4; i++) {
				v = (uint64_t*)(&result)+i;
				v_nl = (uint64_t*)(&result_newline)+i;
				
				while(1==1) {
					x = _tzcnt_u64(*v)/BYTE_SIZE;
					x_nl = _tzcnt_u64(*v_nl)/BYTE_SIZE;

					if(x==8) { /* Character was not found */
						if(x_nl!=8) inc_counter=false; /* If there is a newline we can start counting again */						

						break;
					}

					if(inc_counter==false) {
						counter++;
						inc_counter=true;
					}

					if(x_nl<x && x_nl!=8) {
						inc_counter=false;
						*v_nl=*v_nl & ~((uint64_t)255<<(x_nl*8));
					}
	
	
					*v=*v & ~((uint64_t)255<<(x*8));

				}
			}
		}
	}

	printf("%" PRIu64 "\n", counter);

	return 0;
}
