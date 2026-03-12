/*
 * AXON Citation Tracking
 * Track source citations for entries and facts
 */

#include <lib9.h>
#include "axon.h"

/*
 * Create a new citation
 */
Citation*
citation_create(char *source_path, ulong offset, char *excerpt)
{
    Citation *c;

    c = emalloc9p(sizeof(Citation));
    if(c == nil)
        return nil;

    c->source_path = source_path ? estrdup9p(source_path) : nil;
    c->offset = offset;
    c->excerpt = excerpt ? estrdup9p(excerpt) : nil;

    return c;
}

/*
 * Free a citation
 */
void
citation_free(Citation *c)
{
    if(c == nil)
        return;

    free(c->source_path);
    free(c->excerpt);
    free(c);
}

/*
 * Validate citation (check if source exists)
 */
int
citation_validate(Citation *c)
{
    int fd;

    if(c == nil || c->source_path == nil)
        return 0;

    fd = open(c->source_path, OREAD);
    if(fd < 0)
        return 0;

    close(fd);
    return 1;
}

/*
 * Format citation as text
 */
char*
citation_format(Citation *c)
{
    char *buf;

    if(c == nil)
        return estrdup9p("[invalid citation]");

    if(c->source_path == nil)
        return estrdup9p("[unknown source]");

    if(c->excerpt != nil)
        buf = smprint("[%s:%ul] \"%s\"", c->source_path, c->offset, c->excerpt);
    else
        buf = smprint("[%s:%ul]", c->source_path, c->offset);

    return buf;
}
