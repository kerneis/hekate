#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

#include "hashtable.h"
#include "parse.h"
#include "tracker_conn.h"
#include "list.h"

#define BUF_LEGNTH 512
#define DIR_LEN 512

int is_bitfield = 0;

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
send_bitfield(int fd, ht_torrent *t)
{
    int num_chunks, length, i, rc, pos;
    unsigned char *buf;
    
    num_chunks = (t->f_length-1)/t->p_length+1;
    length = (num_chunks-1)/8+1;

    buf = malloc(length+5);
    if(!buf){
	perror("(send_bitfield)malloc");
	return -1;
    }

    /* TODO: initialise only num_chunks to 1 */
    memset(buf+5, 0xFF, length);

    ((uint32_t *)buf)[0] = htonl(length+1);
    buf[4] = 5;
    
    printf("length=%d, type=%d\n", ntohl(((uint32_t *)buf)[0]), buf[4]);

    pos = 0;
    for(i=0; i<length+5; i++){
	rc = write(fd, buf+pos, length+5-pos);
	if(rc<0 && errno!=EAGAIN){
	    perror("(send_bitfield)write");
	    return -1;
	}
	pos+=rc;
    }
    return 0;
}


int
send_unchoke(int fd)
{
    unsigned char buf[5];
    
    ((uint32_t *)buf)[0] = htonl(1);
    buf[4] = 1;

    if(write(fd, buf, 5)<5){
	perror("(send_unchoke)write");
	return -1;
    }
    return 0;
}

/*
  return EXIT_FAILURE if sth wrong happend in 
  handshake otherwise return EXIT_SUCCESS
 */
int
handshake(hashtable * ht , int fd){
    int rc;
    const char * protocol = "BitTorrent protocol";
    unsigned char hash[20]; 
    char * res , * tmp , t[68];
    ht_torrent * torrent;
    //must use a while to be sure 
    rc = read(fd , t , 68);
    if(rc != 68) {
	perror("Error (handshake) : cannot read from fd");
	return EXIT_FAILURE;
    }
    
    if( t[0] != 19 ){
	fprintf(stderr,"Error (handshake) : bad protocol %c\n",
		*t);
	return EXIT_FAILURE;
    }
    
    rc = memcmp( t+1 , protocol , 19); 
    if(rc != 0){
	perror("Error (handshake) : memcmp");
	return EXIT_FAILURE;
    }
    
    if(!memcpy( hash , t + 28 , 20)){
	perror("Error (handshake) : memcpy");
	return EXIT_FAILURE;
    }
    
    if ( ! (torrent = ht_get( ht , (unsigned char *)hash )) ){
	fprintf(stderr , "Error (handshake) : cannot find the hash in the table");
	return EXIT_FAILURE;
    }
    //so all is ok and we can say that all is ok yupiiiii!!!!
    //now lets generate the ACK response 
    res = malloc( 1 + 19 + 8 + 20 + 20 );
    tmp = res;
    
    tmp[0] = 19;
    tmp ++;
    
    strncpy(tmp , "BitTorrent protocol" , 19);
    tmp += 19;
    
    tmp[0] = tmp[1] = tmp[2] = tmp[3] = 
	tmp[4] =tmp[5] =tmp[6] =tmp[7] = 0;
    tmp += 8;
    
    if(!memcpy(tmp , torrent -> info_hash , 20)){
	perror("Error (handshake) : cannot copy the hash");
	return EXIT_FAILURE;
    }
    tmp += 20;
    
    strncpy(tmp , peer_id , 20 );
    tmp = NULL;
    rc = write(fd , res , 68);
    if(!rc) {
	perror("Error (handshake) : cannot write to fd");
	return EXIT_FAILURE;
    }
    

    if(send_bitfield(fd, torrent)<0){
	fprintf(stderr, "couldn't send bitfield\n");
	return EXIT_FAILURE;
    }
    
    if(send_unchoke(fd)<0){
	fprintf(stderr, "couldn't send unchoke\n");
	return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int
stream_read(int fd)
{
    int length ,rc ,i;
    int32_t tmp = 0;
    char * res;

    rc = read(fd , &tmp , sizeof(int32_t));
    if(rc != sizeof(int32_t)){
        fprintf(stderr , "Error (stream_read) : cannot read from fd1\n");
        return EXIT_FAILURE;
    }
    
    length = ntohl(tmp);
    if(length == 0){
	//keepalive message
	return EXIT_SUCCESS;
    }
    
    //non-keepalive message
    /* XXX : pay attention memory explosion*/
    res = malloc(length);
    rc = read(fd , res , length );
    if(rc != length){
	fprintf(stderr , "Error (stream_read) : cannot read from fd2\n");	
	free(res);
	return EXIT_FAILURE;
    }
    //i am not sure that this is a good idea
    switch(res[0]) {
    case  7 : case  5 : case 0: case 4:
	free(res);
	return EXIT_SUCCESS;
    case  1 :
	//unchoke
	//no payload
	printf("unchoke\n");
	break;
    case  2 : 
	//intrested
	//no payload
	printf("Win : so let him dl everything he want\n");
	break;
    case  3 : 
	//not interested
	//no payload and let set timer
	printf("not intreseted\n");
	//probably i must close the connection
	//close(fd);
	
	break;
	
    case  6 ://request
    case  8 ://cancel
	
	printf("index = %d \tbegin = %d\tlength = %d\n",
	       *((int32_t *)(res+1)) , 
	       *((int32_t *)(res+5)) ,
	       *((int32_t *)(res+9)) );
	break;
    default :
	free(res);
	//not supported by protocol
	fprintf(stderr ,"Error (stream_read):Someone sending wierd message\n");
	return EXIT_FAILURE;
    }
    
    free(res);
    return EXIT_SUCCESS;
}

int 
stream_write(int fd)
{
    return EXIT_SUCCESS;
}

int
client(hashtable * t , int client_fd ){
    int rc;
    unsigned char buffer[BUF_LEGNTH];
    
    rc = handshake(t , client_fd);
    if(rc == EXIT_FAILURE){
	fprintf(stderr,"Error (client) : handshake is just like a footshake\n");
	return EXIT_FAILURE;
    }
    
    while(1){
	if ( stream_read(client_fd) == EXIT_FAILURE ) 
	    break;
	bzero( buffer, rc);
    }
    
    return EXIT_SUCCESS;
}

int
listening(hashtable * table)
{
    /* XXX sureuse set for socket*/
    int rc ,socket_fd ,client_fd ,one = 1;
    socklen_t cli_len;
    struct sockaddr_in serv_addr, cli_addr;
    
    /*TODO : open socket*/
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(6969);
    rc = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(one));
    if(rc < 0) 
	perror("Error (listening) : setsockopt(SO_REUSEADDR)");
    
    if (bind(socket_fd, (struct sockaddr *) &serv_addr,
	     sizeof(serv_addr)) < 0)
	perror("(listening)bind");
    
    
    while(1){
	rc = listen(socket_fd ,20 );
	if(rc) perror("(listening)listen");
	
	while(1){
	    cli_len = sizeof(cli_addr);
	    client_fd = accept(socket_fd ,
			       (struct sockaddr *) &cli_addr ,
			       &cli_len );
	    if(!client(table , client_fd)){
		//write(client_fd , "\n\r\n\r" , 4);
		close(client_fd);
		
	    }
	    break;
	    printf("still in bad while\n");
	    /*TODO : write itf its necessairy donno realy what shall we do?!*/
	}
	printf("socket close let listen someone else\n");
    }
    
    return EXIT_SUCCESS;
}

