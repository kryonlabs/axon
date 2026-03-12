/*
 * AXON Search Index
 * Inverted index for fast text search
 */

#include <lib9.h>
#include "axon.h"

/*
 * Index entry structure
 */
typedef struct IndexEntry {
    char *term;
    Entry **entries;
    int *positions;
    int nentries;
    struct IndexEntry *next;
} IndexEntry;

/*
 * Index state
 */
static IndexEntry **index_hash = nil;
static int index_size = 1024;  /* Hash table size */

/*
 * Simple hash function
 */
static uint
hash_term(const char *term)
{
    uint h;
    const char *p;

    h = 0;
    for(p = term; *p != '\0'; p++) {
        h = (h << 5) + h + *p;
    }

    return h % index_size;
}

/*
 * Initialize search index
 */
int
index_init(void)
{
    int i;

    if(index_hash != nil)
        return 0;  /* Already initialized */

    index_hash = emalloc9p(index_size * sizeof(IndexEntry*));
    if(index_hash == nil)
        return -1;

    for(i = 0; i < index_size; i++) {
        index_hash[i] = nil;
    }

    return 0;
}

/*
 * Add an entry to the index
 */
int
index_add(Entry *e)
{
    char *content, *p;
    char *term_start, *term_end;
    char term[128];
    int term_len;
    uint h;
    IndexEntry *ie;
    int i;

    if(e == nil || index_hash == nil)
        return -1;

    /* Index title */
    index_add_field(e, e->title);

    /* Index content (tokenize by whitespace and punctuation) */
    content = estrdup9p(e->content);
    p = content;

    while(*p != '\0') {
        /* Skip non-alphanumeric */
        while(*p != '\0' && !isalnum((uchar)*p))
            p++;

        if(*p == '\0')
            break;

        term_start = p;

        /* Find end of term */
        while(*p != '\0' && isalnum((uchar)*p))
            p++;

        term_end = p;

        /* Copy term */
        term_len = term_end - term_start;
        if(term_len > 0 && term_len < sizeof(term)) {
            memcpy(term, term_start, term_len);
            term[term_len] = '\0';

            /* Add to index */
            h = hash_term(term);

            /* Look for existing entry */
            for(ie = index_hash[h]; ie != nil; ie = ie->next) {
                if(strcmp(ie->term, term) == 0)
                    break;
            }

            if(ie == nil) {
                /* Create new index entry */
                ie = emalloc9p(sizeof(IndexEntry));
                ie->term = estrdup9p(term);
                ie->entries = nil;
                ie->positions = nil;
                ie->nentries = 0;
                ie->next = index_hash[h];
                index_hash[h] = ie;
            }

            /* Add entry to index entry */
            for(i = 0; i < ie->nentries; i++) {
                if(ie->entries[i] == e)
                    break;  /* Already indexed */
            }

            if(i >= ie->nentries) {
                /* Add new entry reference */
                ie->nentries++;
                ie->entries = erealloc9p(ie->entries,
                    ie->nentries * sizeof(Entry*));
                ie->positions = erealloc9p(ie->positions,
                    ie->nentries * sizeof(int));
                ie->entries[ie->nentries - 1] = e;
                ie->positions[ie->nentries - 1] = term_start - content;
            }
        }
    }

    free(content);
    return 0;
}

/*
 * Add a specific field to the index
 */
int
index_add_field(Entry *e, char *field)
{
    char *p;
    char *term_start, *term_end;
    char term[128];
    int term_len;
    uint h;
    IndexEntry *ie;
    int i;

    if(e == nil || field == nil || index_hash == nil)
        return -1;

    p = field;

    while(*p != '\0') {
        /* Skip non-alphanumeric */
        while(*p != '\0' && !isalnum((uchar)*p))
            p++;

        if(*p == '\0')
            break;

        term_start = p;

        /* Find end of term */
        while(*p != '\0' && isalnum((uchar)*p))
            p++;

        term_end = p;

        /* Copy term */
        term_len = term_end - term_start;
        if(term_len > 0 && term_len < sizeof(term)) {
            memcpy(term, term_start, term_len);
            term[term_len] = '\0';

            /* Add to index (same logic as above) */
            h = hash_term(term);

            for(ie = index_hash[h]; ie != nil; ie = ie->next) {
                if(strcmp(ie->term, term) == 0)
                    break;
            }

            if(ie == nil) {
                ie = emalloc9p(sizeof(IndexEntry));
                ie->term = estrdup9p(term);
                ie->entries = nil;
                ie->positions = nil;
                ie->nentries = 0;
                ie->next = index_hash[h];
                index_hash[h] = ie;
            }

            for(i = 0; i < ie->nentries; i++) {
                if(ie->entries[i] == e)
                    break;
            }

            if(i >= ie->nentries) {
                ie->nentries++;
                ie->entries = erealloc9p(ie->entries,
                    ie->nentries * sizeof(Entry*));
                ie->positions = erealloc9p(ie->positions,
                    ie->nentries * sizeof(int));
                ie->entries[ie->nentries - 1] = e;
                ie->positions[ie->nentries - 1] = term_start - field;
            }
        }
    }

    return 0;
}

/*
 * Search the index for a term
 */
Entry**
index_search(const char *term, int *nresults)
{
    uint h;
    IndexEntry *ie;
    Entry **results;

    if(term == nil || nresults == nil || index_hash == nil)
        return nil;

    h = hash_term(term);

    for(ie = index_hash[h]; ie != nil; ie = ie->next) {
        if(strcmp(ie->term, term) == 0)
            break;
    }

    if(ie == nil) {
        *nresults = 0;
        return nil;
    }

    /* Return copy of entries array */
    results = emalloc9p(ie->nentries * sizeof(Entry*));
    memcpy(results, ie->entries, ie->nentries * sizeof(Entry*));
    *nresults = ie->nentries;

    return results;
}

/*
 * Clean up the index
 */
void
index_cleanup(void)
{
    int i;
    IndexEntry *ie, *next;

    if(index_hash == nil)
        return;

    for(i = 0; i < index_size; i++) {
        for(ie = index_hash[i]; ie != nil; ie = next) {
            next = ie->next;
            free(ie->term);
            free(ie->entries);
            free(ie->positions);
            free(ie);
        }
    }

    free(index_hash);
    index_hash = nil;
}
