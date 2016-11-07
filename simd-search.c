/* gcc -O3 -std=c99 -mbmi -mavx2 -o simd-search simd-search.c */

#include <immintrin.h>
#include <bmiintrin.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <fcntl.h>
#include <malloc.h>
#include <unistd.h>
#include <stdbool.h>
#include <getopt.h>
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
        {"input",   required_argument, 0, 'i'},
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

    if(input[0]=='\0' || match_length==0 || match_length>1) {
        print_usage();
        return 1;
    }

	/* Search Algorithm */

	uint64_t counter=0;
	bool inc_counter=false;
	ssize_t ret_read;
	int fd = open(input, O_RDONLY);
	char* buf = (char*)memalign(MEM_ALIGNMENT, BUF_SIZE*sizeof(char)); 

	__m256i mask1 = _mm256_set1_epi8(match[0]); 
	__m256i mask_newline = _mm256_set1_epi8('\n'); 

	while((ret_read = read(fd, buf, BUF_SIZE)) > 0) {
		__m256i* p_buf = (__m256i*)buf;

		for(int i=0; i<BUF_SIZE/VEC_SIZE; i++) {
			__m256i cmp_res = _mm256_cmpeq_epi8(p_buf[i], mask1);
			__m256i cmp_nl_res = _mm256_cmpeq_epi8(p_buf[i], mask_newline);
			uint32_t reduced_bitmap = _mm256_movemask_epi8(cmp_res);
			uint32_t reduced_nl_bitmap = _mm256_movemask_epi8(cmp_nl_res);

			uint32_t x, x_nl;
			
			while(1==1) {
				x = _tzcnt_u32(reduced_bitmap);	
				x_nl = _tzcnt_u32(reduced_nl_bitmap);

				if(x==32) { /* Character was not found */
					if(x_nl!=32) inc_counter = false; /* If there is a newline we can start counting again */
					break;
				}

				if(x_nl<x && x_nl!=32) {
					inc_counter=false;
					reduced_nl_bitmap = reduced_nl_bitmap & ~((uint32_t)1<<(x_nl));
					continue;
				}

				if(inc_counter==false) {
					counter++;
					inc_counter=true;
				}

				reduced_bitmap = reduced_bitmap & ~((uint32_t)1<<(x));
			}
		}
	}

	printf("%" PRIu64 "\n", counter);

	return 0;
}
