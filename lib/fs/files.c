/*
 * AXON Virtual File Implementations
 * Various helper files and special file handlers
 */

#include <lib9.h>
#include <9p.h>
#include <fcntl.h>
#include "axon.h"
#include "axonfs.h"
#include "minds.h"
#include "facts.h"

/*
 * Forward declarations for external functions
 */
extern Mind* mind_get_by_type(MindType type);
extern void mind_free(Mind *mind);
extern char* mind_type_name(MindType type);

/*
 * Generate confidence file content
 */
char*
generate_confidence_content(Axon *axon)
{
    Entry *e;
    char *buf;
    int len;
    int count;

    if(axon->entries == nil)
        return strdup("");

    /* Calculate total length needed */
    len = 0;
    count = 0;
    for(e = axon->entries; e != nil; e = e->next) {
        len += strlen(e->title) + 32;
        count++;
    }

    buf = emalloc9p(len + 1);
    buf[0] = '\0';

    for(e = axon->entries; e != nil; e = e->next) {
        strcat(buf, e->title);
        strcat(buf, ": ");
        strcat(buf, smprint("%.2f\n", e->confidence));
    }

    return buf;
}

/*
 * Read confidence directory entry
 */
void
read_confidence_file(Req *r, Axon *axon, char *title)
{
    Entry *e;
    char *buf;

    e = axon_find_entry(axon, title);
    if(e == nil) {
        readstr(r, "Entry not found\n");
        return;
    }

    buf = smprint("%.2f\n", e->confidence);
    readstr(r, buf);
    free(buf);
}

/*
 * Generate sources directory content
 */
char*
generate_sources_list(Axon *axon)
{
    char *buf;

    buf = smprint("AXON Sources Directory\n\n");
    /* Placeholder for future implementation */
    return buf;
}

/*
 * Multi-mind status file handlers
 */

/*
 * Read mind-specific status
 */
void
read_mind_status(Req *r, MindType mind_type)
{
    char *status;
    Mind *mind;
    int len;

    mind = mind_get_by_type(mind_type);
    if(mind == nil) {
        respond(r, "mind not found");
        return;
    }

    status = smprint(
        "Mind: %s\n"
        "Type: %s\n"
        "Enabled: %s\n"
        "Default Confidence: %.2f\n"
        "Description: %s\n",
        mind->name,
        mind_type_name(mind->type),
        mind->enabled ? "yes" : "no",
        mind->default_confidence,
        mind->description
    );

    mind_free(mind);

    len = strlen(status);
    r->ofcall.count = len < r->ifcall.count ? len : r->ifcall.count;
    memmove(r->ofcall.data, status, r->ofcall.count);
    free(status);
    respond(r, nil);
}

/*
 * Read inbox status
 */
void
read_inbox_status(Req *r, Axon *axon)
{
    char *inbox_path;
    Dir *files;
    int nfiles;
    char *status;
    int len;
    int i, waiting;

    if(axon == nil) {
        respond(r, "no axon");
        return;
    }

    inbox_path = smprint("%s/inbox/incoming", axon->root_path);
    nfiles = dirreadall(inbox_path, &files);

    /* Count non-directory files */
    waiting = 0;
    for(i = 0; i < nfiles; i++) {
        if(!(files[i].mode & DMDIR))
            waiting++;
    }

    status = smprint(
        "Inbox Status\n"
        "===========\n"
        "Files waiting: %d\n"
        "Status: %s\n"
        "Path: %s\n",
        waiting,
        waiting > 0 ? "Ready to process" : "Empty",
        inbox_path
    );

    free(inbox_path);
    if(files != nil)
        free(files);

    len = strlen(status);
    r->ofcall.count = len < r->ifcall.count ? len : r->ifcall.count;
    memmove(r->ofcall.data, status, r->ofcall.count);
    free(status);
    respond(r, nil);
}

/*
 * Read minds overall status
 */
void
read_minds_status(Req *r)
{
    char *status;
    int len;
    int i;

    status = estrdup9p(
        "AXON Multi-Mind System\n"
        "======================\n\n"
    );

    for(i = 0; i < MIND_MAX; i++) {
        char *line;
        Mind *mind = mind_get_by_type((MindType)i);
        if(mind != nil) {
            line = smprint("%s: %s (confidence: %.2f)\n",
                          mind->name,
                          mind->enabled ? "enabled" : "disabled",
                          mind->default_confidence);
            char *new_status = emalloc9p(strlen(status) + strlen(line) + 1);
            strcpy(new_status, status);
            strcat(new_status, line);
            free(status);
            status = new_status;
            free(line);
            mind_free(mind);
        }
    }

    len = strlen(status);
    r->ofcall.count = len < r->ifcall.count ? len : r->ifcall.count;
    memmove(r->ofcall.data, status, r->ofcall.count);
    free(status);
    respond(r, nil);
}

/*
 * Read consensus facts status
 */
void
read_consensus_status(Req *r, Axon *axon)
{
    char *status;
    int len;

    status = smprint(
        "Consensus Facts Status\n"
        "======================\n"
        "Data Path: %s/facts/consensus\n"
        "Status: Ready\n",
        axon != nil ? axon->root_path : "none"
    );

    len = strlen(status);
    r->ofcall.count = len < r->ifcall.count ? len : r->ifcall.count;
    memmove(r->ofcall.data, status, r->ofcall.count);
    free(status);
    respond(r, nil);
}
