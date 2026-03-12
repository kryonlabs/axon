#ifndef _AXONFS_H_
#define _AXONFS_H_

/*
 * AXON Filesystem - 9P interface
 * Exposes encyclopedia through 9P protocol
 */

#include <lib9.h>
#include <9p.h>
#include "axon.h"

/*
 * Filesystem node types (for aux field)
 */
#define AXON_CTL         "control"
#define AXON_STATUS      "status"
#define AXON_ENTRIES     "entries"
#define AXON_SEARCH      "search"
#define AXON_QUERY       "search_query"
#define AXON_RESULTS     "search_results"
#define AXON_CONFIDENCE  "confidence"
#define AXON_SOURCES     "sources"

/*
 * Control commands
 */
enum {
    CMD_ADD_ENTRY = 1,
    CMD_ADD_SOURCE,
    CMD_SEARCH,
    CMD_STATUS,
    CMD_SYNC,

    /* Multi-mind inbox processing commands */
    CMD_PROCESS_INBOX,      /* Process all files in inbox/incoming */
    CMD_PROCESS_FILE,       /* Process a specific file */
    CMD_MIND_ENABLE,        /* Enable a specific mind */
    CMD_MIND_DISABLE,       /* Disable a specific mind */
    CMD_MIND_STATUS,        /* Get mind status */
};

/*
 * Filesystem tree creation
 */
void* axon_create_tree(Axon *axon);

/*
 * 9P server creation
 */
Srv* axon_create_srv(Axon *axon);

/*
 * Request handlers
 */
void read_status(Req *r);
void read_ctl(Req *r);
void write_ctl(Req *r, Axon *axon);
void read_entry(Req *r, Axon *axon);
void write_search(Req *r, Axon *axon);
void read_search_results(Req *r, Axon *axon);

/*
 * Command handlers
 */
void handle_add_entry(Axon *axon, char *title, char *content);
void handle_add_source(Axon *axon, char *path);
void handle_search(Axon *axon, char *query);
void handle_sync(Axon *axon);

/* Multi-mind inbox processing handlers */
void handle_process_inbox(Axon *axon);
void handle_process_file(Axon *axon, char *path);
void handle_mind_enable(Axon *axon, char *mind_name);
void handle_mind_disable(Axon *axon, char *mind_name);

/*
 * Status file handlers
 */
void read_mind_status(Req *r, MindType mind_type);
void read_inbox_status(Req *r, Axon *axon);
void read_minds_status(Req *r);
void read_consensus_status(Req *r, Axon *axon);

#endif /* _AXONFS_H_ */
