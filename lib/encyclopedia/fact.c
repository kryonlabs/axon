/*
 * AXON Fact Extraction
 * Extract facts and relationships from text
 */

#include <lib9.h>
#include "axon.h"

/*
 * Simple fact extraction from text
 * In full implementation, this would use NLP/AI
 */
Fact**
extract_facts(char *text, int *nfacts)
{
    Fact **facts;
    char *p, *line_start, *line_end;
    char *subject, *predicate, *object;
    int is, ip, io;
    int count;
    char line[512];

    if(text == nil || nfacts == nil)
        return nil;

    *nfacts = 0;
    facts = nil;
    count = 0;

    /* Simple pattern matching for "X is Y" statements */
    p = text;
    while(*p != '\0') {
        /* Skip whitespace */
        while(*p == ' ' || *p == '\t' || *p == '\n')
            p++;

        if(*p == '\0')
            break;

        /* Find end of line */
        line_start = p;
        line_end = strchr(p, '\n');
        if(line_end == nil)
            line_end = strchr(p, '\0');

        /* Copy line to buffer */
        is = line_end - line_start;
        if(is > sizeof(line) - 1)
            is = sizeof(line) - 1;
        memcpy(line, line_start, is);
        line[is] = '\0';

        /* Check for "X is Y" pattern */
        predicate = strstr(line, " is ");
        if(predicate != nil) {
            /* Extract subject */
            subject = line;
            ip = predicate - subject;
            predicate += 4;  /* Skip " is " */
            object = predicate;

            /* Create fact */
            if(count == 0) {
                facts = emalloc9p(sizeof(Fact*));
            } else {
                facts = erealloc9p(facts, (count + 1) * sizeof(Fact*));
            }

            facts[count] = fact_create(strdup(subject), strdup("is"),
                                       strdup(object));
            if(facts[count] != nil)
                count++;
        }

        p = line_end;
        if(*p == '\n')
            p++;
    }

    *nfacts = count;
    return facts;
}

/*
 * Create a new fact
 */
Fact*
fact_create(char *subject, char *predicate, char *object)
{
    Fact *f;

    if(subject == nil || predicate == nil || object == nil)
        return nil;

    f = emalloc9p(sizeof(Fact));
    if(f == nil)
        return nil;

    f->subject = subject;
    f->predicate = predicate;
    f->object = object;
    f->confidence = 0.5;
    f->sources = nil;
    f->nsources = 0;
    f->next = nil;

    return f;
}

/*
 * Check if two facts describe the same relationship
 */
int
fact_match(Fact *a, Fact *b)
{
    if(a == nil || b == nil)
        return 0;

    return (strcmp(a->subject, b->subject) == 0 &&
            strcmp(a->predicate, b->predicate) == 0 &&
            strcmp(a->object, b->object) == 0);
}

/*
 * Free a fact
 */
void
fact_free(Fact *f)
{
    int i;

    if(f == nil)
        return;

    free(f->subject);
    free(f->predicate);
    free(f->object);

    for(i = 0; i < f->nsources; i++) {
        if(f->sources[i] != nil) {
            free(f->sources[i]->source_path);
            free(f->sources[i]->excerpt);
            free(f->sources[i]);
        }
    }

    free(f->sources);
    free(f);
}
