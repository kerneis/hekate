#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <curl/curl.h>

#include "hashtable.h"
#include "parse.h"
#include "server.h"
#include "sha1.h"
#include "tracker_conn.h"

int
main(int argc, char **argv)
{
    hashtable *table;
    CURLcode global;
    
    if(argc > 1 ){ 
      fprintf(stderr, "Error : read the man pages before using this program\n");
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
    listening(table);

    curl_global_cleanup();

    return 0;
}
