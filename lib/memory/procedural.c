/*
 * AXON Procedural Memory
 * Store skills and procedures
 */

#include <lib9.h>
#include "axon.h"

/*
 * Procedural memory entry (skill)
 */
typedef struct ProceduralMemory {
    char *name;
    char *description;
    char *implementation;
    struct ProceduralMemory *next;
} ProceduralMemory;

static ProceduralMemory *procedural_memories = nil;

/*
 * Store procedural memory (skill)
 */
int
procedural_store(char *name, char *description, char *implementation)
{
    ProceduralMemory *pm;

    pm = emalloc9p(sizeof(ProceduralMemory));
    if(pm == nil)
        return -1;

    pm->name = name ? estrdup9p(name) : nil;
    pm->description = description ? estrdup9p(description) : nil;
    pm->implementation = implementation ? estrdup9p(implementation) : nil;
    pm->next = procedural_memories;
    procedural_memories = pm;

    return 0;
}

/*
 * Retrieve procedural memory
 */
char*
procedural_retrieve(char *name)
{
    ProceduralMemory *pm;

    for(pm = procedural_memories; pm != nil; pm = pm->next) {
        if(pm->name != nil && strcmp(pm->name, name) == 0) {
            return pm->implementation;
        }
    }

    return nil;
}
