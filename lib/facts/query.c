/*
 * AXON Fact Query System
 * Query facts by subject, predicate, confidence, etc.
 */

#include <lib9.h>
#include "axon.h"
#include "minds.h"
#include "facts.h"

/*
 * Query facts with criteria
 */
MindFact**
fact_query_mind_facts(FactStore *store, FactQuery *query, int *nresults)
{
    MindFact **results;
    int count;
    int i;
    MindType mind;

    if(store == nil || query == nil) {
        if(nresults != nil)
            *nresults = 0;
        return nil;
    }

    results = nil;
    count = 0;

    /* For each mind type (or specific mind if set) */
    int start_mind = (query->mind_type != MIND_MAX) ? query->mind_type : 0;
    int end_mind = (query->mind_type != MIND_MAX) ? query->mind_type + 1 : MIND_MAX;

    for(mind = start_mind; mind < end_mind; mind++) {
        /* For Phase 1, just scan all entries */
        /* TODO: Phase 2 will use index for fast lookups */

        /* This is a placeholder - would scan mind directories */
        /* For now, return empty results */
    }

    if(nresults != nil)
        *nresults = count;

    return results;
}

/*
 * Query consensus facts
 */
ConsensusFact**
fact_query_consensus(FactStore *store, FactQuery *query, int *nresults)
{
    ConsensusFact **results;
    int count;

    if(store == nil || query == nil) {
        if(nresults != nil)
            *nresults = 0;
        return nil;
    }

    /* Placeholder implementation */
    if(nresults != nil)
        *nresults = 0;

    return nil;
}

/*
 * Query all facts (both mind and consensus)
 */
void**
fact_query_all(FactStore *store, FactQuery *query, int *nresults, int *types)
{
    void **results;
    int count;

    if(store == nil || query == nil) {
        if(nresults != nil)
            *nresults = 0;
        if(types != nil)
            *types = 0;
        return nil;
    }

    /* Placeholder implementation */
    if(nresults != nil)
        *nresults = 0;
    if(types != nil)
        *types = 0;

    return nil;
}

/*
 * Free query results
 */
void
fact_query_free(void **results, int nresults)
{
    int i;

    if(results == nil)
        return;

    /* Note: Don't free the actual facts, just the array */
    free(results);
}

/*
 * Build index from stored facts
 */
int
fact_store_reindex(FactStore *store)
{
    FactIndex *index;

    if(store == nil)
        return -1;

    /* Free old index */
    if(store->index != nil) {
        fact_index_free(store->index);
    }

    /* Create new index */
    index = emalloc9p(sizeof(FactIndex));
    index->by_subject = nil;
    index->nby_subject = 0;
    index->by_predicate = nil;
    index->nby_predicate = 0;
    index->consensus = nil;
    index->nconsensus = 0;
    index->last_built = time(nil);

    /* TODO: Scan directories and build index */

    store->index = index;
    store->dirty = 0;

    return 0;
}

/*
 * Lookup by subject
 */
FactIndexEntry**
fact_index_by_subject(FactIndex *index, const char *subject, int *nresults)
{
    FactIndexEntry **results;
    int i, count;

    if(index == nil || subject == nil) {
        if(nresults != nil)
            *nresults = 0;
        return nil;
    }

    /* Linear search for now */
    count = 0;
    for(i = 0; i < index->nby_subject; i++) {
        if(index->by_subject[i] != nil &&
           strcmp(index->by_subject[i]->subject, subject) == 0) {
            count++;
        }
    }

    if(count == 0) {
        if(nresults != nil)
            *nresults = 0;
        return nil;
    }

    results = emalloc9p(count * sizeof(FactIndexEntry*));
    count = 0;
    for(i = 0; i < index->nby_subject; i++) {
        if(index->by_subject[i] != nil &&
           strcmp(index->by_subject[i]->subject, subject) == 0) {
            results[count++] = index->by_subject[i];
        }
    }

    if(nresults != nil)
        *nresults = count;

    return results;
}

/*
 * Lookup by predicate
 */
