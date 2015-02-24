#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "csapp.h"
#include "sbuf.h"
#include "cache.h"

#define NTHREADS 4
#define SBUFSIZE 100
#define NHEADERS 40

sbuf_t sbuf; /* Shared buffer for connected descriptors */
cache_head cache; /* Shared cache for all worker threads */

/* Because the proxy may encounter errors at any time when there's a broken  */
/* socket (ex. SIGPIPE, EPIPE, ECONNRESET), we must keep track of every      */
/* thread's context in order to prevent from memory leak (ex: close(fd)).    */
typedef struct {
	pthread_t tid;
	jmp_buf read_env;   /* ECONNRESET */
	jmp_buf write_env;  /* EPIPE */
	jmp_buf pipe_env;   /* SIGPIPE */
} t_context;

/* Array to keep track of each thread's context */
t_context thread_context[NTHREADS];

/* You won't lose style points for including these long lines in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3)\
									 Gecko/20120305 Firefox/10.0.3\r\n";
static const char *accept_hdr = "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
static const char *accept_encoding_hdr = "Accept-Encoding: gzip, deflate\r\n";
static const char *connection_hdr = "Connection: close\r\n";
static const char *proxy_connection_hdr = "Proxy-Connection: close\r\n";

void *thread(void *vargp);
void serve_client(int connfd);
void sigpipe_handler(int sig);
int  get_thread_index(pthread_t tid);
int  parse_request(rio_t *rio, int fd, char *method, char *uri, char *version);
int  parse_uri(char *uri, char *hostname, char *pathname, int *port);
int  parse_headers(rio_t *rio, char headers[NHEADERS][MAXLINE], int *n);
void make_request(char *path, char headers[NHEADERS][MAXLINE], 
				  int n_header, int connfd, int clientfd);
void rio_writen_s(int fd, void *usrbuf, size_t n);
ssize_t rio_readlineb_s(rio_t *rio, void *usrbuf, size_t maxlen);
ssize_t rio_readnb_s(rio_t *rio, void *usrbuf, size_t n);
void client_error(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg);

int main(int argc, char **argv)
{
	int listenfd, connfd, port;
    long i;
    struct sockaddr_in clientaddr;
    socklen_t clientlen = sizeof(struct sockaddr_in);
    pthread_t tid;

    /* Check command line args */
    if (argc != 2) {
    	fprintf(stderr, "usage: %s <port>\n", argv[0]);
    	exit(1);
    }
    port = atoi(argv[1]);

    /* initialize shared buffer for connected descriptor */
    sbuf_init(&sbuf, SBUFSIZE);

    /* initialize shared cache for all worker threads */
    cache_init(&cache);

    /* Install the handler for SIGPIPE */
    Signal(SIGPIPE, sigpipe_handler);

    listenfd = Open_listenfd(port);
    printf("Proxy starts running on port %d\n", port);

   	/* Create worker threads */
   	printf("create worker threads...\n");
	for(i=0; i<NTHREADS; i++) {
		Pthread_create(&tid, NULL, thread, (void *)i);
	}    

	while(1) {
		connfd = Accept(listenfd, (SA *) &clientaddr, ((socklen_t *) &clientlen));
		sbuf_insert(&sbuf, connfd);
	}

	/* never should be here */
	printf("server stops...\n");

    cache_deinit(&cache);
	Close(listenfd);
    return 0;
}

void *thread(void *vargp) {
	Pthread_detach(pthread_self());
	long i = (long)vargp; /* vargp is 8 bytes long */
	thread_context[i].tid = pthread_self();
	printf("Worker thread [%ld] is running\n\n", i);
	while(1) {
		int connfd = sbuf_remove(&sbuf);
		printf("Worker thread [%ld] serves connfd[%d]\n", i, connfd);
        serve_client(connfd);
        printf("Worker thread [%ld] closes connfd[%d]\n\n", i, connfd);
		Close(connfd);
	}
	/* never should be here */
	printf("a thread dies...\n");
	return NULL;
}

