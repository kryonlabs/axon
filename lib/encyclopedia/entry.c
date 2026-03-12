/*
 * AXON Entry Management
 * Creation and management of encyclopedia entries
 */

#include <lib9.h>
#include "axon.h"

/*
 * Create a new entry
 */
Entry*
entry_create(char *title, char *content)
{
    Entry *e;

    if(title == nil || content == nil)
        return nil;

    e = emalloc9p(sizeof(Entry));
    if(e == nil)
        return nil;

    e->title = estrdup9p(title);
    e->content = estrdup9p(content);
    e->confidence = 0.5;  /* Default confidence */
    e->sources = nil;
    e->nsources = 0;
    e->qid = 0;
    e->next = nil;

    return e;
}

/*
 * Add a citation to an entry
 */
void
entry_add_citation(Entry *e, Citation *c)
{
    Citation **new_sources;

    if(e == nil || c == nil)
        return;

    e->nsources++;
    new_sources = erealloc9p(e->sources,
        e->nsources * sizeof(Citation*));
    if(new_sources == nil) {
        e->nsources--;
        return;
    }

    e->sources = new_sources;
    e->sources[e->nsources - 1] = c;
}

/*
 * Set entry confidence score
 */
void
entry_set_confidence(Entry *e, double conf)
{
    if(e == nil)
        return;

    if(conf < 0.0) conf = 0.0;
    if(conf > 1.0) conf = 1.0;
    e->confidence = conf;
}

/*
 * Free an entry
 */
void
entry_free(Entry *e)
{
    int i;

    if(e == nil)
        return;

    free(e->title);
    free(e->content);

    for(i = 0; i < e->nsources; i++) {
        if(e->sources[i] != nil) {
            free(e->sources[i]->source_path);
            free(e->sources[i]->excerpt);
            free(e->sources[i]);
        }
    }

    free(e->sources);
    free(e);
}

/*
 * Save entry to disk
 */
int
entry_save(Entry *e, const char *data_path)
{
    char *path;
    int fd;
    char *buf;
    int len;

    if(e == nil || data_path == nil)
        return -1;

    /* Create full path to entry file */
    path = smprint("%s/encyclopedia/entries/%s", data_path, e->title);
    if(path == nil)
        return -1;

    /* Open file for writing */
    fd = create(path, OWRITE, 0644);
    free(path);
    if(fd < 0)
        return -1;

    /* Write entry content */
    buf = smprint("# %s\n\nConfidence: %.2f\n\n%s\n",
                  e->title, e->confidence, e->content);
    if(buf == nil) {
        close(fd);
        return -1;
    }

    len = strlen(buf);
    if(write(fd, buf, len) != len) {
        free(buf);
        close(fd);
        return -1;
    }

    free(buf);
    close(fd);
    return 0;
}

/*
 * Load entry from disk
 */
Entry*
entry_load(const char *path)
{
    Entry *e;
    int fd;
    Dir *d;
    char *buf;
    ulong n;
    char *title_start, *title_end;
    char *content;

    fd = open(path, OREAD);
    if(fd < 0)
        return nil;

    d = dirfstat(fd);
    if(d == nil) {
        close(fd);
        return nil;
    }

    n = d->length;
    free(d);

    buf = emalloc9p(n + 1);
    if(readn(fd, buf, n) != n) {
        free(buf);
        close(fd);
        return nil;
    }
    close(fd);
    buf[n] = '\0';

    /* Extract title from markdown heading */
    title_start = buf;
    if(*title_start == '#')
        title_start++;
    while(*title_start == ' ')
        title_start++;

    title_end = strchr(title_start, '\n');
    if(title_end == nil) {
        free(buf);
        return nil;
    }

    /* Create entry */
    e = emalloc9p(sizeof(Entry));
    e->title = emalloc9p(title_end - title_start + 1);
    memcpy(e->title, title_start, title_end - title_start);
    e->title[title_end - title_start] = '\0';

    /* Find content (after metadata section) */
    content = strstr(title_end, "\n\n");
    if(content != nil)
        content += 2;
    else
        content = title_end + 1;

    e->content = estrdup9p(content);
    e->confidence = 0.5;
    e->sources = nil;
    e->nsources = 0;
    e->qid = 0;
    e->next = nil;

    free(buf);
    return e;
}
