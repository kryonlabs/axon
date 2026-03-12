#ifndef _FACTS_H_
#define _FACTS_H_

/*
 * AXON Fact Storage and Query System
 * Efficient storage and retrieval of extracted facts
 */

#include <lib9.h>
#include "axon.h"
#include "minds.h"

/*
 * Fact storage format:
 * subject|predicate|object|confidence|evidence
 *
 * Example:
 * sound_waves|are|mechanical waves|0.95|source states it clearly
 * sound_waves|speed|343 m/s in air at 20°C|0.90|measured property
 */

#define FACT_DELIMITER "|"
#define FACT_FILE_EXTENSION ".facts"

/*
 * Fact query - search criteria
 */
typedef struct FactQuery {
    char *subject;           /* Filter by subject (nil = any) */
    char *predicate;         /* Filter by predicate (nil = any) */
    char *object;           /* Filter by object (nil = any) */
    double min_confidence;   /* Minimum confidence (0.0 = any) */
    MindType mind_type;     /* Filter by mind type (MIND_MAX = any) */
    int include_mind_specific;  /* Include individual mind facts */
    int include_consensus;      /* Include consensus facts */
    int limit;              /* Max results (0 = unlimited) */
    int offset;             /* Skip N results (for pagination) */
} FactQuery;

/*
 * Fact index entry - for fast lookups
 */
typedef struct FactIndexEntry {
    char *key;              /* "subject:predicate:object" */
    char *subject;
    char *predicate;
    char *object;
    char *file_path;        /* Path to fact file */
    ulong offset;           /* Offset in file */
    int mind_mask;          /* Bitmask of minds that have this fact */
    double consensus_confidence;  /* Current consensus confidence */
    vlong last_updated;     /* Timestamp */
} FactIndexEntry;

/*
 * Fact index - maintains multiple indices
 */
typedef struct FactIndex {
    FactIndexEntry **by_subject;    /* Sorted by subject */
    int nby_subject;
    FactIndexEntry **by_predicate;  /* Sorted by predicate */
    int nby_predicate;
    FactIndexEntry **consensus;     /* Consensus facts */
    int nconsensus;
    vlong last_built;              /* When index was built */
} FactIndex;

/*
 * Fact store - manages all fact storage
 */
typedef struct FactStore {
    char *data_path;         /* Path to facts directory */
    FactIndex *index;        /* Current index */
    int dirty;              /* Index needs rebuild */
} FactStore;

/*
 * Fact storage functions
 */

/* Initialize fact store */
FactStore* fact_store_init(const char *data_path);

/* Close and cleanup fact store */
void fact_store_close(FactStore *store);

/* Save a mind fact to storage */
int fact_store_mind_fact(FactStore *store, MindType mind_type, MindFact *fact, Entry *source);

/* Save a consensus fact */
int fact_store_consensus_fact(FactStore *store, ConsensusFact *fact);

/* Load facts for a specific mind */
MindFact** fact_load_mind_facts(FactStore *store, MindType mind_type, const char *entry_title, int *nfacts);

/* Load consensus facts */
ConsensusFact** fact_load_consensus(FactStore *store, int *nfacts);

/* Rebuild the fact index */
int fact_store_reindex(FactStore *store);

/* Get current index */
FactIndex* fact_store_get_index(FactStore *store);

/*
 * Fact query functions
 */

/* Query facts with criteria */
MindFact** fact_query_mind_facts(FactStore *store, FactQuery *query, int *nresults);

/* Query consensus facts */
ConsensusFact** fact_query_consensus(FactStore *store, FactQuery *query, int *nresults);

/* Query all facts (both mind and consensus) */
void** fact_query_all(FactStore *store, FactQuery *query, int *nresults, int *types);

/* Free query results */
void fact_query_free(void **results, int nresults);

/*
 * Fact indexing functions
 */

/* Build index from stored facts */
int fact_index_build(FactStore *store);

/* Add entry to index */
int fact_index_add(FactIndex *index, FactIndexEntry *entry);

/* Lookup by subject */
FactIndexEntry** fact_index_by_subject(FactIndex *index, const char *subject, int *nresults);

/* Lookup by predicate */
FactIndexEntry** fact_index_by_predicate(FactIndex *index, const char *predicate, int *nresults);

/* Search consensus facts */
FactIndexEntry** fact_index_consensus(FactIndex *index, const char *subject, const char *predicate, int *nresults);

/* Free index */
void fact_index_free(FactIndex *index);

/* Free index entry */
void fact_index_entry_free(FactIndexEntry *entry);

/*
 * Fact file operations
 */

/* Generate path for mind fact file */
char* fact_mind_file_path(FactStore *store, MindType mind_type, const char *entry_title);

/* Generate path for consensus fact file */
char* fact_consensus_file_path(FactStore *store, const char *fact_key);

/* Generate subject directory path */
char* fact_subject_dir_path(FactStore *store, const char *subject);

/* Generate predicate directory path */
char* fact_predicate_dir_path(FactStore *store, const char *predicate);

/*
 * Fact parsing and serialization
 */

/* Parse a fact line from storage */
MindFact* fact_parse_line(char *line);

/* Serialize a mind fact to a line */
char* fact_serialize_mind_fact(MindFact *fact);

/* Serialize a consensus fact to a multi-line format */
char* fact_serialize_consensus_fact(ConsensusFact *fact);

/* Parse a consensus fact from lines */
ConsensusFact* fact_parse_consensus_fact(char **lines, int nlines);

#endif /* _FACTS_H_ */
