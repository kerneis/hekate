#ifndef TRACKER_IO_H
#define TRACKER_IO_H

#include "list.h"

extern int notracker;

char *port;
extern char peer_id[21];
tr_list *trackers;

void init_trackers();
#endif
