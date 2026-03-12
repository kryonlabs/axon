/*
 * AXON Filesystem Tree
 * Creates the 9P filesystem structure for AXON
 */

#include <lib9.h>
#include <9p.h>
#include "axon.h"
#include "axonfs.h"

/*
 * Create the AXON filesystem tree
 */
void*
axon_create_tree(Axon *axon)
{
    Tree *t;
    File *root, *encyclopedia, *memory, *ingest;
    File *entries_dir, *search_dir, *confidence_dir;
    File *contradictions_dir, *sources_dir;

    t = alloctree("axon", "axon", 0555 | DMDIR, nil);
    if(t == nil)
        return nil;

    root = t->root;

    /* Create main directories */
    encyclopedia = createfile(root, "encyclopedia", nil, 0755 | DMDIR, axon);
    memory = createfile(root, "memory", nil, 0755 | DMDIR, axon);
    ingest = createfile(root, "ingest", nil, 0755 | DMDIR, axon);

    /* Create ctl and status files */
    createfile(root, "ctl", nil, 0666, AXON_CTL);
    createfile(root, "status", nil, 0444, AXON_STATUS);

    /* Create encyclopedia subdirectories */
    entries_dir = createfile(encyclopedia, "entries", nil, 0755 | DMDIR, AXON_ENTRIES);
    search_dir = createfile(encyclopedia, "search", nil, 0755 | DMDIR, AXON_SEARCH);
    confidence_dir = createfile(encyclopedia, "confidence", nil, 0755 | DMDIR, AXON_CONFIDENCE);
    contradictions_dir = createfile(encyclopedia, "contradictions", nil, 0755 | DMDIR, AXON_CONTRADICTIONS);
    sources_dir = createfile(encyclopedia, "sources", nil, 0755 | DMDIR, AXON_SOURCES);

    /* Create search interface files */
    createfile(search_dir, "query", nil, 0666, AXON_QUERY);
    createfile(search_dir, "results", nil, 0444, AXON_RESULTS);

    /* Create memory subdirs */
    createfile(memory, "episodic", nil, 0755 | DMDIR, "episodic");
    createfile(memory, "semantic", nil, 0755 | DMDIR, "semantic");
    createfile(memory, "procedural", nil, 0755 | DMDIR, "procedural");

    /* Create ingestion files */
    createfile(ingest, "queue", nil, 0666, "ingest_queue");
    createfile(ingest, "status", nil, 0444, "ingest_status");
    createfile(ingest, "sources", nil, 0755 | DMDIR, "ingest_sources");

    return t;
}
