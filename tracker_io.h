#ifndef TRACKER_IO_H
#define TRACKER_IO_H

#include "list.h"

char *port;
char *peer_id;
list *trackers;

int init_trackers(list * trackers);

#endif
