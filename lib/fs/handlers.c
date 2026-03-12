/*
 * AXON File Handlers
 * Implements read/write operations for virtual files
 */

#include <lib9.h>
#include <9p.h>
#include "axon.h"
#include "axonfs.h"

/*
 * Read from status file
 */
void
read_status(Req *r)
{
    char *buf;
    int len;
    Axon *axon;

    axon = r->fid->aux;
    if(axon == nil) {
        readstr(r, "AXON Encyclopedia Filesystem\nNo entries\n");
        return;
    }

    /* Format status information */
    len = 512;
    buf = emalloc9p(len);

    snprint(buf, len,
            "AXON Encyclopedia Filesystem\n"
            "Entries: %d\n"
            "Facts: %d\n"
            "Last query: %s\n"
            "Data path: %s\n",
            axon->nentries,
            axon->nfacts,
            axon->last_query ? axon->last_query : "(none)",
            axon->root_path ? axon->root_path : "(unknown)");

    readstr(r, buf);
    free(buf);
}

/*
 * Read from control file (echo commands)
 */
void
read_ctl(Req *r)
{
    readstr(r, "AXON control file\nCommands: add_entry, add_source, search, status, sync\n");
}

/*
 * Write to search query file
 */
void
write_search_query(Req *r, Axon *axon)
{
    char *query;

    query = emalloc9p(r->ifcall.count + 1);
    memcpy(query, r->ifcall.data, r->ifcall.count);
    query[r->ifcall.count] = '\0';

    /* Strip newline */
    if(r->ifcall.count > 0 && query[r->ifcall.count - 1] == '\n')
        query[r->ifcall.count - 1] = '\0';

    free(axon->last_query);
    axon->last_query = query;

    r->ofcall.count = r->ifcall.count;
    respond(r, nil);
}

/*
 * Read from search results file
 */
void
read_search_results(Req *r, Axon *axon)
{
    char *results;
    SearchResult *sr;
    int n;

    if(axon->last_query == nil) {
        readstr(r, "No search query\n");
        return;
    }

    sr = search_entries(axon, axon->last_query, &n);
    if(sr == nil || n == 0) {
        readstr(r, "No results\n");
        return;
    }

    results = format_search_results(sr, n);
    readstr(r, results);
    free(results);
    search_free(sr, n);
}

/*
 * Read from entry file
 */
void
read_entry(Req *r, Axon *axon)
{
    File *f;
    char *path;
    Entry *e;
    char *buf;
    int len;

    f = r->fid->file;
    if(f == nil) {
        respond(r, "no file");
        return;
    }

    /* Build entry title from filename */
    path = smprint("%s", f->name);

    e = axon_find_entry(axon, path);
    free(path);

    if(e == nil) {
        readstr(r, "Entry not found\n");
        return;
    }

    /* Format entry content with metadata */
    len = strlen(e->content) + 512;
    buf = emalloc9p(len);

    snprint(buf, len,
            "# %s\n\n"
            "Confidence: %.2f\n"
            "Sources: %d\n\n"
            "%s\n",
            e->title,
            e->confidence,
            e->nsources,
            e->content);

    readstr(r, buf);
    free(buf);
}

/*
 * Command handlers
 */

void
handle_add_entry(Axon *axon, char *title, char *content)
{
    Entry *e;

    e = entry_create(title, content);
    if(e == nil) {
        fprint(2, "handle_add_entry: failed to create entry\n");
        return;
    }

    axon_add_entry(axon, e);
}

void
handle_add_source(Axon *axon, char *path)
{
    /* Placeholder for source ingestion */
    fprint(2, "handle_add_source: %s (not yet implemented)\n", path);
}

void
handle_search(Axon *axon, char *query)
{
    /* Store query for results file */
    free(axon->last_query);
    axon->last_query = estrdup9p(query);
}

void
handle_sync(Axon *axon)
{
    Entry *e;

    /* Save all entries to disk */
    for(e = axon->entries; e != nil; e = e->next) {
        entry_save(e, axon->root_path);
    }
}
