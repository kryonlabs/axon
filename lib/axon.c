/*
 * AXON Core Implementation
 * Main AXON initialization and management
 */

#include <lib9.h>
#include "axon.h"
#include "minds.h"
#include "facts.h"
#include "llm_config.h"

/*
 * AXON-specific mind management
 * Each AXON instance maintains its own set of minds with LLM backends
 */

typedef struct AxonMindState {
    Mind **minds;
    int nminds;
} AxonMindState;

/*
 * Initialize AXON
 */
Axon*
axon_init(const char *data_path)
{
    Axon *axon;
    char llm_config_path[512];

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

        /* Multi-mind system directories */
        snprint(path, sizeof(path), "%s/knowledge", data_path);
        (void)create(path, OREAD, 0755 | DMDIR);
        snprint(path, sizeof(path), "%s/minds", data_path);
        (void)create(path, OREAD, 0755 | DMDIR);
        snprint(path, sizeof(path), "%s/facts", data_path);
        (void)create(path, OREAD, 0755 | DMDIR);
        snprint(path, sizeof(path), "%s/facts/by_subject", data_path);
        (void)create(path, OREAD, 0755 | DMDIR);
        snprint(path, sizeof(path), "%s/facts/by_predicate", data_path);
        (void)create(path, OREAD, 0755 | DMDIR);
        snprint(path, sizeof(path), "%s/facts/consensus", data_path);
        (void)create(path, OREAD, 0755 | DMDIR);
        snprint(path, sizeof(path), "%s/contradictions", data_path);
        (void)create(path, OREAD, 0755 | DMDIR);
        snprint(path, sizeof(path), "%s/inbox", data_path);
        (void)create(path, OREAD, 0755 | DMDIR);
        snprint(path, sizeof(path), "%s/inbox/incoming", data_path);
        (void)create(path, OREAD, 0755 | DMDIR);
        snprint(path, sizeof(path), "%s/llm", data_path);
        (void)create(path, OREAD, 0755 | DMDIR);

        /* Create mind-specific directories */
        snprint(path, sizeof(path), "%s/minds/literal", data_path);
        (void)create(path, OREAD, 0755 | DMDIR);
        snprint(path, sizeof(path), "%s/minds/literal/facts", data_path);
        (void)create(path, OREAD, 0755 | DMDIR);
        snprint(path, sizeof(path), "%s/minds/skeptic", data_path);
        (void)create(path, OREAD, 0755 | DMDIR);
        snprint(path, sizeof(path), "%s/minds/skeptic/facts", data_path);
        (void)create(path, OREAD, 0755 | DMDIR);
        snprint(path, sizeof(path), "%s/minds/synthesizer", data_path);
        (void)create(path, OREAD, 0755 | DMDIR);
        snprint(path, sizeof(path), "%s/minds/synthesizer/facts", data_path);
        (void)create(path, OREAD, 0755 | DMDIR);
        snprint(path, sizeof(path), "%s/minds/pattern_matcher", data_path);
        (void)create(path, OREAD, 0755 | DMDIR);
        snprint(path, sizeof(path), "%s/minds/pattern_matcher/facts", data_path);
        (void)create(path, OREAD, 0755 | DMDIR);
        snprint(path, sizeof(path), "%s/minds/questioner", data_path);
        (void)create(path, OREAD, 0755 | DMDIR);
        snprint(path, sizeof(path), "%s/minds/questioner/facts", data_path);
        (void)create(path, OREAD, 0755 | DMDIR);
    }

    /* Initialize LLM configuration */
    snprint(llm_config_path, sizeof(llm_config_path), "%s/llm/llm.conf", data_path);
    llm_config_init(llm_config_path);

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

    /* Cleanup LLM configuration */
    llm_config_cleanup();

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

/*
 * Get all minds
 */
Mind**
axon_get_minds(Axon *axon, int *nminds)
{
    if(nminds != nil)
        *nminds = 0;
    return mind_get_all(nminds);
}

/*
 * Process an entry with all minds
 */
int
axon_process_entry_with_minds(Axon *axon, Entry *e)
{
    MindResult **results;
    int nresults;
    ConsensusFact **consensus;
    int nconsensus;
    FactStore *store;
    int i;

    if(axon == nil || e == nil)
        return -1;

    /* Extract facts from all minds */
    results = axon_extract_from_all_minds(axon, e, &nresults);
    if(results == nil || nresults == 0)
        return 0;  /* No minds ran */

    /* Build consensus */
    consensus = consensus_build(results, nresults, &nconsensus);

    /* Get fact store */
    store = axon_get_fact_store(axon);
    if(store == nil) {
        mind_results_free(results, nresults);
        return -1;
    }

    /* Store mind facts */
    for(i = 0; i < nresults; i++) {
        mind_runner_store_mind_result(store, results[i]);
    }

    /* Store consensus facts */
    if(consensus != nil && nconsensus > 0) {
        for(i = 0; i < nconsensus; i++) {
            fact_store_consensus_fact(store, consensus[i]);
        }
    }

    /* Find and store contradictions */
    mind_runner_find_contradictions(results, nresults, store);

    /* Cleanup */
    mind_results_free(results, nresults);
    if(consensus != nil) {
        for(i = 0; i < nconsensus; i++) {
            consensus_fact_free(consensus[i]);
        }
        free(consensus);
    }

    return 0;
}

/*
 * Extract from all minds
 */
MindResult**
axon_extract_from_all_minds(Axon *axon, Entry *e, int *nresults)
{
    if(axon == nil || e == nil) {
        if(nresults != nil)
            *nresults = 0;
        return nil;
    }

    return mind_extract_all(e, nresults);
}

/*
 * Build consensus
 */
int
axon_build_consensus(Axon *axon, MindResult **results, int nresults)
{
    if(axon == nil || results == nil || nresults == 0)
        return -1;

    /* Consensus building is handled in axon_process_entry_with_minds */
    return 0;
}

/*
 * Get fact store
 */
FactStore*
axon_get_fact_store(Axon *axon)
{
    if(axon == nil)
        return nil;

    return fact_store_init(axon->root_path);
}

/*
 * Query facts
 */
int
axon_query_facts(Axon *axon, FactQuery *query, void **results, int *nresults)
{
    FactStore *store;
    void **facts;
    int types;

    if(axon == nil || query == nil) {
        if(nresults != nil)
            *nresults = 0;
        return -1;
    }

    store = axon_get_fact_store(axon);
    if(store == nil) {
        if(nresults != nil)
            *nresults = 0;
        return -1;
    }

    facts = fact_query_all(store, query, nresults, &types);

    fact_store_close(store);

    if(results != nil && facts != nil)
        *results = facts;

    return 0;
}

/*
 * Store mind facts
 */
int
axon_store_mind_facts(Axon *axon, MindResult *result)
{
    FactStore *store;
    int ret;

    if(axon == nil || result == nil)
        return -1;

    store = axon_get_fact_store(axon);
    if(store == nil)
        return -1;

    ret = mind_runner_store_mind_result(store, result);

    fact_store_close(store);

    return ret;
}

/*
 * Store consensus facts
 */
int
axon_store_consensus_facts(Axon *axon, ConsensusFact **facts, int nfacts)
{
    FactStore *store;
    int ret;

    if(axon == nil || facts == nil || nfacts == 0)
        return -1;

    store = axon_get_fact_store(axon);
    if(store == nil)
        return -1;

    ret = mind_runner_store_consensus(store, facts, nfacts);

    fact_store_close(store);

    return ret;
}
