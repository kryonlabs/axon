/*
 * AXON Episodic Memory
 * Store conversation/session history
 */

#include <lib9.h>
#include "axon.h"

/*
 * Episodic memory entry
 */
typedef struct EpisodicEntry {
    ulong timestamp;
    char *session_id;
    char *content;
    struct EpisodicEntry *next;
} EpisodicEntry;

static EpisodicEntry *episodic_memories = nil;

/*
 * Store episodic memory
 */
int
episodic_store(char *session_id, char *content)
{
    EpisodicEntry *e;

    e = emalloc9p(sizeof(EpisodicEntry));
    if(e == nil)
        return -1;

    e->timestamp = time(0);
    e->session_id = session_id ? estrdup9p(session_id) : estrdup9p("default");
    e->content = content ? estrdup9p(content) : nil;
    e->next = episodic_memories;
    episodic_memories = e;

    return 0;
}

/*
 * Retrieve episodic memories
 */
char**
episodic_retrieve(char *session_id, int *nentries)
{
    /* Placeholder */
    if(nentries != nil)
        *nentries = 0;
    return nil;
}
