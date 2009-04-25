#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>

#include "hashtable.h"
#include "list.h"

char *peer_id = "BittorrentSeeder-000" ;
char *port = "6969";

char *
generate_get(ht_torrent * t, CURL * e_handler)
{
    int length;
    char *res , *hash;

    hash = curl_easy_escape(e_handler, (char *)t->info_hash, 20);

    length  =
	strlen(t->tracker) /* url */
	+ 11 + strlen(hash) /* ?info_hash=_hash_ */
	+ 29 /* :peer_id=_peer_id_ */
	+ 6 + strlen(port) /* :port= */
	+ 11 /* :uploaded=0 */
	+ 13 /* :downloaded=0 */
	+ 7 /* :left=0 */
	+ 14 /* :event=started */;


    res = malloc(length+1);
    if(!res) return NULL;

    /* XXX manque le URL-encoding */
    sprintf(res , "%s?info_hash=%s&peer_id=%.20s&port=%s&"
	    "uploaded=0&downloaded=0&left=0&event=started",
	    t->tracker , hash, peer_id, port);

    curl_free(hash);
    return res;
}

size_t
writer(void  *ptr ,size_t size ,size_t nmemb,void *stream ){
    int i;
    size_t realsize = size * nmemb;

    printf("tracker response: ");
    for(i=0; i<realsize; i++)
	printf("%c", ((char *)ptr)[i]);
    printf("\n");

    return 0;
}

int
connection(list *trackers){
    CURLcode rc;
    CURL *e_handle;
    char *url, *tracker;
    list *current;

    assert(trackers);

    while(trackers){
	current = trackers->elmt;
	tracker = ((ht_torrent *)current->elmt)->tracker;

	if(strncmp(tracker ,"http://",7 ) != 0){
	    fprintf(stderr,"(connection)bad tracker url : %s\n",tracker);
	    continue;
	}

	e_handle = curl_easy_init();
	assert(e_handle);

	do{
	    url = generate_get((ht_torrent *)current->elmt , e_handle);
	    printf("url = %s\n",url);

	    rc = curl_easy_setopt(e_handle ,CURLOPT_URL ,url );
	    if(rc!=CURLE_OK) goto curl_error;

	    rc = curl_easy_setopt(e_handle ,CURLOPT_WRITEFUNCTION ,writer );
	    if(rc!=CURLE_OK) goto curl_error;

	    rc = curl_easy_perform(e_handle);
	    if(!rc) goto curl_error;

	    free(url);
	}while((current  = current -> next));

	/*always cleanup*/
	curl_easy_cleanup(e_handle);
	trackers = trackers -> next;
    }

    return 0;

 curl_error:
    curl_easy_strerror(rc);
    exit(EXIT_FAILURE);
}

