/*
 * AXON Fact Store
 * Manages storage and retrieval of extracted facts
 */

#include <lib9.h>
#include <9p.h>  /* For vlong */
#include "axon.h"
#include "minds.h"
#include "facts.h"

/*
 * Initialize fact store
 */
FactStore*
fact_store_init(const char *data_path)
{
    FactStore *store;
    char *path;

    if(data_path == nil)
        return nil;

    store = emalloc9p(sizeof(FactStore));
    store->data_path = estrdup9p(data_path);
    store->index = nil;
    store->dirty = 1;

    return store;
}

/*
 * Close and cleanup fact store
 */
void
fact_store_close(FactStore *store)
{
    if(store == nil)
        return;

    if(store->index != nil)
        fact_index_free(store->index);

    free(store->data_path);
    free(store);
}

/*
 * Generate path for mind fact file
 */
char*
fact_mind_file_path(FactStore *store, MindType mind_type, const char *entry_title)
{
    char *mind_name;
    char *path;

    if(store == nil || entry_title == nil)
        return nil;

    mind_name = mind_type_name(mind_type);
    if(mind_name == nil)
        return nil;

    path = smprint("%s/minds/%s/facts/%s.facts",
                   store->data_path, mind_name, entry_title);

    return path;
}

/*
 * Generate path for consensus fact file
 */
char*
fact_consensus_file_path(FactStore *store, const char *fact_key)
{
    char *path;

    if(store == nil || fact_key == nil)
        return nil;

    path = smprint("%s/facts/consensus/%s.facts", store->data_path, fact_key);

    return path;
}

/*
 * Generate subject directory path
 */
char*
fact_subject_dir_path(FactStore *store, const char *subject)
{
    char *path;

    if(store == nil || subject == nil)
        return nil;

    path = smprint("%s/facts/by_subject/%s", store->data_path, subject);

    return path;
}

/*
 * Generate predicate directory path
 */
char*
fact_predicate_dir_path(FactStore *store, const char *predicate)
{
    char *path;

    if(store == nil || predicate == nil)
        return nil;

    path = smprint("%s/facts/by_predicate/%s", store->data_path, predicate);

    return path;
}

/*
 * Serialize a mind fact to a line
 * Format: subject|predicate|object|confidence|flags|evidence
 */
char*
fact_serialize_mind_fact(MindFact *fact)
{
    char *line;

    if(fact == nil)
        return nil;

    line = smprint("%s|%s|%s|%.3f|%d|%s",
                   fact->subject,
                   fact->predicate,
                   fact->object,
                   fact->confidence,
                   fact->flags,
                   fact->evidence != nil ? fact->evidence : "");

    return line;
}

/*
 * Parse a fact line from storage
 */
MindFact*
fact_parse_line(char *line)
{
    MindFact *fact;
    char *fields[6];
    int nfields;
    char *p, *start;
    int i;

    if(line == nil)
        return nil;

    /* Parse pipe-delimited fields */
    nfields = 0;
    start = line;
    for(p = line; *p != '\0' && nfields < 6; p++) {
        if(*p == '|') {
            *p = '\0';
            fields[nfields++] = start;
            start = p + 1;
        }
    }
    if(nfields < 5)
        return nil;  /* Need at least 5 fields */

    /* Last field */
    fields[nfields++] = start;

    fact = emalloc9p(sizeof(MindFact));
    fact->subject = estrdup9p(fields[0]);
    fact->predicate = estrdup9p(fields[1]);
    fact->object = estrdup9p(fields[2]);
    fact->confidence = atof(fields[3]);
    fact->flags = atoi(fields[4]);
    fact->evidence = (nfields > 5 && strlen(fields[5]) > 0) ?
        estrdup9p(fields[5]) : nil;
    fact->explanation = nil;  /* Not serialized */

    return fact;
}

/*
 * Save a mind fact to storage
 */
int
fact_store_mind_fact(FactStore *store, MindType mind_type, MindFact *fact, Entry *source)
{
    char *path;
    int fd;
    char *line;
    int len;

    if(store == nil || fact == nil || source == nil)
        return -1;

    /* Get file path for this mind and entry */
    path = fact_mind_file_path(store, mind_type, source->title);
    if(path == nil)
        return -1;

    /* Serialize fact */
    line = fact_serialize_mind_fact(fact);
    if(line == nil) {
        free(path);
        return -1;
    }

    /* Open file for append */
    fd = open(path, OWRITE|OAPPEND|OCREATE);
    if(fd < 0) {
        free(path);
        free(line);
        return -1;
    }

    /* Write fact */
    len = strlen(line);
    if(write(fd, line, len) != len || write(fd, "\n", 1) != 1) {
        close(fd);
        free(path);
        free(line);
        return -1;
    }

    close(fd);
    free(path);
    free(line);

    /* Mark index as dirty */
    store->dirty = 1;

    return 0;
}

