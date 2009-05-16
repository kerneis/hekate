#ifndef TRACKER_IO_H
#define TRACKER_IO_H

#include "list.h"

char *port;
char *peer_id;
tr_list *trackers;

void init_trackers();
#endif
