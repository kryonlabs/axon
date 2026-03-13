/*
 * AXON File Handlers
 * Implements read/write operations for virtual files
 */

#include <lib9.h>
#include <9p.h>
#include <fcntl.h>
#include "axon.h"
#include "axonfs.h"
#include "minds.h"
#include "facts.h"

/*
 * Forward declarations for mind functions
 */
extern Mind** mind_get_all(int *nminds);
extern Mind** axon_get_minds(Axon *axon, int *nminds);
extern int axon_process_entry_with_minds(Axon *axon, Entry *e);
extern MindResult** mind_extract_all(Entry *entry, int *nresults);
extern ConsensusFact** consensus_build(MindResult **results, int nresults, int *nconsensus);
extern Contradiction** contradictions_find(MindResult **results, int nresults, int *ncontra);
extern void mind_result_free(MindResult *result);
extern void mind_results_free(MindResult **results, int nresults);
extern void contradiction_free(Contradiction *con);
extern char* mind_type_name(MindType type);

/*
 * Read from status file
 */
void
read_status(Req *r)
{
    char *buf;
    int len;
    Axon *axon;

    axon = r->fid->aux;
    if(axon == nil) {
        readstr(r, "AXON Encyclopedia Filesystem\nNo entries\n");
        return;
    }

    /* Format status information */
    len = 512;
    buf = emalloc9p(len);

    snprint(buf, len,
            "AXON Encyclopedia Filesystem\n"
            "Entries: %d\n"
            "Facts: %d\n"
            "Last query: %s\n"
            "Data path: %s\n",
            axon->nentries,
            axon->nfacts,
            axon->last_query ? axon->last_query : "(none)",
            axon->root_path ? axon->root_path : "(unknown)");

    readstr(r, buf);
    free(buf);
}

/*
 * Read from control file (echo commands)
 */
void
read_ctl(Req *r)
{
    readstr(r, "AXON control file\nCommands: add_entry, add_source, search, status, sync\n");
}

/*
 * Write to search query file
 */
void
write_search_query(Req *r, Axon *axon)
{
    char *query;

    query = emalloc9p(r->ifcall.count + 1);
    memcpy(query, r->ifcall.data, r->ifcall.count);
    query[r->ifcall.count] = '\0';

    /* Strip newline */
    if(r->ifcall.count > 0 && query[r->ifcall.count - 1] == '\n')
        query[r->ifcall.count - 1] = '\0';

    free(axon->last_query);
    axon->last_query = query;

    r->ofcall.count = r->ifcall.count;
    respond(r, nil);
}

/*
 * Read from search results file
 */
void
read_search_results(Req *r, Axon *axon)
{
    char *results;
    SearchResult *sr;
    int n;

    if(axon->last_query == nil) {
        readstr(r, "No search query\n");
        return;
    }

    sr = search_entries(axon, axon->last_query, &n);
    if(sr == nil || n == 0) {
        readstr(r, "No results\n");
        return;
    }

    results = format_search_results(sr, n);
    readstr(r, results);
    free(results);
    search_free(sr, n);
}

/*
 * Read from entry file
 */
void
read_entry(Req *r, Axon *axon)
{
    File *f;
    char *path;
    Entry *e;
    char *buf;
    int len;

    f = r->fid->file;
    if(f == nil) {
        respond(r, "no file");
        return;
    }

    /* Build entry title from filename */
    path = smprint("%s", f->name);

    e = axon_find_entry(axon, path);
    free(path);

    if(e == nil) {
        readstr(r, "Entry not found\n");
        return;
    }

    /* Format entry content with metadata */
    len = strlen(e->content) + 512;
    buf = emalloc9p(len);

    snprint(buf, len,
            "# %s\n\n"
            "Confidence: %.2f\n"
            "Sources: %d\n\n"
            "%s\n",
            e->title,
            e->confidence,
            e->nsources,
            e->content);

    readstr(r, buf);
    free(buf);
}