/* Serve the connected client fd with following steps:                        */
/* 1. Parse the request(method, url, HTTP version, HEADERs)                   */
/* 2. If the web object has been cached by URL, return it directly.           */
/* 3. Otherwise, Forward request to the remote server on behalf of the client */
/* 4. Pass the received response from the remote server to the client         */
/* 5. and store the reponse in cache with Tag(uri)                            */
void serve_client(int connfd) {
	char method[MAXLINE], uri[MAXLINE], version[MAXLINE], buf[MAXLINE];
	char hostname[MAXLINE], path[MAXLINE];
	char headers[NHEADERS][MAXLINE];
	int  n_header, port = 80;

	rio_t rio_c; /* rio_client */ 
	rio_t rio_s; /* rio_remote_server */

	rio_readinitb(&rio_c, connfd);

	/* for read/write function before clientfd is created. */
	int t_index = get_thread_index(pthread_self());
	if(t_index < 0) {
		printf("[ERROR] cannot not find thread index\n");
		return;
	}
	/* Store the current context for ECONNRESET */
    if (setjmp(thread_context[t_index].read_env) != 0) {  
        return; 
    }
    /* Store the current context for EPIPE */
    if (setjmp(thread_context[t_index].write_env) != 0) {  
        return; 
    }
    /* Store the current context for SIGPIPE */
    if (sigsetjmp(thread_context[t_index].pipe_env, 1) != 0) { 
        /* may jmp here cause clientfd create failure */
        return; 
    }

    /* Parse incoming requests and headers. Extract method, uri, version */
	if(parse_request(&rio_c, connfd, method, uri, version) < 0) {
		printf("Error in parse_request()\n");
		return;
	}

    if(parse_uri(uri, hostname, path, &port) < 0) {
    	client_error(connfd, uri, "400", "Bad Request", "Bad URL");
    	return;
    }

    if(parse_headers(&rio_c, headers, &n_header) < 0) {
    	client_error(connfd, uri, "400", "Bad Request", "Bad header");
    	return ;
    }

    int  byteread = 0;
   	char cache_buf[MAX_OBJECT_SIZE];

    /* Find the object in cache. Return the object directly */
    if(find_cache(&cache, uri, cache_buf, &byteread) == 1) {

    	rio_writen_s(connfd, cache_buf, byteread);

    }
    /* Object not found. Make requests to remote server and cache the response */
    else {

    	char *c_buf = cache_buf;
    	int  object_size = 0;

        int clientfd = open_clientfd_r(hostname, port);
        if (clientfd < 0) {
            client_error(connfd, "", "1000", "DNS failed", "DNS failed");
            return;
        }

        /* After clientfd is created, we should close it to prevent from   */
        /* memory leak when the socket is broken and causes read()/write() */
        /* fail. Therefore, we set jump point here to close fd to deal with*/
        /* any future ERROR */

	    /* Store the current context for ECONNRESET */
        if (setjmp(thread_context[t_index].read_env) != 0) {
        	if(clientfd > 0)
        		Close(clientfd);  
            return; 
        }
        /* Store the current context for EPIPE */
        if (setjmp(thread_context[t_index].write_env) != 0) {  
        	if(clientfd > 0)
        		Close(clientfd);
            return; 
        }
        /* Store the current context for SIGPIPE */
        if (sigsetjmp(thread_context[t_index].pipe_env, 1) != 0) { 
        	if(clientfd > 0)
        		Close(clientfd);
            return; 
        }

	    rio_readinitb(&rio_s, clientfd);

        /* Send request to the remote server on behalf of the client */
	    make_request(path, headers, n_header, connfd, clientfd);

        /* Pass the response from the remote server to the client */
	    while ((byteread = rio_readnb_s(&rio_s, buf, MAXLINE))) {
	    	object_size += byteread;
	    	if(object_size <= MAX_OBJECT_SIZE) {
	    		memcpy(c_buf, buf, byteread);
	    		c_buf += byteread;
	    	}
	        rio_writen_s(connfd, buf, byteread);
	    }

        /* Cache the response with URL as Tag for future requests */
	    if(object_size <= MAX_OBJECT_SIZE) {
	    	store_cache(&cache, uri, cache_buf, object_size);
        }

        Close(clientfd);
    }

    return;
}

