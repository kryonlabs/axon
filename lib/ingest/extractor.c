/*
 * AXON Fact Extractor
 * Extract facts from parsed documents
 */

#include <lib9.h>
#include "axon.h"

/*
 * Extract facts from document text
 */
Fact**
extract_facts_from_doc(char *text, int *nfacts)
{
    /* Delegate to the main extract_facts function */
    return extract_facts(text, nfacts);
}

/*
 * Extract entities from text (placeholder)
 */
char**
extract_entities(char *text, int *nentities)
{
    if(nentities != nil)
        *nentities = 0;
    return nil;
}