/*
 * Command handlers
 */

void
handle_add_entry(Axon *axon, char *title, char *content)
{
    Entry *e;

    e = entry_create(title, content);
    if(e == nil) {
        fprint(2, "handle_add_entry: failed to create entry\n");
        return;
    }

    axon_add_entry(axon, e);
}

void
handle_add_source(Axon *axon, char *path)
{
    /* Placeholder for source ingestion */
    fprint(2, "handle_add_source: %s (not yet implemented)\n", path);
}

void
handle_search(Axon *axon, char *query)
{
    /* Store query for results file */
    free(axon->last_query);
    axon->last_query = estrdup9p(query);
}

void
handle_sync(Axon *axon)
{
    Entry *e;

    /* Save all entries to disk */
    for(e = axon->entries; e != nil; e = e->next) {
        entry_save(e, axon->root_path);
    }
}

/*
 * Multi-mind inbox processing handlers
 */

/*
 * Process all files in inbox/incoming/
 */
void
handle_process_inbox(Axon *axon)
{
    char *inbox_path;
    Dir *files;
    int i, nfiles;

    if(axon == nil)
        return;

    /* Build path to inbox/incoming */
    inbox_path = smprint("%s/inbox/incoming", axon->root_path);

    /* List files in directory */
    nfiles = dirreadall(inbox_path, &files);
    if(nfiles <= 0) {
        free(inbox_path);
        return;
    }

    /* Process each file */
    for(i = 0; i < nfiles; i++) {
        char *file_path;
        char *content;
        Entry *entry;
        int fd;

        /* Skip directories */
        if(files[i].mode & DMDIR)
            continue;

        /* Build full path */
        file_path = smprint("%s/%s", inbox_path, files[i].name);

        /* Read file content */
        fd = open(file_path, OREAD);
        if(fd >= 0) {
            /* Read content */
            Dir *d = dirfstat(fd);
            if(d != nil) {
                content = emalloc9p(d->length + 1);
                if(readn(fd, content, d->length) == d->length) {
                    content[d->length] = '\0';

                    /* Create entry */
                    entry = entry_create(files[i].name, content);

                    /* Process with all minds */
                    axon_process_entry_with_minds(axon, entry);

                    /* Move to knowledge/ (immutable) */
                    char *knowledge_path;
                    knowledge_path = smprint("%s/knowledge/%s",
                                             axon->root_path, files[i].name);

                    /* Move file by renaming */
                    if(rename(file_path, knowledge_path) < 0) {
                        /* If rename fails, try copy + remove */
                        int out_fd = create(knowledge_path, OWRITE, 0644);
                        if(out_fd >= 0) {
                            write(out_fd, content, d->length);
                            close(out_fd);
                            remove(file_path);
                        }
                    }
                    free(knowledge_path);
                }
                free(d);
                free(content);
            }
            close(fd);
        }

        free(file_path);
    }

    free(files);
    free(inbox_path);
}

/*
 * Process a specific file
 */
void
handle_process_file(Axon *axon, char *path)
{
    char *full_path;
    int fd;
    char *content;
    Entry *entry;
    Dir *d;

    if(axon == nil || path == nil)
        return;

    /* Build full path - could be relative or absolute */
    if(path[0] == '/') {
        full_path = estrdup9p(path);
    } else {
        full_path = smprint("%s/%s", axon->root_path, path);
    }

    /* Read file */
    fd = open(full_path, OREAD);
    if(fd < 0) {
        free(full_path);
        return;
    }

    /* Read content */
    d = dirfstat(fd);
    if(d == nil) {
        close(fd);
        free(full_path);
        return;
    }

    content = emalloc9p(d->length + 1);
    if(readn(fd, content, d->length) != d->length) {
        free(content);
        free(d);
        close(fd);
        free(full_path);
        return;
    }
    content[d->length] = '\0';
    free(d);
    close(fd);

    /* Create entry and process */
    entry = entry_create(path, content);
    axon_process_entry_with_minds(axon, entry);

    free(content);
    free(full_path);
}