FactIndexEntry**
fact_index_by_predicate(FactIndex *index, const char *predicate, int *nresults)
{
    FactIndexEntry **results;
    int i, count;

    if(index == nil || predicate == nil) {
        if(nresults != nil)
            *nresults = 0;
        return nil;
    }

    /* Linear search for now */
    count = 0;
    for(i = 0; i < index->nby_predicate; i++) {
        if(index->by_predicate[i] != nil &&
           strcmp(index->by_predicate[i]->predicate, predicate) == 0) {
            count++;
        }
    }

    if(count == 0) {
        if(nresults != nil)
            *nresults = 0;
        return nil;
    }

    results = emalloc9p(count * sizeof(FactIndexEntry*));
    count = 0;
    for(i = 0; i < index->nby_predicate; i++) {
        if(index->by_predicate[i] != nil &&
           strcmp(index->by_predicate[i]->predicate, predicate) == 0) {
            results[count++] = index->by_predicate[i];
        }
    }

    if(nresults != nil)
        *nresults = count;

    return results;
}

/*
 * Search consensus facts
 */
FactIndexEntry**
fact_index_consensus(FactIndex *index, const char *subject, const char *predicate, int *nresults)
{
    FactIndexEntry **results;
    int i, count;

    if(index == nil) {
        if(nresults != nil)
            *nresults = 0;
        return nil;
    }

    /* Linear search for now */
    count = 0;
    for(i = 0; i < index->nconsensus; i++) {
        int match = 1;
        if(index->consensus[i] == nil)
            continue;
        if(subject != nil && strcmp(index->consensus[i]->subject, subject) != 0)
            match = 0;
        if(predicate != nil && strcmp(index->consensus[i]->predicate, predicate) != 0)
            match = 0;
        if(match)
            count++;
    }

    if(count == 0) {
        if(nresults != nil)
            *nresults = 0;
        return nil;
    }

    results = emalloc9p(count * sizeof(FactIndexEntry*));
    count = 0;
    for(i = 0; i < index->nconsensus; i++) {
        int match = 1;
        if(index->consensus[i] == nil)
            continue;
        if(subject != nil && strcmp(index->consensus[i]->subject, subject) != 0)
            match = 0;
        if(predicate != nil && strcmp(index->consensus[i]->predicate, predicate) != 0)
            match = 0;
        if(match)
            results[count++] = index->consensus[i];
    }

    if(nresults != nil)
        *nresults = count;

    return results;
}

/*
 * Add entry to index
 */
int
fact_index_add(FactIndex *index, FactIndexEntry *entry)
{
    if(index == nil || entry == nil)
        return -1;

    /* Add to by_subject index */
    index->nby_subject++;
    index->by_subject = erealloc9p(index->by_subject,
        index->nby_subject * sizeof(FactIndexEntry*));
    index->by_subject[index->nby_subject - 1] = entry;

    /* Add to by_predicate index */
    index->nby_predicate++;
    index->by_predicate = erealloc9p(index->by_predicate,
        index->nby_predicate * sizeof(FactIndexEntry*));
    index->by_predicate[index->nby_predicate - 1] = entry;

    return 0;
}

/*
 * Free index
 */
void
fact_index_free(FactIndex *index)
{
    int i;

    if(index == nil)
        return;

    if(index->by_subject != nil) {
        /* Free only once - entries are shared */
        for(i = 0; i < index->nby_subject; i++) {
            fact_index_entry_free(index->by_subject[i]);
        }
        free(index->by_subject);
    }

    if(index->by_predicate != nil)
        free(index->by_predicate);  /* Entries already freed */

    if(index->consensus != nil) {
        for(i = 0; i < index->nconsensus; i++) {
            fact_index_entry_free(index->consensus[i]);
        }
        free(index->consensus);
    }

    free(index);
}

/*
 * Free index entry
 */
void
fact_index_entry_free(FactIndexEntry *entry)
{
    if(entry == nil)
        return;

    free(entry->key);
    free(entry->subject);
    free(entry->predicate);
    free(entry->file_path);
    free(entry);
}
