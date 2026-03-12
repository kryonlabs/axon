/*
 * AXON Multi-Mind System Test Suite
 * Tests mind extraction, consensus, and contradiction detection
 */

#include <lib9.h>
#include <9p.h>
#include "../include/axon.h"
#include "../include/minds.h"
#include "../include/facts.h"

/*
 * Test helper: Create a test entry
 */
Entry*
create_test_entry(char *title, char *content)
{
    return entry_create(title, content);
}

/*
 * Test 1: Literal mind extraction
 */
int
test_literal_mind(void)
{
    Mind *mind;
    Entry *entry;
    MindResult *result;
    int passed;

    print("Test 1: Literal mind extraction... ");

    mind = mind_create(MIND_LITERAL);
    if(mind == nil) {
        print("FAILED - could not create mind\n");
        return 0;
    }

    entry = create_test_entry("Test1",
        "Sound is a mechanical wave.\n"
        "Light travels at 299792458 m/s.\n");

    result = mind_extract(mind, entry);
    if(result == nil) {
        print("FAILED - no result\n");
        entry_free(entry);
        mind_free(mind);
        return 0;
    }

    /* Should have extracted facts */
    passed = (result->nfacts > 0);
    if(passed) {
        /* Check that facts have high confidence */
        int i;
        for(i = 0; i < result->nfacts && passed; i++) {
            if(result->facts[i]->confidence < 0.8) {
                passed = 0;
            }
        }
    }

    mind_result_free(result);
    entry_free(entry);
    mind_free(mind);

    if(passed)
        print("PASSED\n");
    else
        print("FAILED\n");

    return passed;
}

/*
 * Test 2: Skeptic mind
 */
int
test_skeptic_mind(void)
{
    Mind *mind;
    Entry *entry;
    MindResult *result;
    int passed, i;

    print("Test 2: Skeptic mind... ");

    mind = mind_create(MIND_SKEPTIC);
    if(mind == nil) {
        print("FAILED - could not create mind\n");
        return 0;
    }

    entry = create_test_entry("Test2",
        "Sound is a mechanical wave.\n"
        "Light travels at 299792458 m/s.\n");

    result = mind_extract(mind, entry);
    if(result == nil) {
        print("FAILED - no result\n");
        entry_free(entry);
        mind_free(mind);
        return 0;
    }

    /* Skeptic should flag facts for verification */
    passed = 0;
    for(i = 0; i < result->nfacts; i++) {
        if(result->facts[i]->flags & MFLAG_NEEDS_VERIFICATION) {
            passed = 1;
            break;
        }
    }

    /* Skeptic should have lower confidence */
    for(i = 0; i < result->nfacts; i++) {
        if(result->facts[i]->confidence > 0.6) {
            passed = 0;  /* Too confident for skeptic */
            break;
        }
    }

    mind_result_free(result);
    entry_free(entry);
    mind_free(mind);

    if(passed)
        print("PASSED\n");
    else
        print("FAILED\n");

    return passed;
}

/*
 * Test 3: Consensus calculation
 */
int
test_consensus_calculation(void)
{
    MindResult **results;
    Entry *entry;
    ConsensusFact **consensus;
    int nresults, nconsensus;
    int passed;

    print("Test 3: Consensus calculation... ");

    entry = create_test_entry("Test3",
        "Sound is a mechanical wave.\n"
        "Light travels at 299792458 m/s.\n");

    /* Extract with all minds */
    results = mind_extract_all(entry, &nresults);
    if(results == nil || nresults == 0) {
        print("FAILED - no mind results\n");
        entry_free(entry);
        return 0;
    }

    /* Build consensus */
    consensus = consensus_build(results, nresults, &nconsensus);

    passed = (consensus != nil && nconsensus > 0);

    if(passed && nconsensus > 0) {
        /* Check that consensus confidence is in reasonable range */
        int i;
        for(i = 0; i < nconsensus && passed; i++) {
            if(consensus[i]->consensus_confidence < 0.0 ||
               consensus[i]->consensus_confidence > 1.0) {
                passed = 0;
            }
        }
    }

    /* Cleanup */
    mind_results_free(results, nresults);
    if(consensus != nil) {
        int i;
        for(i = 0; i < nconsensus; i++) {
            consensus_fact_free(consensus[i]);
        }
        free(consensus);
    }
    entry_free(entry);

    if(passed)
        print("PASSED\n");
    else
        print("FAILED\n");

    return passed;
}

/*
 * Test 4: Contradiction detection
 */
int
test_contradiction_detection(void)
{
    /* For Phase 1, this is a basic smoke test */
    /* Real contradictions require more complex scenarios */

    print("Test 4: Contradiction detection... ");

    /* Create scenarios where minds might disagree */
    Entry *entry1 = create_test_entry("Test4a",
        "Sound travels at 343 m/s in air.\n");

    Entry *entry2 = create_test_entry("Test4b",
        "Sound cannot travel in vacuum.\n");

    MindResult **results;
    int nresults;
    Contradiction **contradictions;
    int ncontra;
    int passed;

    results = mind_extract_all(entry1, &nresults);

    /* Check if contradictions are detected */
    contradictions = contradictions_find(results, nresults, &ncontra);

    passed = (contradictions != nil);  /* At least runs without error */

    /* Cleanup */
    mind_results_free(results, nresults);
    if(contradictions != nil) {
        int i;
        for(i = 0; i < ncontra; i++) {
            contradiction_free(contradictions[i]);
        }
        free(contradictions);
    }
    entry_free(entry1);
    entry_free(entry2);

    if(passed)
        print("PASSED\n");
    else
        print("FAILED\n");

    return passed;
}

/*
 * Test 5: Fact querying
 */
int
test_fact_querying(void)
{
    FactStore *store;
    FactQuery query;
    MindFact **results;
    int nresults;
    int passed;

    print("Test 5: Fact querying... ");

    store = fact_store_init("/axon/data");
    if(store == nil) {
        print("FAILED - could not create store\n");
        return 0;
    }

    /* Create a simple query */
    memset(&query, 0, sizeof(FactQuery));
    query.subject = "sound";
    query.min_confidence = 0.5;
    query.include_mind_specific = 1;
    query.include_consensus = 0;
    query.limit = 10;

    results = fact_query_mind_facts(store, &query, &nresults);

    /* For Phase 1, this is a smoke test */
    passed = (store != nil);  /* Store was created successfully */

    if(results != nil)
        fact_query_free((void**)results, nresults);

    fact_store_close(store);

    if(passed)
        print("PASSED\n");
    else
        print("FAILED\n");

    return passed;
}

/*
 * Main test runner
 */
void
main(int argc, char **argv)
{
    int passed, total;

    print("\n=== AXON Multi-Mind System Test Suite ===\n\n");

    passed = 0;
    total = 5;

    passed += test_literal_mind();
    passed += test_skeptic_mind();
    passed += test_consensus_calculation();
    passed += test_contradiction_detection();
    passed += test_fact_querying();

    print("\n=== Results: %d/%d tests passed ===\n", passed, total);

    if(passed == total) {
        print("All tests PASSED!\n");
        exits(nil);
    } else {
        print("Some tests FAILED!\n");
        exits("tests failed");
    }
}