/*
 * Load facts for a specific mind
 */
MindFact**
fact_load_mind_facts(FactStore *store, MindType mind_type, const char *entry_title, int *nfacts)
{
    char *path;
    int fd;
    Dir *d;
    char *buf;
    ulong n;
    MindFact **facts;
    int count;
    char *line, *newline;

    if(store == nil || entry_title == nil) {
        if(nfacts != nil)
            *nfacts = 0;
        return nil;
    }

    path = fact_mind_file_path(store, mind_type, entry_title);
    if(path == nil) {
        if(nfacts != nil)
            *nfacts = 0;
        return nil;
    }

    fd = open(path, OREAD);
    free(path);
    if(fd < 0) {
        if(nfacts != nil)
            *nfacts = 0;
        return nil;
    }

    d = dirfstat(fd);
    if(d == nil) {
        close(fd);
        if(nfacts != nil)
            *nfacts = 0;
        return nil;
    }

    n = d->length;
    free(d);

    if(n == 0) {
        close(fd);
        if(nfacts != nil)
            *nfacts = 0;
        return nil;
    }

    buf = emalloc9p(n + 1);
    if(readn(fd, buf, n) != n) {
        free(buf);
        close(fd);
        if(nfacts != nil)
            *nfacts = 0;
        return nil;
    }
    close(fd);
    buf[n] = '\0';

    /* Parse lines */
    facts = nil;
    count = 0;
    line = buf;
    while(line != nil && *line != '\0') {
        newline = strchr(line, '\n');
        if(newline != nil)
            *newline = '\0';

        if(strlen(line) > 0) {
            MindFact *fact = fact_parse_line(line);
            if(fact != nil) {
                count++;
                facts = erealloc9p(facts, count * sizeof(MindFact*));
                facts[count - 1] = fact;
            }
        }

        line = (newline != nil) ? newline + 1 : nil;
    }

    free(buf);

    if(nfacts != nil)
        *nfacts = count;

    return facts;
}

/*
 * Serialize a consensus fact to a multi-line format
 */
char*
fact_serialize_consensus_fact(ConsensusFact *fact)
{
    char *buf;
    int i;

    if(fact == nil)
        return nil;

    buf = smprint("Subject: %s\n", fact->subject);
    buf = eappend9p(buf, smprint("Predicate: %s\n", fact->predicate));
    buf = eappend9p(buf, smprint("Object: %s\n", fact->object));
    buf = eappend9p(buf, smprint("Consensus Confidence: %.3f\n", fact->consensus_confidence));
    buf = eappend9p(buf, smprint("Source Count: %d\n", fact->source_count));
    buf = eappend9p(buf, smprint("Mind Agreement: 0x%X\n", fact->mind_agreement));
    buf = eappend9p(buf, smprint("Last Updated: %lld\n", fact->last_updated));

    /* Add per-mind confidences */
    for(i = 0; i < MIND_MAX; i++) {
        if(fact->mind_confidences[i] > 0.0) {
            buf = eappend9p(buf, smprint("Mind %s: %.3f\n",
                           mind_type_name((MindType)i), fact->mind_confidences[i]));
        }
    }

    /* Add evidence */
    if(fact->nevidence > 0) {
        buf = eappend9p(buf, estrdup9p("Evidence:\n"));
        for(i = 0; i < fact->nevidence; i++) {
            buf = eappend9p(buf, smprint("  - %s\n", fact->evidence[i]));
        }
    }

    return buf;
}

/*
 * Save a consensus fact
 */
int
fact_store_consensus_fact(FactStore *store, ConsensusFact *fact)
{
    char *path, *key, *line;
    int fd, len;

    if(store == nil || fact == nil)
        return -1;

    /* Generate key */
    key = smprint("%s:%s:%s", fact->subject, fact->predicate, fact->object);

    path = fact_consensus_file_path(store, key);
    free(key);
    if(path == nil)
        return -1;

    line = fact_serialize_consensus_fact(fact);
    if(line == nil) {
        free(path);
        return -1;
    }

    fd = create(path, OWRITE, 0644);
    if(fd < 0) {
        free(path);
        free(line);
        return -1;
    }

    len = strlen(line);
    if(write(fd, line, len) != len) {
        close(fd);
        free(path);
        free(line);
        return -1;
    }

    close(fd);
    free(path);
    free(line);

    store->dirty = 1;

    return 0;
}

/*
 * Load consensus facts
 */
ConsensusFact**
fact_load_consensus(FactStore *store, int *nfacts)
{
    /* Placeholder - would scan consensus directory */
    if(nfacts != nil)
        *nfacts = 0;
    return nil;
}

/*
 * Get current index
 */
FactIndex*
fact_store_get_index(FactStore *store)
{
    if(store == nil)
        return nil;

    if(store->dirty || store->index == nil)
        fact_store_reindex(store);

    return store->index;
}