/*
 * Parse mind name to type
 */
static MindType
mind_name_to_type(char *name)
{
    if(name == nil)
        return MIND_MAX;

    if(strcmp(name, "literal") == 0)
        return MIND_LITERAL;
    else if(strcmp(name, "skeptic") == 0)
        return MIND_SKEPTIC;
    else if(strcmp(name, "synthesizer") == 0)
        return MIND_SYNTHESIZER;
    else if(strcmp(name, "pattern_matcher") == 0 || strcmp(name, "pattern") == 0)
        return MIND_PATTERN_MATCHER;
    else if(strcmp(name, "questioner") == 0)
        return MIND_QUESTIONER;
    else
        return MIND_MAX;
}

/*
 * Enable a specific mind
 */
void
handle_mind_enable(Axon *axon, char *mind_name)
{
    MindType type;
    Mind *mind;

    if(axon == nil || mind_name == nil)
        return;

    /* Parse mind name */
    type = mind_name_to_type(mind_name);
    if(type == MIND_MAX)
        return;

    /* Enable the mind */
    mind = mind_get_by_type(type);
    if(mind != nil) {
        mind_set_enabled(mind, 1);
        mind_free(mind);
    }
}

/*
 * Disable a specific mind
 */
void
handle_mind_disable(Axon *axon, char *mind_name)
{
    MindType type;
    Mind *mind;

    if(axon == nil || mind_name == nil)
        return;

    /* Parse mind name */
    type = mind_name_to_type(mind_name);
    if(type == MIND_MAX)
        return;

    /* Disable the mind */
    mind = mind_get_by_type(type);
    if(mind != nil) {
        mind_set_enabled(mind, 0);
        mind_free(mind);
    }
}

/*
 * LLM Configuration Handlers
 */

extern void llm_config_init(char *config_path);
extern char* llm_get_status(void);
extern char* llm_get_mind_status(MindType mind_type);
extern char* llm_get_config(void);
extern int llm_test_connection(void);
extern int llm_set_mind_model(MindType mind_type, char *provider_name);

/*
 * Set default LLM provider
 */
void
handle_llm_set_provider(Axon *axon, char *provider_name)
{
    /* TODO: Implement provider switching */
    /* For now, this is a placeholder */
    if(axon == nil || provider_name == nil)
        return;
}

/*
 * Test LLM connection
 */
void
handle_llm_test(Axon *axon)
{
    if(axon == nil)
        return;

    int result = llm_test_connection();
    /* Result is logged via status file */
}

/*
 * Set model for specific mind
 */
void
handle_llm_set_model(Axon *axon, char *mind_name, char *provider)
{
    MindType type;

    if(axon == nil || mind_name == nil || provider == nil)
        return;

    type = mind_name_to_type(mind_name);
    if(type == MIND_MAX)
        return;

    llm_set_mind_model(type, provider);
}

/*
 * LLM status handler
 */
void
handle_llm_status(Axon *axon)
{
    /* Status is read via /llm/status file */
    if(axon == nil)
        return;
}

/*
 * Read LLM status file
 */
void
read_llm_status(Req *r)
{
    char *status;

    status = llm_get_status();
    if(status == nil) {
        readstr(r, "LLM not initialized\n");
        return;
    }

    readstr(r, status);
    free(status);
}

/*
 * Read LLM config file
 */
void
read_llm_config(Req *r)
{
    char *config;

    config = llm_get_config();
    if(config == nil) {
        readstr(r, "# LLM Configuration\nprovider lm_studio\n");
        return;
    }

    readstr(r, config);
    free(config);
}

/*
 * Write LLM config file
 */
void
write_llm_config(Req *r, Axon *axon)
{
    /* TODO: Implement config writing */
    if(axon == nil)
        return;

    r->ofcall.count = r->ifcall.count;
    respond(r, nil);
}
