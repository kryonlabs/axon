/*
 * AXON Search Functionality
 * Text search over encyclopedia entries
 */

#include <lib9.h>
#include <9p.h>
#include "axon.h"

/*
 * Simple text search
 */
SearchResult*
search_entries(Axon *axon, char *query, int *nresults)
{
    SearchResult *results;
    Entry *e;
    int count;
    int i;

    if(axon == nil || query == nil || nresults == nil)
        return nil;

    count = 0;
    results = nil;

    /* Search through all entries */
    for(e = axon->entries; e != nil; e = e->next) {
        int title_match, content_match;
        double relevance;
        char *excerpt;

        title_match = (strstr(e->title, query) != nil);
        content_match = (strstr(e->content, query) != nil);

        if(!title_match && !content_match)
            continue;

        /* Calculate relevance score */
        relevance = 0.0;
        if(title_match)
            relevance += 0.8;
        if(content_match)
            relevance += 0.5;

        /* Boost by confidence */
        relevance *= e->confidence;

        /* Extract excerpt */
        excerpt = extract_excerpt(e, query);
        if(excerpt == nil)
            excerpt = estrdup9p("");

        /* Add to results */
        count++;
        if(count == 1) {
            results = emalloc9p(sizeof(SearchResult));
        } else {
            results = erealloc9p(results, count * sizeof(SearchResult));
        }

        results[count - 1].entry = e;
        results[count - 1].relevance = relevance;
        results[count - 1].excerpt = excerpt;
    }

    *nresults = count;
    return results;
}

/*
 * Extract excerpt around search term
 */
char*
extract_excerpt(Entry *e, char *query)
{
    char *match;
    char *start, *end;
    char *excerpt;
    int len;
    int context = 50;  /* Characters before and after match */

    if(e == nil || query == nil)
        return nil;

    /* Find first match in content */
    match = strstr(e->content, query);
    if(match == nil)
        match = strstr(e->title, query);

    if(match == nil)
        return nil;

    /* Calculate boundaries */
    start = match - context;
    if(start < e->content)
        start = e->content;

    end = match + strlen(query) + context;
    if(end > e->content + strlen(e->content))
        end = e->content + strlen(e->content);

    /* Extract excerpt */
    len = end - start;
    excerpt = emalloc9p(len + 1);
    memcpy(excerpt, start, len);
    excerpt[len] = '\0';

    return excerpt;
}

/*
 * Format search results as text
 */
char*
format_search_results(SearchResult *results, int nresults)
{
    char *buf;
    int i;
    int len;
    int pos;

    if(results == nil || nresults <= 0)
        return estrdup9p("No results\n");

    /* Calculate total length needed */
    len = 0;
    for(i = 0; i < nresults; i++) {
        len += strlen(results[i].entry->title) + 64;
        if(results[i].excerpt != nil)
            len += strlen(results[i].excerpt);
    }

    buf = emalloc9p(len + 1);
    pos = 0;

    for(i = 0; i < nresults; i++) {
        pos += snprint(buf + pos, len - pos,
                      "%d. %s (relevance: %.2f, confidence: %.2f)\n",
                      i + 1,
                      results[i].entry->title,
                      results[i].relevance,
                      results[i].entry->confidence);

        if(results[i].excerpt != nil) {
            pos += snprint(buf + pos, len - pos, "   %s\n",
                          results[i].excerpt);
        }
    }

    return buf;
}

/*
 * Free search results
 */
void
search_free(SearchResult *results, int nresults)
{
    int i;

    if(results == nil)
        return;

    for(i = 0; i < nresults; i++) {
        if(results[i].excerpt != nil)
            free(results[i].excerpt);
    }

    free(results);
}
