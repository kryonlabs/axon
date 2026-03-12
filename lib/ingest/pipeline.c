/*
 * AXON Ingestion Pipeline
 * Complete pipeline for document ingestion
 */

#include <lib9.h>
#include "axon.h"

/*
 * Ingest a document
 */
int
ingest_document(Axon *axon, const char *path)
{
    char *content;
    Entry *e;
    char *title;
    char *ext;

    if(axon == nil || path == nil)
        return -1;

    /* Determine file type from extension */
    ext = strrchr(path, '.');
    if(ext != nil) {
        ext++;
        if(strcmp(ext, "pdf") == 0) {
            content = parse_pdf(path);
        } else if(strcmp(ext, "md") == 0 || strcmp(ext, "markdown") == 0) {
            content = parse_markdown(path);
        } else {
            content = parse_text(path);
        }
    } else {
        content = parse_text(path);
    }

    if(content == nil)
        return -1;

    /* Create entry from filename */
    title = strrchr(path, '/');
    if(title != nil)
        title++;
    else
        title = (char*)path;

    /* Remove extension from title */
    {
        char *dot = strrchr(title, '.');
        if(dot != nil) {
            char *t = estrdup9p(title);
            t[dot - title] = '\0';
            e = entry_create(t, content);
            free(t);
        } else {
            e = entry_create(title, content);
        }
    }

    free(content);

    if(e == nil)
        return -1;

    return axon_add_entry(axon, e);
}
