#include <assert.h>
#include <dirent.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include <curl/curl.h>

#include "hashtable.h"
#include "parse.h"
#include "server.h"
#include "sha1.h"
#include "tracker_conn.h"

/*
  getting the directory path and try to add them all to our hashtable
*/
int
upload(hashtable *table, char * path)
{
    DIR * dir;
    int res, pos;
    unsigned char tmp[20];
    char * curr_path;
    buffer * buf;
    benc * result;
    ht_torrent * elmt;
    struct dirent * entry;
    struct stat st;

    dir = opendir(path);
    if(dir == NULL){
	perror("Erreur (Main) : cannot open directory\n");
	return -1;
    }

    while(( entry = readdir(dir) )){
	curr_path = malloc(strlen(path)+strlen(entry->d_name)+2);
	if(!curr_path)
	    return -1;
	sprintf(curr_path,"%s/%s", path, entry->d_name);

	if(strcmp(entry->d_name,".") &&
	   strcmp(entry->d_name,"..")){

	    if(lstat(curr_path,&st)){
		perror("upload");
		continue;
	    }
	    //it's a normal file
	    if(S_ISREG(st.st_mode)){
		pos = strlen(entry->d_name) - 8;

		if(pos>=0 && strcmp(entry->d_name+pos,".torrent") == 0) {
		    buf = open_buffer(curr_path);
		    if(!buf) {
			fprintf(stderr, "Error : open_buffer %s\n",curr_path);
			continue;
		    }

		    result = parsing(buf);
		    memcpy(tmp, result->hash, 20);

		    res = ht_load(table, result);
		    if(res == -1) {
			fprintf(stderr, "fscked append in ht_load.\n");
			exit(EXIT_FAILURE);
		    }
		    if(res == -2) {
			fprintf(stderr, "bad torrent file\n");
			exit(EXIT_FAILURE);
		    }

		    elmt = ht_get(table, tmp);
		    printf("tracker address: %s\n", elmt->tracker);
		    printf("path: %s\n", elmt->path);
		    printf("file length: %lld\n", (long long int)elmt->f_length);
		    printf("chunk size: %lld\n", (long long int)elmt->p_length);

		    close_buffer(buf);
		}
	    }
	    //is  a directory
	    else if(S_ISDIR(st.st_mode)) {
		if(upload(table, curr_path)<0){
		    free(curr_path);
		    closedir(dir);
		    return -1;
		}
	    }
	}
	free(curr_path);
    }
    
    closedir(dir);
    return 0;
}

int
main(int argc, char **argv)
{
    hashtable *table;
    CURLcode global;
    
    if(argc < 2 ){
      fprintf(stderr, "Error : read the man pages before using this program\n");
      exit(EXIT_FAILURE);
    }

    global = curl_global_init(CURL_GLOBAL_ALL);
    assert(!global);

    table = ht_create(10);
    if(!table) {
        perror("ht_create");
        exit(1);
    }
    
    upload(table, argv[1]);
    
    connection(trackers);

    signal(SIGPIPE, SIG_IGN);
    listening(table);

    curl_global_cleanup();

    return 0;
}