/*
 *	Helper functions Starts
 */
void sigpipe_handler(int sig) {
	printf("SIGPIPE caught.\n");
	int i = get_thread_index(pthread_self());
	siglongjmp(thread_context[i].pipe_env, -1);
}

/* Return current thread's index in t_context array. -1 if not found */
int get_thread_index(pthread_t tid) {
	int i;
	for(i=0; i<NTHREADS; i++) {
		if(thread_context[i].tid == tid)
			return i;
	}
	return -1;
}

/* Return 1 if successful, -1 if failed */
int parse_request(rio_t *rio, int fd, char *method, char *uri, char *version) {
    char buf[MAXLINE];
	rio_readlineb_s(rio, buf, MAXLINE);
	sscanf(buf, "%s %s %s", method, uri, version);

	/* check METHOD, URI, VERSION from the request */
    if(strcasecmp(method, "GET")) { 
        client_error(fd, method, "501", "Not Implemented",
                    "Proxy does not implement this method");
        return -1;
    }
    if(strlen(uri) == 0)  {
    	client_error(fd, uri, "400", "Bad Request", "Missing uri");
    	return -1;
    }
    if(strcasecmp(version, "HTTP/1.0") && strcasecmp(version, "HTTP/1.1")) {
    	client_error(fd, version, "400", "Bad Request", "Version Illegal");
    	return -1;
    }
    return 1;
 }

/* Return 1 if successful, -1 if failed */
/* Parse argument uri and store the hostname, pathname and port */
/* in the passed-in address */
int parse_uri(char *uri, char *hostname, char *pathname, int *port) {
    char *str_start = NULL;
    char *str_end = NULL;
    int  str_len  = 0;

    // Parse hostname
    if(strncasecmp(uri, "http://", 1)){
        printf("Bad URL. Cannot GET hostname\n");
        return -1;
    }
   
    str_start = uri + 7;
    str_end = strpbrk(str_start, " :/");
    if(str_end == NULL) {
        str_end = str_start + strlen(str_start);
        *str_end = '/';
        *(str_end + 1) = '\0';
    }
    str_len = str_end - str_start; 
    strncpy(hostname, str_start, str_len);
    *(hostname + str_len) = '\0';

    // Parse port 
    str_start += str_len;
    *port = 80;
    if(!strncmp(str_start, ":", 1)) {
        str_end = strchr(str_start, '/');
        *str_end = '\0';
        *port = atoi(str_start + 1);
        *str_end = '/';
        str_start = str_end;
    }

    // Parse pathname 
    if(!strncmp(str_start, "/", 1)) {
        str_len = strlen(str_start);
        strcpy(pathname, str_start);
    }
    
    return 1;
}

/* Return 1 if successful, -1 if failed */
int parse_headers(rio_t *rio, char headers[NHEADERS][MAXLINE], int *n) {
	char buf[MAXLINE], key[MAXLINE];
	char *tok;
	int i=0;

	rio_readlineb_s(rio, buf, MAXLINE);

	while(strcmp(buf, "\r\n") && strcmp(buf, "\n")) {
		tok = strchr(buf, ':');
		
		if (!tok) {
            return -1; // not key-value pair
        } else {
            *tok = '\0';
            strcpy(key, buf);

            if(!strcmp(key, "Host")) {
            	*tok = ':';
            	if(i == NHEADERS) {
            		printf("Too many headers\n");
            		return -1;
            	}
            	strcpy(headers[i++], buf);
            	*n = i;
            }
            else if(!strcmp(key, "User-Agent")){} 
            else if(!strcmp(key, "Accept")){}
            else if(!strcmp(key, "Accept-Encoding")){}
            else if(!strcmp(key, "Connection")){}
            else if(!strcmp(key, "Proxy-Connection")){}
            else {
            	/* Store this header and forward it directly  */
            	*tok = ':';
            	if(i == NHEADERS) {
            		printf("Too many headers\n");
            		return -1;
            	}
            	strcpy(headers[i++], buf);
            	*n = i;
            }
        }
        rio_readlineb_s(rio, buf, MAXLINE);
	}
	return 1;
}

