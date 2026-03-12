/*
 * AXON Mind Runner
 * Coordinates running multiple minds on entries
 */

#include <lib9.h>
#include "axon.h"
#include "minds.h"
#include "facts.h"

/*
 * Process an entry through all active minds and collect results
 */
MindResult**
mind_runner_process_all(Entry *entry, int *nresults)
{
    MindResult **results;

    if(entry == nil) {
        if(nresults != nil)
            *nresults = 0;
        return nil;
    }

    /* Use the mind system to extract from all minds */
    results = mind_extract_all(entry, nresults);

    return results;
}

/*
 * Store results from a single mind
 */
int
mind_runner_store_mind_result(FactStore *store, MindResult *result)
{
    int i;

    if(store == nil || result == nil)
        return -1;

    /* Store each fact from this mind */
    for(i = 0; i < result->nfacts; i++) {
        if(fact_store_mind_fact(store, result->mind_type,
                               result->facts[i], result->source_entry) < 0)
            return -1;
    }

    return 0;
}

/*
 * Store all mind results
 */
int
mind_runner_store_all_results(FactStore *store, MindResult **results, int nresults)
{
    int i;

    if(store == nil || results == nil)
        return -1;

    for(i = 0; i < nresults; i++) {
        if(mind_runner_store_mind_result(store, results[i]) < 0)
            return -1;
    }

    return 0;
}

/*
 * Build consensus from mind results
 */
ConsensusFact**
mind_runner_build_consensus(MindResult **results, int nresults, int *nconsensus)
{
    return consensus_build(results, nresults, nconsensus);
}

/*
 * Store consensus facts
 */
int
mind_runner_store_consensus(FactStore *store, ConsensusFact **consensus, int nconsensus)
{
    int i;

    if(store == nil || consensus == nil)
        return -1;

    for(i = 0; i < nconsensus; i++) {
        if(fact_store_consensus_fact(store, consensus[i]) < 0)
            return -1;
    }

    return 0;
}

/*
 * Find and store contradictions
 */
int
mind_runner_find_contradictions(MindResult **results, int nresults, FactStore *store)
{
    Contradiction **contradictions;
    int ncontra, i;
    char *path;

    if(results == nil || nresults == 0 || store == nil)
        return -1;

    contradictions = contradictions_find(results, nresults, &ncontra);

    if(contradictions == nil || ncontra == 0)
        return 0;  /* No contradictions */

    /* Store each contradiction */
    for(i = 0; i < ncontra; i++) {
        Contradiction *con = contradictions[i];
        int fd;
        char *content;

        if(con == nil)
            continue;

        /* Create contradiction file */
        path = smprint("%s/contradictions/%s", store->data_path, con->fact_key);
        if(path == nil)
            continue;

        fd = create(path, OWRITE, 0644);
        if(fd >= 0) {
            content = smprint("Fact Key: %s\n"
                             "Discovered: %lld\n"
                             "Claims: %d\n"
                             "Resolved: %s\n",
                             con->fact_key,
                             con->discovered_at,
                             con->nclaims,
                             con->resolved ? "Yes" : "No");

            if(content != nil) {
                write(fd, content, strlen(content));
                free(content);
            }

            close(fd);
        }

        free(path);
    }

    /* Free contradictions */
    for(i = 0; i < ncontra; i++) {
        contradiction_free(contradictions[i]);
    }
    free(contradictions);

    return ncontra;
}
