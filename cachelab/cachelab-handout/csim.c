/*
 * Name: Chih-Ang Wang  
 * Andrew ID: chihangw
 */
#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <ctype.h>
#include "cachelab.h"

// structure for per cache line
typedef struct cache_line{
	int tag;
	int age;
} line;

int main(int argc, char *argv[]) {

	int s, E, b;           // cache convention in CSAPP2
	int verbose = 0;       // flag for verbose mode
	char *path = NULL;     // path to the tracefile
	char memory_addr[16];  // memory address in tracefile
	char instruction;      // instruction in tracefile
	int i, j, opt, set_size, line_size, 
		hit_count = 0, miss_count = 0, evict_count = 0;
	long cache_set, cache_tag; // set # and tag # of the address 

	// obtain the command line arguments
	while((opt = getopt(argc, argv, "vs:E:b:t:")) != -1){

		switch(opt){
        		
			case 's':
				s = atoi(optarg);
				break;
			case 'E':
				E = atoi(optarg);
				break;
			case 'b':
				b = atoi(optarg);
				break;
			case 'v':
				verbose = 1;
				break;
			case 't':
				path = optarg;
				break;
			case '?':  /* '?' */
				printf("./csim [-v] -s <s> -E <E> -b <b> -t\
				 <tracefile>");
				return 1;
		}
	}

	// obtain the # of sets and lines of the specified cache
	set_size = 1 << s;  // set_size = 2^s
	line_size = E;      // line_size = E 

	// allocate two dimensional arrays to represent 
	// M sets, N lines cache structure.
	line **cache = (line**) malloc( sizeof(line*) * set_size );
	if(cache == NULL){
		fprintf(stderr, "malloc fail!");
		exit(1);
	}
	for(i=0; i<set_size; i++){
		cache[i] = (line*) malloc( sizeof(line) * line_size );
		if(cache[i] == NULL){
			fprintf(stderr, "malloc fail!");
			exit(1);
		}
	}

	// initialize the empty cache by setting tag and age field to -1.
	for(i=0; i<set_size; i++)
		for(j=0; j<line_size; j++){
			cache[i][j].tag = -1;
			cache[i][j].age = 0;
		}
	// open the tracefile with read mode
	FILE *fp = NULL; 
	char *trance_line = NULL;
	size_t line_length = 0;

	if((fp = fopen(path, "r")) == NULL){
		fprintf(stderr, "%s cannot open %s\n", argv[0], path); 
		exit(1);	
	}
	// read each line from tracefile
	while(getline(&trance_line, &line_length, fp) != -1){
		// for instruction L, M, S
		if(isspace(trance_line[0])){

			j = 0; // reinitialize the memory address counter

			instruction = trance_line[1];
			
			i = 3; // memory address starts at line+3 for each line in tracefile
			while(trance_line[i] != ','){
				memory_addr[j++] = trance_line[i++];
			}

			while(isdigit(trance_line[++i]))
				trance_line[++i] = '\0';	
			// manually add '\0' to the end of char *memory_addr
			memory_addr[j] = '\0'; 

			// convert char* address into unsigned long number of base 16
			char *pEnd;
			unsigned long addr = strtol(memory_addr, &pEnd, 16);
			
			unsigned long mask = ~0;
			int t = 64 - (s+b);
			unsigned long tag_mask = (mask >> (s+b)) << (s+b);
			unsigned long set_mask = ((mask << t) >> (t+b)) << b;
			// extract set # and tag from the address
			cache_tag = (long)((addr & tag_mask) >> (s+b));
			cache_set = (long)((addr & set_mask) >> b);

			int is_cache_hit = 0, has_space = 0, evicted_line = 0,
				max_age = 0, index = 0;
			switch(instruction){

				case 'L':
					// check cache if it contains the object
					max_age = cache[cache_set][0].age;
					for(i=0; i<line_size; i++){
						line l = cache[cache_set][i];
						// if tag match
						if(l.tag == cache_tag){
							is_cache_hit = 1;
							index = i;
						}
						// oldest age in the set
						if(l.age > max_age)
							max_age = l.age;
					}
					// if load hit
					if(is_cache_hit){
						if(verbose)
							printf("%s hit\n", trance_line);
						cache[cache_set][index].age = max_age + 1;
						hit_count++;
					// if load miss
					}else{
						// check if there's empty room in cache, 
						// if not, evicted by LRU
						int lru_index = cache[cache_set][0].age;
						for(i=0; i<line_size; i++){
							if(cache[cache_set][i].tag == -1){
								has_space = 1;
								j = i;
								break;
							}
							if(cache[cache_set][i].age < lru_index){
								evicted_line = i;
								lru_index = cache[cache_set][i].age;
							}
						}
						// there's space to store the object	
						if(has_space){
							if(verbose)
								printf("%s miss\n", trance_line);
							cache[cache_set][j].tag = cache_tag;
							cache[cache_set][j].age = max_age + 1;
							miss_count++;
						// there isn't space, evict the oldest one	
						}else{
							if(verbose)
								printf("%s miss eviction\n", trance_line);
							cache[cache_set][evicted_line].tag = cache_tag;
							cache[cache_set][evicted_line].age = max_age + 1;
							miss_count++;
							evict_count++;
						}
					}
					break;

				case 'M':
					// check cache if it contains the object
					max_age = cache[cache_set][0].age;
					for(i=0; i<line_size; i++){
						line l = cache[cache_set][i];
						// if tag match
						if(l.tag == cache_tag){
							is_cache_hit = 1;
							index = i;
						}
						// oldest age in the set
						if(l.age > max_age)
							max_age = l.age;
					}
					// if load hit <hit hit>
					if(is_cache_hit){
						if(verbose)
							printf("%s hit hit\n", trance_line);
						cache[cache_set][index].age = max_age + 1;
						hit_count += 2;
					// if load miss <miss hit> or <miss eviction hit>
					}else{
						// check if there's empty room in cache, 
						// if not, evicted by LRU
						int lru_index = cache[cache_set][0].age;
						for(i=0; i<line_size; i++){
							if(cache[cache_set][i].tag == -1){
								has_space = 1;
								j = i;
								break;
							}
							if(cache[cache_set][i].age < lru_index){
								evicted_line = i;
								lru_index = cache[cache_set][i].age;
							}
						}
						// there's space to store the object	
						if(has_space){
							if(verbose)
								printf("%s miss hit\n", trance_line);
							cache[cache_set][j].tag = cache_tag;
							cache[cache_set][j].age = max_age + 1;
							miss_count++;
							hit_count++;
						// there isn't space, evict the oldest one	
						}else{
							if(verbose)
								printf("%s miss eviction hit\n", trance_line);
							cache[cache_set][evicted_line].tag = cache_tag;
							cache[cache_set][evicted_line].age = max_age + 1;
							miss_count++;
							evict_count++;
							hit_count++;
						}
					}

					break;

				case 'S':
					// check cache if it contains the object
					max_age = cache[cache_set][0].age;
					for(i=0; i<line_size; i++){
						line l = cache[cache_set][i];
						// if tag match
						if(l.tag == cache_tag){
							is_cache_hit = 1;
							index = i;
						}
						// oldest age in the set
						if(l.age > max_age)
							max_age = l.age;
					}
					// if load hit
					if(is_cache_hit){
						if(verbose)
							printf("%s hit\n", trance_line);
						cache[cache_set][index].age = max_age + 1;
						hit_count++;
					// if load miss
					}else{
						// check if there's empty room in cache, 
						// if not, evicted by LRU
						int lru_index = cache[cache_set][0].age;
						for(i=0; i<line_size; i++){
							if(cache[cache_set][i].tag == -1){
								has_space = 1;
								j = i;
								break;
							}
							if(cache[cache_set][i].age < lru_index){
								evicted_line = i;
								lru_index = cache[cache_set][i].age;
							}
						}
						// there's space to store the object	
						if(has_space){
							if(verbose)
								printf("%s miss\n", trance_line);
							cache[cache_set][j].tag = cache_tag;
							cache[cache_set][j].age = max_age + 1;
							miss_count++;
						// there isn't space, evict the oldest one	
						}else{
							if(verbose)
								printf("%s miss eviction\n", trance_line);
							cache[cache_set][evicted_line].tag = cache_tag;
							cache[cache_set][evicted_line].age = max_age + 1;
							miss_count++;
							evict_count++;
						}
					}
					break;

			}

		}else{ // for instruction I, ignore it.
			continue;
		}
	}

	fclose(fp);
	free(cache);

	printSummary(hit_count, miss_count, evict_count);

	return 0;
}
