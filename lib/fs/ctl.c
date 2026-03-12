/*
 * AXON Control File Interface
 * Handles commands written to /ctl file
 */

#include <lib9.h>
#include <9p.h>
#include "axon.h"
#include "axonfs.h"

/*
 * Command table
 */
typedef struct Cmdtab {
    int index;
    char *cmd;
    int narg;
} Cmdtab;

static Cmdtab cmds[] = {
    { CMD_ADD_ENTRY,  "add_entry",  2 },
    { CMD_ADD_SOURCE, "add_source", 1 },
    { CMD_SEARCH,     "search",     1 },
    { CMD_STATUS,     "status",     0 },
    { CMD_SYNC,       "sync",       0 },
    { 0, nil, 0 }
};

/*
 * External handlers (defined in handlers.c)
 */
extern void handle_add_entry(Axon *axon, char *title, char *content);
extern void handle_add_source(Axon *axon, char *path);
extern void handle_search(Axon *axon, char *query);
extern void handle_sync(Axon *axon);

/*
 * Write to control file
 */
void
write_ctl(Req *r, Axon *axon)
{
    Cmdbuf *cb;
    Cmdtab *ct;
    char *cmd_buf;

    /* Make a null-terminated copy of the command */
    cmd_buf = emalloc9p(r->ifcall.count + 1);
    memcpy(cmd_buf, r->ifcall.data, r->ifcall.count);
    cmd_buf[r->ifcall.count] = '\0';

    cb = parsecmd(cmd_buf, r->ifcall.count);
    if(cb == nil) {
        respond(r, "parse error");
        free(cmd_buf);
        return;
    }

    ct = lookupcmd(cb, cmds, nelem(cmds));
    if(ct == nil) {
        respondcmderror(r, cb, "unknown command");
        free(cmd_buf);
        free(cb->buf);
        free(cb);
        return;
    }

    switch(ct->index) {
    case CMD_ADD_ENTRY:
        /* add_entry <title> <content> */
        if(cb->nf < 3) {
            respondcmderror(r, cb, "usage: add_entry title content");
            break;
        }
        handle_add_entry(axon, cb->f[1], cb->f[2]);
        r->ofcall.count = r->ifcall.count;
        respond(r, nil);
        break;

    case CMD_ADD_SOURCE:
        /* add_source <path> */
        if(cb->nf < 2) {
            respondcmderror(r, cb, "usage: add_source path");
            break;
        }
        handle_add_source(axon, cb->f[1]);
        r->ofcall.count = r->ifcall.count;
        respond(r, nil);
        break;

    case CMD_SEARCH:
        /* search <query> */
        if(cb->nf < 2) {
            respondcmderror(r, cb, "usage: search query");
            break;
        }
        handle_search(axon, cb->f[1]);
        r->ofcall.count = r->ifcall.count;
        respond(r, nil);
        break;

    case CMD_STATUS:
        /* Return status info (handled by read) */
        r->ofcall.count = r->ifcall.count;
        respond(r, nil);
        break;

    case CMD_SYNC:
        /* Sync to disk */
        handle_sync(axon);
        r->ofcall.count = r->ifcall.count;
        respond(r, nil);
        break;
    }

    free(cmd_buf);
    free(cb->buf);
    free(cb);
}
