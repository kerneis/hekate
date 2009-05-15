#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>

void accept_connection(int fd);
void handle_connection(int fd);

static struct sockaddr_in addr;
static socklen_t len;

char *root = "/usr/share/polipo/www/";

int port = 8080;
static char resp[] = "HTTP/1.0 200 OK\r\n"
                    "Server: dummy-tracker\r\n"
                    "Connection: close\r\n\r\n"
  "d8:intervali600e5:peersld2:id20:123456789012345678902:ip9:127.0.0.14:porti6969eeee";
static int resp_size = sizeof(resp);


#undef htons

int
main()
{
    int fd, rc;
    int one = 1;
    
    signal(SIGPIPE, SIG_IGN);

    fd = socket(PF_INET, SOCK_STREAM, 0);
    if(fd < 0) { perror("socket"); exit(1); }

    rc = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(one));
    if(rc < 0) perror("setsockopt(SO_REUSEADDR)");

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    rc = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
    if(rc < 0) { perror("bind"); exit(1); }

    rc = listen(fd, 1024);
    if(rc < 0) { perror("listen"); exit(1); }

    accept_connection(fd);

    return 0;
}

void
accept_connection(int fd)
{
    while(1) {
        int rc, fd2;
        len = sizeof(addr);
        fd2 = accept(fd, (struct sockaddr*)&addr, &len);
        if(fd2 < 0) {
            if(errno == EINTR) {
                goto nofork;
            } else {
                perror("accept");
                continue;
            }
        }
        rc = fork();
        if(rc < 0) {
            perror("fork");
            continue;
        }

        if(rc == 0) {
            handle_connection(fd2);
            exit(0);
        }

        close(fd2);

    nofork:
        while(1) {
            rc = waitpid(-1, NULL, WNOHANG);
            if(rc <= 0) {
                if(rc < 0 && errno != ECHILD)
                    perror("wait");
                break;
            }
        }
    }
}

void
handle_connection(int fd)
{
    int rc, n, ffd, len;
    char *buf, *fn;
    int i, val;

    val = 1;
    rc = setsockopt(fd, SOL_TCP, TCP_NODELAY, (char *)&val, sizeof(val));
    if(rc < 0)
        goto fail;

    buf = malloc(4096);
    rc = 0;

 again:
    rc += read(fd, buf + rc, 4096 - rc);
    if(rc < 0) { perror("read"); goto fail; }

    if(rc < 4)
        goto fail;

    if(memcmp(buf, "GET ", 4) != 0)
        goto fail;

    for(i = 5; i < rc; i++)
        if(buf[i] == ' ' || buf[i] == '?' || buf[i] == '\r' || buf[i] == '\n')
            break;
    if(i == rc && rc < 4096)
        goto again;

    len = strlen(root);

    fn = malloc(len + 1 + i - 5 + 12);
    strcpy(fn, root);
    memcpy(fn + len, buf + 5, i - 5);

    if(buf[i - 1] == '/')
        strcpy(fn + len + i - 5, "index.html");
    else
        fn[len + i - 5] = '\0';

    i--;

 search:
    while(i < rc - 3)
      if(buf[i++] == '\r' && buf[i] == '\n'){
        i++; if(buf[i++] == '\r' && buf[i] == '\n')
          goto send;
        }


    if(rc < 4096) {
        rc += read(fd, buf + rc, 4096 - rc);
        goto search;
    }

 send:
    rc = write(fd, &resp, resp_size);
    if(rc != resp_size) { perror("st_write"); goto fail; }
    fprintf(stderr,"Responded to a request.\n");

 fail:
    close(fd);
    free(buf);
    return;
}
