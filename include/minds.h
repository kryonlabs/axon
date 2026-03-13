#ifndef _MINDS_H_
#define _MINDS_H_

/*
 * AXON Multi-Mind Fact Extraction System
 * Multiple AI perspectives for robust knowledge extraction
 */

#include <lib9.h>
#include "axon.h"

/*
 * Mind types - different extraction strategies
 */
typedef enum MindType {
    MIND_LITERAL = 0,          /* Takes everything literally */
    MIND_SKEPTIC,              /* Doubts claims, wants sources */
    MIND_SYNTHESIZER,          /* Combines related concepts */
    MIND_PATTERN_MATCHER,      /* Finds relationships */
    MIND_QUESTIONER,           /* Generates questions to verify */
    MIND_MAX
} MindType;

/*
 * Mind flags - special states
 */
enum {
    MFLAG_NONE = 0,
    MFLAG_NEEDS_VERIFICATION = (1<<0),   /* Skeptic flag */
    MFLAG_CONTRADICTORY = (1<<1),        /* Contradicts other minds */
    MFLAG_RELATIONSHIP = (1<<2),         /* Found relationship */
    MFLAG_PRINCIPLE = (1<<3),            /* Abstract principle */
    MFLAG_QUESTION = (1<<4),             /* Unanswered question */
    MFLAG_GAP = (1<<5),                  /* Knowledge gap */
};

/*
 * Mind fact - extracted fact with mind-specific metadata
 */
typedef struct MindFact {
    char *subject;            /* Subject entity */
    char *predicate;          /* Relationship/property */
    char *object;            /* Object/value */
    double confidence;        /* This mind's confidence (0.0-1.0) */
    int flags;               /* MFLAG_* values */
    char *explanation;       /* Why this confidence/flags */
    char *evidence;          /* Evidence excerpt from source */
} MindFact;

/*
 * Mind result - output from a mind processing an entry
 */
typedef struct MindResult {
    MindType mind_type;       /* Which mind produced this */
    Entry *source_entry;      /* Entry that was processed */
    MindFact **facts;         /* Extracted facts */
    int nfacts;              /* Number of facts */
    char **questions;         /* Questions raised (for Questioner) */
    int nquestions;          /* Number of questions */
    double avg_confidence;    /* Average confidence across facts */
    int processing_time_ms;   /* Time taken to process */
} MindResult;

/*
 * Mind - AI extraction agent
 */
typedef struct Mind {
    char *name;              /* Mind name */
    MindType type;           /* Mind type */
    char *description;       /* What this mind does */
    char *prompt;            /* System prompt for LLM */
    double default_confidence; /* Default confidence for facts */
    int enabled;             /* Whether this mind is active */

    /*
     * LLM backend for this mind
     * Can be nil if using fallback extraction
     */
    struct LLMBackend *llm;
    char *model_override;    /* Specific model override for this mind */

    /*
     * Extract facts from an entry
     * Returns allocated MindResult that must be freed
     */
    MindResult* (*extract)(struct Mind *mind, Entry *entry);

    /*
     * Free mind-specific resources
     */
    void (*cleanup)(struct Mind *mind);
} Mind;

/*
 * Consensus fact - aggregated fact from multiple minds
 */
typedef struct ConsensusFact {
    char *subject;
    char *predicate;
    char *object;
    double consensus_confidence;  /* Average across minds */
    int source_count;            /* How many minds extracted this */
    int mind_agreement;          /* Bitmask of agreeing minds */
    double *mind_confidences;    /* Per-mind confidence [MIND_MAX] */
    char **evidence;             /* Evidence per mind */
    int nevidence;
    int disagreement_flags;      /* Minds that strongly disagree */
    char *contradiction_reason;  /* Why minds disagree */
    vlong last_updated;          /* Timestamp */
} ConsensusFact;

/*
 * Contradiction record - conflict between minds
 */
typedef struct Contradiction {
    char *fact_key;             /* "subject:predicate:object" */
    ConsensusFact *consensus;   /* Current consensus state */
    MindFact **conflicting_claims; /* Conflicting facts */
    int nclaims;
    char *resolution;           /* How resolved, if any */
    int resolved;               /* 0 = pending, 1 = resolved */
    vlong discovered_at;        /* When discovered */
    vlong resolved_at;          /* When resolved */
} Contradiction;

/*
 * Mind management functions
 */

/* Create a mind of given type */
Mind* mind_create(MindType type);

/* Free a mind and its resources */
void mind_free(Mind *mind);

/* Get mind by type */
Mind* mind_get_by_type(MindType type);

/* Get all active minds */
Mind** mind_get_all(int *nminds);

/* Enable/disable a mind */
int mind_set_enabled(Mind *mind, int enabled);

/*
 * Mind extraction functions
 */

/* Process an entry through a mind */
MindResult* mind_extract(Mind *mind, Entry *entry);

/* Process entry through all active minds */
MindResult** mind_extract_all(Entry *entry, int *nresults);

/* Free mind result */
void mind_result_free(MindResult *result);

/* Free array of mind results */
void mind_results_free(MindResult **results, int nresults);

/*
 * Mind fact functions
 */

/* Create a mind fact */
MindFact* mind_fact_create(char *subject, char *predicate, char *object);

/* Free a mind fact */
void mind_fact_free(MindFact *fact);

/* Check if two mind facts match (same s:p:o) */
int mind_fact_match(MindFact *a, MindFact *b);

/* Generate fact key for indexing */
char* mind_fact_key(MindFact *fact);

/*
 * Consensus functions
 */

/* Calculate consensus from multiple mind results */
ConsensusFact** consensus_build(MindResult **results, int nresults, int *nconsensus);

/* Update consensus with new mind results */
int consensus_update(ConsensusFact *consensus, MindResult **results, int nresults);

/* Find contradictions among mind results */
Contradiction** contradictions_find(MindResult **results, int nresults, int *ncontra);

/* Resolve a contradiction */
int contradiction_resolve(Contradiction *con, char *resolution);

/* Free consensus fact */
void consensus_fact_free(ConsensusFact *fact);

/* Free contradiction */
void contradiction_free(Contradiction *con);

/*
 * Mind type utilities
 */

/* Get mind type name */
char* mind_type_name(MindType type);

/* Get mind description */
char* mind_type_description(MindType type);

/* Get default prompt for mind type */
char* mind_type_prompt(MindType type);

/*
 * LLM response parsing utilities
 */

/* Parse LLM response text into MindResult */
MindResult* parse_llm_response(char *response_text, MindType mind_type,
                               Entry *source_entry);

/* Build extraction prompt for a mind */
char* build_extraction_prompt(Mind *mind, Entry *entry);

/* Format entry content for LLM prompt */
char* format_entry_for_prompt(Entry *entry);

#endif /* _MINDS_H_ */
