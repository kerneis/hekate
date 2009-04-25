#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>

#define INTERVAL 3600
#define PEER_ID "torrent_seeder 00000"
#define PEER_IP "127.0.0.1"
#define PORT 6969

int
main(int argc, char **argv)
{
    char peer_id[20];
    char peer_ip[16];

    char announce[256];
    int fd;
    int pos, size, rc;

    memcpy(peer_id, PEER_ID, 20);
    memset(peer_ip, 0, 16);
    memcpy(peer_ip, PEER_IP, strlen(PEER_IP));

    fd = open("announce", O_WRONLY|O_TRUNC|O_CREAT, 0644);
    if(fd<0){
	perror("open");
	exit(EXIT_FAILURE);
    }
    
    size = sprintf(announce,
		   "d8:intervali%d5:peersld7:peer id20:%.20s"
		   "2:ip%d:%s4:porti%deeee",
		   INTERVAL, peer_id,
		   (int)strlen(peer_ip), peer_ip, PORT);

    pos = 0;
    while(pos<size){
	rc = write(fd, announce+pos, size-pos);
	if(rc<0){
	   perror("write");
	   close(fd);
	   exit(EXIT_FAILURE);
	}
	pos+=rc;
    }

    close(fd);
    exit(EXIT_SUCCESS);
}