/* Return 1 if cached, -i if not cached */
void make_request(char *path, char headers[NHEADERS][MAXLINE], 
				  int n_header, int connfd, int clientfd) 
{
	int i;
	char buf[MAXLINE];

    sprintf(buf, "GET %s HTTP/1.0\r\n", path);
    rio_writen_s(clientfd, buf, strlen(buf));

    sprintf(buf, "%s", headers[0]);
    rio_writen_s(clientfd, buf, strlen(buf));

    sprintf(buf, "%s", user_agent_hdr);
    rio_writen_s(clientfd, buf, strlen(buf));

    sprintf(buf, "%s", accept_hdr);
    rio_writen_s(clientfd, buf, strlen(buf));

    sprintf(buf, "%s", accept_encoding_hdr);
    rio_writen_s(clientfd, buf, strlen(buf));

    sprintf(buf, "%s", connection_hdr);
    rio_writen_s(clientfd, buf, strlen(buf));

    sprintf(buf, "%s", proxy_connection_hdr);
    rio_writen_s(clientfd, buf, strlen(buf));

    for(i=1; i<n_header; i++) {
    	sprintf(buf, "%s", headers[i]);
    	rio_writen_s(clientfd, buf, strlen(buf));   	
    }

    sprintf(buf, "\r\n");
    rio_writen_s(clientfd, buf, strlen(buf));

    return;
}

/*  Warpper for rio_writen with consideration of errno EPIPE */
void rio_writen_s(int fd, void *usrbuf, size_t n) 
{
    if (rio_writen(fd, usrbuf, n) != n) {
    	switch(errno) {
    		case EPIPE:
    			printf("[Error] socket closed when write(), recovered.\n");
    			int i = get_thread_index(pthread_self());
    			longjmp(thread_context[i].write_env, -1);
    		default:
    			printf("[Error] Unknown Error in rio_writen_s\n");
    			break;
    	}
    }
}

/*  Warpper for rio_readlineb with consideration of errno ECONNRESET */
ssize_t rio_readlineb_s(rio_t *rio, void *usrbuf, size_t maxlen) 
{
    ssize_t rc;

    if ((rc = rio_readlineb(rio, usrbuf, maxlen)) < 0) {
    	switch(errno) {
    		case ECONNRESET:
    			printf("[Error] socket closed when read(), recovered.");
    			int i = get_thread_index(pthread_self());
    			longjmp(thread_context[i].read_env, -1);
    		default:
    			printf("[Error] Unknown Error in rio_readlineb_s\n");
    			break;
    	}
    }
    return rc;
} 

/*  Warpper for rio_readnb with consideration of errno ECONNRESET */
ssize_t rio_readnb_s(rio_t *rio, void *usrbuf, size_t n) 
{
    ssize_t rc;

    if ((rc = rio_readnb(rio, usrbuf, n)) < 0) {
    	switch(errno) {
    		case ECONNRESET:
    			printf("[Error] socket closed when read(), recovered.");
    			int i = get_thread_index(pthread_self());
    			longjmp(thread_context[i].read_env, -1);
    		default:
    			printf("[Error]Unknown Error in rio_readnb_s\n");
    			break;
    	}
    }
    return rc;
}

void client_error(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg) 
{
    char buf[MAXLINE], body[MAXBUF];

    /* Build the HTTP response body */
    sprintf(body, "<html><title>Proxy Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

    /* Print the HTTP response */
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    rio_writen_s(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    rio_writen_s(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    rio_writen_s(fd, buf, strlen(buf));
    rio_writen_s(fd, body, strlen(body));
}
/*
 *	Helper functions Ends
 */


