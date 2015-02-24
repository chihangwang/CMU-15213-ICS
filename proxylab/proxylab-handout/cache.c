#include "cache.h"
#include "csapp.h"
#include <string.h>

/* Based on the readers-writers problem in CSAPP textbook */
static int readcnt;
static sem_t mutex;
static sem_t w;  

static void age_cache(cache_head *cache);

void cache_init(cache_head *cache) {
	cache->total_object = 0;
	cache->total_size = 0;
	cache->head = NULL;
    readcnt = 0;
	sem_init(&mutex, 0, 1);
	sem_init(&w, 0, 1);
}

void cache_deinit(cache_head *cache) {
	cache_node *c = cache->head;
	cache_node *next;
	while(c != NULL) {
		free(c->tag);
		free(c->content);
		next = c->next;
		free(c);
		c = next;
	}
}

/* reader */
/* Return 1 if cache hit and buf and size will be filled. Otherwise, -1. */
int find_cache(cache_head *cache, char *uri, char *buf, int *size) {
	P(&mutex);
	readcnt ++;
	if(readcnt == 1)   /* First in */
		P(&w);
	V(&mutex);

	int rtn = 0;

	/* $Critical Section START */
	cache_node *c = cache->head;
	cache_node *return_node = NULL;
	while(c != NULL) {
		/* Check if cache hit */
		if(!strcmp(uri, c->tag)) {
			return_node = c;
            c->age = 0;
		}
		else{
			/* Aging the node if not be hit */
			c->age += 1; 
		}
		c = c->next;
	}
	/* cache miss */
	if(return_node == NULL) {
        printf("cache miss\n");
		rtn = -1;
	}
	/* cache hit */
	else {
        printf("cache hit\n");
		*size = return_node->size;
		memcpy(buf, return_node->content, return_node->size);
		rtn = 1;
	}
	/* $Critical Section END */

	P(&mutex);
	readcnt--;
	if(readcnt == 0)   /* Last out */
		V(&w);
	V(&mutex);

	return rtn;
}

/* writer */
void store_cache(cache_head *cache, char *uri, char *buf, int size) {
	P(&w);

	/* $Critical Section START */
	if(size <= MAX_OBJECT_SIZE) {
		if(cache->total_size + size > MAX_CACHE_SIZE) {
			/* eviction occurs */
			cache_node *c = cache->head;
			cache_node *evicted_node;
			int max_age = 0;
			int max_size = 0;

			/* find the node that is the oldest and then */
            /* the largest to be evicted. Evicting large cached object */
            /* can release more space for larger object. */
			while(c != NULL) {
				if(c->age >= max_age) {
					if(c->age == max_age) {
					    if(c->size > max_size) {
                        	max_size = c->size;
						    max_age = c->age;
						    evicted_node = c;
                        }
					}
                    else {
						max_size = c->size;
						max_age = c->age;
						evicted_node = c;
					}
				}
				c = c->next;
			}

			/* Check if the toal size after eviction is within MAX_CACHE_SIZE */
			int new_size = cache->total_size - evicted_node->size + size;
            
			if(new_size <= MAX_CACHE_SIZE){
				/* eviction begins */

				/* Update the evicted cache node */
				evicted_node->tag = realloc(evicted_node->tag, strlen(uri)+1);
				strcpy(evicted_node->tag, uri);

				evicted_node->content = realloc(evicted_node->content, size);
				memcpy(evicted_node->content, buf, size);

				evicted_node->size = size;

				/* Update the cache head */
				cache->total_size = new_size;

				/* Aging each node */
				age_cache(cache);
				evicted_node->age = 0;
			}
            else {
                printf("No Enough space for this object\n");
                printf("Eviction not occur !\n");
            }
		}
		else {
			/* simply store the current object in the head of the list */
			cache_node *node = malloc(sizeof(cache_node));
			node->tag = malloc(strlen(uri) + 1); /* +1 for '\0' */
			node->content = malloc(size);
			strcpy(node->tag, uri);
			memcpy(node->content, buf, size);
			node->size = size;
            /* Update the linked list */
			node->next = cache->head;
			cache->head = node;
			cache->total_size += size;
			cache->total_object += 1;

			/* Aging each node */
			age_cache(cache);
			node->age = 0;
		}
	}
	/* $Critical Section END */
	
	V(&w);
	return;
}

static void age_cache(cache_head *cache) {
	cache_node *c = cache->head;
	while(c != NULL) {
		c->age += 1;
		c = c->next;
	}	
}





