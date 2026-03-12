/*
 * AXON Filesystem Tree
 * Creates the 9P filesystem structure for AXON
 *
 * Structure:
 * /axon/
 * ├── ctl                    - Control file (write commands here)
 * ├── status                 - Overall status
 * ├── knowledge/             - Immutable source entries
 * ├── minds/                 - Multi-mind AI extraction agents
 * │   ├── literal/           - Takes everything literally
 * │   ├── skeptic/           - Doubts claims, wants sources
 * │   ├── synthesizer/       - Combines related concepts
 * │   ├── pattern_matcher/   - Finds relationships
 * │   ├── questioner/        - Generates questions
 * │   └── status             - Overall minds status
 * ├── facts/                 - Extracted, mutable facts
 * │   ├── by_subject/        - Facts indexed by subject
 * │   ├── by_predicate/      - Facts indexed by predicate
 * │   ├── consensus/         - Agreed facts (high confidence)
 * │   └── status             - Fact storage status
 * ├── inbox/                 - New knowledge to process
 * │   ├── incoming/          - Drop files here
 * │   └── status             - Inbox status
 * └── contradictions/        - Conflicts between minds
 */

#include <lib9.h>
#include <9p.h>
#include "axon.h"
#include "axonfs.h"
#include "minds.h"

/*
 * Filesystem node types for aux field
 * (Additional types beyond those in axonfs.h)
 */
#define AXON_MINDS_STATUS       "minds_status"
#define AXON_MIND_LITERAL       "mind_literal"
#define AXON_MIND_SKEPTIC       "mind_skeptic"
#define AXON_MIND_SYNTHESIZER   "mind_synthesizer"
#define AXON_MIND_PATTERN       "mind_pattern"
#define AXON_MIND_QUESTIONER    "mind_questioner"

#define AXON_FACTS_STATUS       "facts_status"
#define AXON_INBOX_STATUS       "inbox_status"
#define AXON_CONTRADICTIONS_DIR "contradictions_dir"

/*
 * Create the AXON filesystem tree
 */
void*
axon_create_tree(Axon *axon)
{
    Tree *t;
    File *root;
    File *knowledge, *minds, *facts, *inbox, *contradictions, *encyclopedia, *memory, *ingest;
    File *entries_dir, *search_dir, *confidence_dir, *sources_dir;
    File *literal, *skeptic, *synthesizer, *pattern, *questioner;
    File *by_subject, *by_predicate, *consensus;
    File *inbox_incoming;

    t = alloctree("axon", "axon", 0555 | DMDIR, nil);
    if(t == nil)
        return nil;

    root = t->root;

    /* Create ctl and status files */
    createfile(root, "ctl", nil, 0666, AXON_CTL);
    createfile(root, "status", nil, 0444, AXON_STATUS);

    /* Create main directories */
    knowledge = createfile(root, "knowledge", nil, 0755 | DMDIR, axon);
    minds = createfile(root, "minds", nil, 0755 | DMDIR, axon);
    facts = createfile(root, "facts", nil, 0755 | DMDIR, axon);
    inbox = createfile(root, "inbox", nil, 0755 | DMDIR, axon);
    contradictions = createfile(root, "contradictions", nil, 0755 | DMDIR, AXON_CONTRADICTIONS_DIR);

    /* Create legacy directories for backward compatibility */
    encyclopedia = createfile(root, "encyclopedia", nil, 0755 | DMDIR, axon);
    memory = createfile(root, "memory", nil, 0755 | DMDIR, axon);
    ingest = createfile(root, "ingest", nil, 0755 | DMDIR, axon);

    /* Create encyclopedia subdirectories */
    entries_dir = createfile(encyclopedia, "entries", nil, 0755 | DMDIR, AXON_ENTRIES);
    search_dir = createfile(encyclopedia, "search", nil, 0755 | DMDIR, AXON_SEARCH);
    confidence_dir = createfile(encyclopedia, "confidence", nil, 0755 | DMDIR, AXON_CONFIDENCE);
    sources_dir = createfile(encyclopedia, "sources", nil, 0755 | DMDIR, AXON_SOURCES);

    /* Create search interface files */
    createfile(search_dir, "query", nil, 0666, AXON_QUERY);
    createfile(search_dir, "results", nil, 0444, AXON_RESULTS);

    /* Create memory subdirs */
    createfile(memory, "episodic", nil, 0755 | DMDIR, "episodic");
    createfile(memory, "semantic", nil, 0755 | DMDIR, "semantic");
    createfile(memory, "procedural", nil, 0755 | DMDIR, "procedural");

    /* Create ingestion files */
    createfile(ingest, "queue", nil, 0666, "ingest_queue");
    createfile(ingest, "status", nil, 0444, "ingest_status");
    createfile(ingest, "sources", nil, 0755 | DMDIR, "ingest_sources");

    /* === NEW: Multi-mind system === */

    /* Create minds directory with per-mind subdirectories */
    literal = createfile(minds, "literal", nil, 0755 | DMDIR, AXON_MIND_LITERAL);
    createfile(literal, "status", nil, 0444, "literal_status");
    createfile(literal, "facts", nil, 0444, "literal_facts");

    skeptic = createfile(minds, "skeptic", nil, 0755 | DMDIR, AXON_MIND_SKEPTIC);
    createfile(skeptic, "status", nil, 0444, "skeptic_status");
    createfile(skeptic, "facts", nil, 0444, "skeptic_facts");

    synthesizer = createfile(minds, "synthesizer", nil, 0755 | DMDIR, AXON_MIND_SYNTHESIZER);
    createfile(synthesizer, "status", nil, 0444, "synthesizer_status");
    createfile(synthesizer, "facts", nil, 0444, "synthesizer_facts");

    pattern = createfile(minds, "pattern_matcher", nil, 0755 | DMDIR, AXON_MIND_PATTERN);
    createfile(pattern, "status", nil, 0444, "pattern_status");
    createfile(pattern, "facts", nil, 0444, "pattern_facts");

    questioner = createfile(minds, "questioner", nil, 0755 | DMDIR, AXON_MIND_QUESTIONER);
    createfile(questioner, "status", nil, 0444, "questioner_status");
    createfile(questioner, "facts", nil, 0444, "questioner_facts");

    /* Create overall minds status file */
    createfile(minds, "status", nil, 0444, AXON_MINDS_STATUS);

    /* Create facts directory structure */
    by_subject = createfile(facts, "by_subject", nil, 0755 | DMDIR, "facts_by_subject");
    by_predicate = createfile(facts, "by_predicate", nil, 0755 | DMDIR, "facts_by_predicate");
    consensus = createfile(facts, "consensus", nil, 0755 | DMDIR, "facts_consensus");
    createfile(facts, "status", nil, 0444, AXON_FACTS_STATUS);

    /* Create inbox structure */
    inbox_incoming = createfile(inbox, "incoming", nil, 0755 | DMDIR, "inbox_incoming");
    createfile(inbox, "status", nil, 0444, AXON_INBOX_STATUS);

    return t;
}
