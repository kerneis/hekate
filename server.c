#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "hashtable.h"
#include "parse.h"
#include "tracker_conn.h"
#include "list.h"

#define BUF_LEGNTH 512
#define DIR_LEN 512

ht_torrent *test;

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

int
send_chunk(int fd, int chunk, int begin, int length)
{
    int file, rc, pos;
    unsigned char buf[13];
    void *source;
    ht_torrent *t;

    t = test; /* XXX */
    printf("send chunk: length=%d, offset=%lld\n",
	   length, (long long int)chunk*t->p_length+begin);

    *((uint32_t *)buf) = htonl(length+9);
    buf[4] = 7;
    *((uint32_t *)(buf+5)) = htonl(chunk);
    *((uint32_t *)(buf+9)) = htonl(begin);

    if(write(fd, buf, 13)<13){
	perror("(send_unchoke)write");
	return -1;
    }

    file = open(t->path, O_RDONLY);
    if(file<0){
	perror("(send_chunk)open");
	return -1;
    }

    source = mmap(NULL, length, PROT_READ, MAP_PRIVATE,
		  file, chunk*t->p_length+begin);
    if(source==MAP_FAILED){
	perror("(send_chunk)mmap");
	return -1;
    }
    
    for(pos = 0; pos<length; pos+=rc){
	printf(".\n");
	if( (rc=write(fd, source+pos, length-pos))<0
	    && errno != EAGAIN ){
	    perror("(send_chunk)write");
	    munmap(source, length);
	    return -1;
	}}

    if(munmap(source, length)){
	perror("(send_chunk)munmap");
	return -1;
    }

    close(file);

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

    /* XXX */ test = torrent;

    return EXIT_SUCCESS;
}

int
stream_read(int fd)
{
    int rc, pos;
    uint32_t tmp, length;
    char * res;

    rc = read(fd, &tmp, 4);
    if(rc != 4){
	perror("(stream_read) didn't read 4 bytes");
	return EXIT_FAILURE;
    }

    length = ntohl(tmp);
    if(length == 0){
	//keepalive message
	return EXIT_SUCCESS;
    }
    //non-keepalive message
    //pay attention memory explosion
    res = malloc(length);
    pos = 0;
    while(pos<length){
	rc = read(fd, res, length);
	if(rc<0 && errno!=EAGAIN){
	    perror("stream_read)read");
	    free(res);
	    return EXIT_FAILURE;
	}
	pos+=rc;
    }

    //i am not sure that this is a good idea
    printf("message length: %d, type: %d\n", length, res[0]);
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
	printf("index = %d \tbegin = %d\tlength = %d\n",
	       ntohl(*((int32_t *)(res+1))) ,
	       ntohl(*((int32_t *)(res+5))) ,
	       ntohl(*((int32_t *)(res+9))) );
	send_chunk(fd, ntohl(*((int32_t *)(res+1))) ,
		   ntohl(*((int32_t *)(res+5))) ,
		   ntohl(*((int32_t *)(res+9))) );
	break;
    case  8 ://cancel
	printf("index = %d \tbegin = %d\tlength = %d\n",
	       ntohl(*((int32_t *)(res+1))) ,
	       ntohl(*((int32_t *)(res+5))) ,
	       ntohl(*((int32_t *)(res+9))) );
	break;
    default :
	free(res);
	fprintf(stderr , "(stream_read) : Someone sending wierd message\n");
	//not supported by protocol
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
	if(stream_read(client_fd)==EXIT_FAILURE)
	    return EXIT_FAILURE;
	bzero( buffer, rc);
    }
    return EXIT_SUCCESS;
}

int
listening(hashtable * table)
{
    /* XXX sureuse set for socket*/
    int rc, one, socket_fd ,client_fd;
    socklen_t cli_len;
    struct sockaddr_in serv_addr, cli_addr;

    /*TODO : open socket*/
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    rc = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    if(rc){
	perror("listening");
	return EXIT_FAILURE;
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(6969);

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
