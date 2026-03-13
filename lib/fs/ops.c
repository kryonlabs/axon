/*
 * AXON 9P Operation Handlers
 * Handles 9P protocol operations
 */

#include <lib9.h>
#include <9p.h>
#include "axon.h"
#include "axonfs.h"

/*
 * External request handlers (defined in handlers.c and files.c)
 */
extern void read_status(Req *r);
extern void read_ctl(Req *r);
extern void write_ctl(Req *r, Axon *axon);
extern void read_entry(Req *r, Axon *axon);
extern void write_search_query(Req *r, Axon *axon);
extern void read_search_results(Req *r, Axon *axon);

/* Multi-mind status handlers */
extern void read_mind_status(Req *r, MindType mind_type);
extern void read_inbox_status(Req *r, Axon *axon);
extern void read_minds_status(Req *r);
extern void read_consensus_status(Req *r, Axon *axon);

/* LLM status handlers */
extern void read_llm_status(Req *r);
extern void read_llm_config(Req *r);
extern void write_llm_config(Req *r, Axon *axon);

/*
 * Attach handler
 */
static void
fs_attach(Req *r)
{
    r->fid->aux = r->srv->tree->root->aux;
    respond(r, nil);
}

/*
 * Open handler
 */
static void
fs_open(Req *r)
{
    File *f;
    Axon *axon;

    f = r->fid->file;
    if(f == nil) {
        respond(r, "file not found");
        return;
    }

    /* Attach axon context to fid */
    axon = f->aux;
    if(axon == nil && f->parent != nil)
        axon = f->parent->aux;

    r->fid->aux = axon;
    respond(r, nil);
}

/*
 * Read handler
 */
static void
fs_read(Req *r)
{
    File *f;
    Axon *axon;
    char *aux;

    f = r->fid->file;
    if(f == nil) {
        respond(r, "no file");
        return;
    }

    aux = f->aux;
    axon = r->fid->aux;

    /* Route to appropriate reader based on file type */
    if(aux == nil) {
        /* Directory read - use default */
        dirread9p(r, nil, nil);
        return;
    }

    /* Multi-mind status files */
    if(strcmp(aux, "literal_status") == 0) {
        read_mind_status(r, MIND_LITERAL);
        return;
    }
    if(strcmp(aux, "skeptic_status") == 0) {
        read_mind_status(r, MIND_SKEPTIC);
        return;
    }
    if(strcmp(aux, "synthesizer_status") == 0) {
        read_mind_status(r, MIND_SYNTHESIZER);
        return;
    }
    if(strcmp(aux, "pattern_status") == 0) {
        read_mind_status(r, MIND_PATTERN_MATCHER);
        return;
    }
    if(strcmp(aux, "questioner_status") == 0) {
        read_mind_status(r, MIND_QUESTIONER);
        return;
    }
    if(strcmp(aux, "inbox_status") == 0) {
        read_inbox_status(r, axon);
        return;
    }
    if(strcmp(aux, "minds_status") == 0) {
        read_minds_status(r);
        return;
    }
    if(strcmp(aux, "facts_status") == 0) {
        read_consensus_status(r, axon);
        return;
    }

    /* LLM status and config files */
    if(strcmp(aux, AXON_LLM_STATUS) == 0) {
        read_llm_status(r);
        return;
    }
    if(strcmp(aux, AXON_LLM_CONFIG) == 0) {
        read_llm_config(r);
        return;
    }

    /* Original status files */
    if(strcmp(aux, AXON_STATUS) == 0) {
        read_status(r);
    } else if(strcmp(aux, AXON_CTL) == 0) {
        read_ctl(r);
    } else if(strcmp(aux, AXON_QUERY) == 0) {
        /* Query file - just return empty */
        readbuf(r, "", 0);
    } else if(strcmp(aux, AXON_RESULTS) == 0) {
        if(axon != nil)
            read_search_results(r, axon);
        else
            readbuf(r, "no context", 0);
    } else if(f->parent != nil && f->parent->aux != nil &&
              strcmp(f->parent->aux, AXON_ENTRIES) == 0) {
        /* Entry file */
        if(axon != nil)
            read_entry(r, axon);
        else
            readbuf(r, "no context", 0);
    } else {
        /* Default: empty read for directories */
        dirread9p(r, nil, nil);
    }
}

/*
 * Write handler
 */
static void
fs_write(Req *r)
{
    File *f;
    Axon *axon;
    char *aux;

    f = r->fid->file;
    if(f == nil) {
        respond(r, "no file");
        return;
    }

    aux = f->aux;
    axon = r->fid->aux;

    if(aux == nil) {
        respond(r, "write not supported");
        return;
    }

    if(strcmp(aux, AXON_CTL) == 0) {
        if(axon != nil) {
            write_ctl(r, axon);
        } else {
            respond(r, "no context");
        }
    } else if(strcmp(aux, AXON_QUERY) == 0) {
        if(axon != nil) {
            write_search_query(r, axon);
        } else {
            respond(r, "no context");
        }
    } else if(strcmp(aux, AXON_LLM_CONFIG) == 0) {
        if(axon != nil) {
            write_llm_config(r, axon);
        } else {
            respond(r, "no context");
        }
    } else {
        respond(r, "write not supported");
    }
}

/*
 * Walk handler
 */
static void
fs_walk(Req *r)
{
    respond(r, nil);
}

/*
 * Stat handler
 */
static void
fs_stat(Req *r)
{
    respond(r, nil);
}

/*
 * Create handler (for dynamic entry creation)
 */
static void
fs_create(Req *r)
{
    respond(r, "create not supported - use ctl file");
}

/*
 * Remove handler
 */
static void
fs_remove(Req *r)
{
    respond(r, "remove not supported");
}

/*
 * Flush handler
 */
static void
fs_flush(Req *r)
{
    respond(r, nil);
}

/*
 * Initialize 9P server
 */
Srv*
axon_create_srv(Axon *axon)
{
    Srv *s;

    s = emalloc9p(sizeof(Srv));
    if(s == nil)
        return nil;

    s->tree = axon_create_tree(axon);
    if(s->tree == nil) {
        free(s);
        return nil;
    }

    /* Set handlers */
    s->attach = fs_attach;
    s->open = fs_open;
    s->read = fs_read;
    s->write = fs_write;
    s->walk = fs_walk;
    s->stat = fs_stat;
    s->create = fs_create;
    s->remove = fs_remove;
    s->flush = fs_flush;

    return s;
}
