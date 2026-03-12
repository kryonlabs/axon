/*
 * AXON Semantic Memory
 * Store facts and entities
 */

#include <lib9.h>
#include "axon.h"

/*
 * Semantic memory entry
 */
typedef struct SemanticMemory {
    Fact *fact;
    struct SemanticMemory *next;
} SemanticMemory;

static SemanticMemory *semantic_memories = nil;

/*
 * Store semantic memory (fact)
 */
int
semantic_store(Fact *fact)
{
    SemanticMemory *sm;

    if(fact == nil)
        return -1;

    sm = emalloc9p(sizeof(SemanticMemory));
    if(sm == nil)
        return -1;

    sm->fact = fact;
    sm->next = semantic_memories;
    semantic_memories = sm;

    return 0;
}

/*
 * Retrieve semantic memories
 */
Fact**
semantic_retrieve(char *subject, int *nresults)
{
    /* Placeholder */
    if(nresults != nil)
        *nresults = 0;
    return nil;
}
