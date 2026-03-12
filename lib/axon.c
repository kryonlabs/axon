/*
 * AXON Core Implementation
 * Main AXON initialization and management
 */

#include <lib9.h>
#include "axon.h"

/*
 * Initialize AXON
 */
Axon*
axon_init(const char *data_path)
{
    Axon *axon;

    if(data_path == nil)
        return nil;

    axon = emalloc9p(sizeof(Axon));
    if(axon == nil)
        return nil;

    axon->root_path = estrdup9p(data_path);
    axon->entries = nil;
    axon->facts = nil;
    axon->nentries = 0;
    axon->nfacts = 0;
    axon->last_query = nil;
    axon->fs_tree = nil;

    /* Create data directories if they don't exist */
    {
        char path[512];
        snprint(path, sizeof(path), "%s/encyclopedia/entries", data_path);
        (void)create(path, OREAD, 0755 | DMDIR);
        snprint(path, sizeof(path), "%s/encyclopedia/confidence", data_path);
        (void)create(path, OREAD, 0755 | DMDIR);
        snprint(path, sizeof(path), "%s/encyclopedia/contradictions", data_path);
        (void)create(path, OREAD, 0755 | DMDIR);
        snprint(path, sizeof(path), "%s/encyclopedia/sources", data_path);
        (void)create(path, OREAD, 0755 | DMDIR);
        snprint(path, sizeof(path), "%s/index", data_path);
        (void)create(path, OREAD, 0755 | DMDIR);
    }

    return axon;
}

/*
 * Cleanup AXON
 */
void
axon_cleanup(Axon *axon)
{
    Entry *e, *next_e;
    Fact *f, *next_f;

    if(axon == nil)
        return;

    /* Free entries */
    for(e = axon->entries; e != nil; e = next_e) {
        next_e = e->next;
        entry_free(e);
    }

    /* Free facts */
    for(f = axon->facts; f != nil; f = next_f) {
        next_f = f->next;
        fact_free(f);
    }

    free(axon->root_path);
    free(axon->last_query);
    free(axon);
}

/*
 * Add an entry to AXON
 */
int
axon_add_entry(Axon *axon, Entry *e)
{
    if(axon == nil || e == nil)
        return -1;

    /* Add to front of list */
    e->next = axon->entries;
    axon->entries = e;
    axon->nentries++;

    /* Save to disk */
    return entry_save(e, axon->root_path);
}

/*
 * Find an entry by title
 */
Entry*
axon_find_entry(Axon *axon, const char *title)
{
    Entry *e;

    if(axon == nil || title == nil)
        return nil;

    for(e = axon->entries; e != nil; e = e->next) {
        if(strcmp(e->title, title) == 0)
            return e;
    }

    return nil;
}
