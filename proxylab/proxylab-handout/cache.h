#ifndef _CACHE_H_
#define _CACHE_H_

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

typedef struct cache_node{
	char *tag;     /* treat uri as cache tag */
	char *content; /* content of the current cached object */
	int age;       /* age for eviction */
	int size;      /* size of the current cached object */
	struct cache_node *next; /* linked list pointer */
} cache_node;

typedef struct {
	int total_object; /* total objects in cache */
	int total_size;   /* total cached objects' size */
	cache_node *head; /* head of the linked list */
} cache_head;

void cache_init(cache_head *cache);
void cache_deinit(cache_head *cache);
int  find_cache(cache_head *cache, char *uri, char *buf, int *size);
void store_cache(cache_head *cache, char *uri, char *buf, int size);
//static void age_cache(cache_head *cache);

#endif
