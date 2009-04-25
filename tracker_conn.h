#ifndef TRACKER_CONN_H
#define TRACKER_CONN_H

#include "list.h"

char * port;
char * peer_id;
list *trackers;

int connection(list * trackers);

#endif
