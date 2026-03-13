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
    { CMD_ADD_ENTRY,     "add_entry",     2 },
    { CMD_ADD_SOURCE,    "add_source",    1 },
    { CMD_SEARCH,        "search",        1 },
    { CMD_STATUS,        "status",        0 },
    { CMD_SYNC,          "sync",          0 },

    /* Multi-mind inbox processing */
    { CMD_PROCESS_INBOX, "process_inbox", 0 },
    { CMD_PROCESS_FILE,  "process_file",  1 },
    { CMD_MIND_ENABLE,   "mind_enable",   1 },
    { CMD_MIND_DISABLE,  "mind_disable",  1 },
    { CMD_MIND_STATUS,   "mind_status",   0 },

    /* LLM configuration commands */
    { CMD_LLM_SET_PROVIDER, "llm_set_provider", 1 },
    { CMD_LLM_TEST,         "llm_test",          0 },
    { CMD_LLM_SET_MODEL,    "llm_set_model",     2 },
    { CMD_LLM_STATUS,       "llm_status",        0 },

    { 0, nil, 0 }
};

/*
 * External handlers (defined in handlers.c)
 */
extern void handle_add_entry(Axon *axon, char *title, char *content);
extern void handle_add_source(Axon *axon, char *path);
extern void handle_search(Axon *axon, char *query);
extern void handle_sync(Axon *axon);

/* Multi-mind inbox processing handlers */
extern void handle_process_inbox(Axon *axon);
extern void handle_process_file(Axon *axon, char *path);
extern void handle_mind_enable(Axon *axon, char *mind_name);
extern void handle_mind_disable(Axon *axon, char *mind_name);

/* LLM configuration handlers */
extern void handle_llm_set_provider(Axon *axon, char *provider_name);
extern void handle_llm_test(Axon *axon);
extern void handle_llm_set_model(Axon *axon, char *mind_name, char *model);
extern void handle_llm_status(Axon *axon);

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

    case CMD_PROCESS_INBOX:
        /* Process all files in inbox/incoming */
        handle_process_inbox(axon);
        r->ofcall.count = r->ifcall.count;
        respond(r, nil);
        break;

    case CMD_PROCESS_FILE:
        /* Process a specific file */
        if(cb->nf < 2) {
            respondcmderror(r, cb, "usage: process_file path");
            break;
        }
        handle_process_file(axon, cb->f[1]);
        r->ofcall.count = r->ifcall.count;
        respond(r, nil);
        break;

    case CMD_MIND_ENABLE:
        /* Enable a specific mind */
        if(cb->nf < 2) {
            respondcmderror(r, cb, "usage: mind_enable name");
            break;
        }
        handle_mind_enable(axon, cb->f[1]);
        r->ofcall.count = r->ifcall.count;
        respond(r, nil);
        break;

    case CMD_MIND_DISABLE:
        /* Disable a specific mind */
        if(cb->nf < 2) {
            respondcmderror(r, cb, "usage: mind_disable name");
            break;
        }
        handle_mind_disable(axon, cb->f[1]);
        r->ofcall.count = r->ifcall.count;
        respond(r, nil);
        break;

    case CMD_MIND_STATUS:
        /* Status is read via /minds/status file */
        r->ofcall.count = r->ifcall.count;
        respond(r, nil);
        break;

    case CMD_LLM_SET_PROVIDER:
        /* Set default LLM provider */
        if(cb->nf < 2) {
            respondcmderror(r, cb, "usage: llm_set_provider provider");
            break;
        }
        handle_llm_set_provider(axon, cb->f[1]);
        r->ofcall.count = r->ifcall.count;
        respond(r, nil);
        break;

    case CMD_LLM_TEST:
        /* Test LLM connection */
        handle_llm_test(axon);
        r->ofcall.count = r->ifcall.count;
        respond(r, nil);
        break;

    case CMD_LLM_SET_MODEL:
        /* Set model for specific mind */
        if(cb->nf < 3) {
            respondcmderror(r, cb, "usage: llm_set_model mind provider");
            break;
        }
        handle_llm_set_model(axon, cb->f[1], cb->f[2]);
        r->ofcall.count = r->ifcall.count;
        respond(r, nil);
        break;

    case CMD_LLM_STATUS:
        /* Status is read via /llm/status file */
        r->ofcall.count = r->ifcall.count;
        respond(r, nil);
        break;
    }

    free(cmd_buf);
    free(cb->buf);
    free(cb);
}
