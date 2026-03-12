/*
 * AXON Virtual File Implementations
 * Various helper files and special file handlers
 */

#include <lib9.h>
#include <9p.h>
#include "axon.h"
#include "axonfs.h"

/*
 * Generate confidence file content
 */
char*
generate_confidence_content(Axon *axon)
{
    Entry *e;
    char *buf;
    int len;
    int count;

    if(axon->entries == nil)
        return strdup("");

    /* Calculate total length needed */
    len = 0;
    count = 0;
    for(e = axon->entries; e != nil; e = e->next) {
        len += strlen(e->title) + 32;
        count++;
    }

    buf = emalloc9p(len + 1);
    buf[0] = '\0';

    for(e = axon->entries; e != nil; e = e->next) {
        strcat(buf, e->title);
        strcat(buf, ": ");
        strcat(buf, smprint("%.2f\n", e->confidence));
    }

    return buf;
}

/*
 * Read confidence directory entry
 */
void
read_confidence_file(Req *r, Axon *axon, char *title)
{
    Entry *e;
    char *buf;

    e = axon_find_entry(axon, title);
    if(e == nil) {
        readstr(r, "Entry not found\n");
        return;
    }

    buf = smprint("%.2f\n", e->confidence);
    readstr(r, buf);
    free(buf);
}

/*
 * Generate sources directory content
 */
char*
generate_sources_list(Axon *axon)
{
    char *buf;

    buf = smprint("AXON Sources Directory\n\n");
    /* Placeholder for future implementation */
    return buf;
}
