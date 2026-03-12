#ifndef _AXON_H_
#define _AXON_H_

/*
 * AXON - Encyclopedia Filesystem for TaijiOS
 * Core data structures and types
 */

#include <lib9.h>

/*
 * Forward declarations
 */
typedef struct Axon Axon;
typedef struct Entry Entry;
typedef struct Fact Fact;
typedef struct Citation Citation;
typedef struct SearchResult SearchResult;

/*
 * Citation - Reference to a source document
 */
struct Citation {
    char *source_path;        /* Path to source document */
    ulong offset;            /* Offset in document */
    char *excerpt;           /* Relevant excerpt */
};

/*
 * Entry - Encyclopedia knowledge entry
 */
struct Entry {
    char *title;              /* Entry title */
    char *content;            /* Entry content (markdown) */
    double confidence;        /* Confidence score (0.0-1.0) */
    Citation **sources;       /* Source citations */
    int nsources;
    ulong qid;               /* 9P QID */
    Entry *next;             /* Linked list for storage */
};

/*
 * Fact - Extracted fact/relationship
 */
struct Fact {
    char *subject;            /* Subject entity */
    char *predicate;          /* Relationship */
    char *object;            /* Object/value */
    double confidence;
    Citation **sources;
    int nsources;
    Fact *next;              /* Linked list for storage */
};

/*
 * SearchResult - Search result with relevance
 */
struct SearchResult {
    Entry *entry;
    double relevance;
    char *excerpt;
};

/*
 * Axon - Main AXON state
 */
struct Axon {
    char *root_path;           /* Path to data directory */
    Entry *entries;            /* Linked list of entries */
    Fact *facts;              /* Linked list of facts */
    int nentries;
    int nfacts;
    char *last_query;         /* Last search query */
    void *fs_tree;            /* 9P file tree */
};

/*
 * Entry management
 */
Entry* entry_create(char *title, char *content);
void entry_add_citation(Entry *e, Citation *c);
void entry_set_confidence(Entry *e, double conf);
void entry_free(Entry *e);
int entry_save(Entry *e, const char *data_path);
Entry* entry_load(const char *path);

/*
 * Fact extraction and management
 */
Fact** extract_facts(char *text, int *nfacts);
Fact* fact_create(char *subject, char *predicate, char *object);
int fact_match(Fact *a, Fact *b);
void fact_free(Fact *f);

/*
 * Search
 */
SearchResult* search_entries(Axon *axon, char *query, int *nresults);
char* format_search_results(SearchResult *results, int nresults);
void search_free(SearchResult *results, int nresults);

/*
 * AXON initialization
 */
Axon* axon_init(const char *data_path);
void axon_cleanup(Axon *axon);
int axon_add_entry(Axon *axon, Entry *e);
Entry* axon_find_entry(Axon *axon, const char *title);

#endif /* _AXON_H_ */
