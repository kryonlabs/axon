/*
 * AXON Mind System - Core Interface
 * Manages multiple AI extraction minds
 */

#include <lib9.h>
#include "axon.h"
#include "minds.h"
#include "llm.h"

/*
 * Forward declarations for LLM response parsing
 */
extern MindResult* parse_llm_response(char *response_text, MindType mind_type,
                                      Entry *source_entry);
extern char* build_extraction_prompt(Mind *mind, Entry *entry);

/*
 * Default prompts for each mind type
 */

static char*
mind_get_prompt_literal(void)
{
    return "Extract every factual claim exactly as stated. Trust the source. "
           "Extract entities, properties, relationships, and values. "
           "Assign high confidence (0.8-1.0) to facts clearly stated in the source.";
}

static char*
mind_get_prompt_skeptic(void)
{
    return "Extract claims but doubt everything. Flag what needs verification. "
           "Rate confidence by source quality - be conservative (0.2-0.6). "
           "Mark facts with MFLAG_NEEDS_VERIFICATION if sources are unclear. "
           "Question assumptions and demand evidence.";
}

static char*
mind_get_prompt_synthesizer(void)
{
    return "Extract facts and how they relate to existing knowledge. "
           "Look for connections to other concepts, causes, and effects. "
           "Mark relationships with MFLAG_RELATIONSHIP. "
           "Build a knowledge graph of interconnected ideas.";
}

static char*
mind_get_prompt_pattern_matcher(void)
{
    return "Extract underlying principles and patterns, not just facts. "
           "Look for laws, theories, formulas, and systematic relationships. "
           "Mark abstract principles with MFLAG_PRINCIPLE. "
           "Identify mathematical or logical patterns.";
}

static char*
mind_get_prompt_questioner(void)
{
    return "What questions does this entry NOT answer? What's missing? "
           "Identify knowledge gaps with MFLAG_GAP. "
           "Generate questions that would verify or expand the knowledge. "
           "Be critical and curious.";
}

/*
 * Mind extraction functions - each mind implements its strategy
 */

static MindResult*
literal_extract(Mind *mind, Entry *entry)
{
    MindResult *result = nil;
    LLMResponse *llm_resp;
    char *prompt;

    if(mind == nil || entry == nil)
        return nil;

    /* Try LLM extraction first */
    if(mind->llm != nil) {
        prompt = build_extraction_prompt(mind, entry);
        llm_resp = llm_complete(mind->llm,
                               "You are a literal fact extractor. Extract every factual claim exactly as stated.",
                               prompt);

        if(llm_resp != nil && llm_resp->success) {
            result = parse_llm_response(llm_resp->text, MIND_LITERAL, entry);
            free(prompt);
            llm_response_free(llm_resp);
            return result;
        }

        free(prompt);
        if(llm_resp != nil)
            llm_response_free(llm_resp);
    }

    /* Fallback to simple regex extraction */
    result = emalloc9p(sizeof(MindResult));
    result->mind_type = MIND_LITERAL;
    result->source_entry = entry;
    result->facts = nil;
    result->nfacts = 0;
    result->questions = nil;
    result->nquestions = 0;
    result->avg_confidence = 0.0;
    result->processing_time_ms = 0;

    /* Simple "X is Y" pattern extraction as fallback */
    char *content = entry->content;
    char *line = content;

    while(line != nil && *line != '\0') {
        char *newline = strchr(line, '\n');
        char *is_pos = strstr(line, " is ");

        if(is_pos != nil && (newline == nil || is_pos < newline)) {
            int subject_len = is_pos - line;
            char *subject = emalloc9p(subject_len + 1);
            memcpy(subject, line, subject_len);
            subject[subject_len] = '\0';

            char *object_start = is_pos + 4;
            int object_len = 0;
            if(newline != nil)
                object_len = newline - object_start;
            else
                object_len = strlen(object_start);

            char *object = emalloc9p(object_len + 1);
            memcpy(object, object_start, object_len);
            object[object_len] = '\0';

            MindFact *fact = mind_fact_create(subject, "is", object);
            if(fact != nil) {
                fact->confidence = mind->default_confidence;
                fact->explanation = estrdup9p("Extracted by literal mind (fallback)");
                fact->evidence = estrdup9p(line);

                result->nfacts++;
                result->facts = erealloc9p(result->facts,
                    result->nfacts * sizeof(MindFact*));
                result->facts[result->nfacts - 1] = fact;
            }

            free(subject);
            free(object);
        }

        if(newline == nil)
            break;
        line = newline + 1;
    }

    return result;
}

static MindResult*
skeptic_extract(Mind *mind, Entry *entry)
{
    MindResult *result = nil;
    LLMResponse *llm_resp;
    char *prompt;

    if(mind == nil || entry == nil)
        return nil;

    /* Try LLM extraction first */
    if(mind->llm != nil) {
        prompt = build_extraction_prompt(mind, entry);
        llm_resp = llm_complete(mind->llm,
                               "You are a skeptic. Extract claims but question everything. Flag what needs verification. Use low confidence scores (0.2-0.6).",
                               prompt);

        if(llm_resp != nil && llm_resp->success) {
            result = parse_llm_response(llm_resp->text, MIND_SKEPTIC, entry);
            free(prompt);
            llm_response_free(llm_resp);
            return result;
        }

        free(prompt);
        if(llm_resp != nil)
            llm_response_free(llm_resp);
    }

    /* Fallback to literal extraction with lower confidence */
    result = literal_extract(mind, entry);
    if(result != nil) {
        result->mind_type = MIND_SKEPTIC;

        for(int i = 0; i < result->nfacts; i++) {
            result->facts[i]->confidence *= 0.5;
            result->facts[i]->flags |= MFLAG_NEEDS_VERIFICATION;
            if(result->facts[i]->explanation != nil)
                free(result->facts[i]->explanation);
            result->facts[i]->explanation = estrdup9p("Needs verification (fallback)");
        }

        if(result->nfacts > 0) {
            double sum = 0.0;
            for(int i = 0; i < result->nfacts; i++) {
                sum += result->facts[i]->confidence;
            }
            result->avg_confidence = sum / result->nfacts;
        }
    }

    return result;
}

static MindResult*
synthesizer_extract(Mind *mind, Entry *entry)
{
    MindResult *result = nil;
    LLMResponse *llm_resp;
    char *prompt;

    if(mind == nil || entry == nil)
        return nil;

    /* Try LLM extraction first */
    if(mind->llm != nil) {
        prompt = build_extraction_prompt(mind, entry);
        llm_resp = llm_complete(mind->llm,
                               "You are a synthesizer. Extract facts and how they relate to existing knowledge. Look for connections, causes, and effects.",
                               prompt);

        if(llm_resp != nil && llm_resp->success) {
            result = parse_llm_response(llm_resp->text, MIND_SYNTHESIZER, entry);
            free(prompt);
            llm_response_free(llm_resp);
            return result;
        }

        free(prompt);
        if(llm_resp != nil)
            llm_response_free(llm_resp);
    }

    /* Fallback */
    result = literal_extract(mind, entry);
    if(result != nil) {
        result->mind_type = MIND_SYNTHESIZER;

        for(int i = 0; i < result->nfacts; i++) {
            result->facts[i]->flags |= MFLAG_RELATIONSHIP;
            if(result->facts[i]->explanation != nil)
                free(result->facts[i]->explanation);
            result->facts[i]->explanation = estrdup9p("Relationship identified (fallback)");
        }
    }

    return result;
}

static MindResult*
pattern_matcher_extract(Mind *mind, Entry *entry)
{
    MindResult *result = nil;
    LLMResponse *llm_resp;
    char *prompt;

    if(mind == nil || entry == nil)
        return nil;

    /* Try LLM extraction first */
    if(mind->llm != nil) {
        prompt = build_extraction_prompt(mind, entry);
        llm_resp = llm_complete(mind->llm,
                               "You are a pattern matcher. Extract underlying principles, laws, theories, formulas, and systematic relationships.",
                               prompt);

        if(llm_resp != nil && llm_resp->success) {
            result = parse_llm_response(llm_resp->text, MIND_PATTERN_MATCHER, entry);
            free(prompt);
            llm_response_free(llm_resp);
            return result;
        }

        free(prompt);
        if(llm_resp != nil)
            llm_response_free(llm_resp);
    }

    /* Fallback */
    result = literal_extract(mind, entry);
    if(result != nil) {
        result->mind_type = MIND_PATTERN_MATCHER;

        for(int i = 0; i < result->nfacts; i++) {
            result->facts[i]->flags |= MFLAG_PRINCIPLE;
            if(result->facts[i]->explanation != nil)
                free(result->facts[i]->explanation);
            result->facts[i]->explanation = estrdup9p("Pattern identified (fallback)");
        }
    }

    return result;
}

static MindResult*
questioner_extract(Mind *mind, Entry *entry)
{
    MindResult *result = nil;
    LLMResponse *llm_resp;
    char *prompt;

    if(mind == nil || entry == nil)
        return nil;

    /* Try LLM extraction first */
    if(mind->llm != nil) {
        prompt = build_extraction_prompt(mind, entry);
        llm_resp = llm_complete(mind->llm,
                               "You are a questioner. Identify what questions this entry does NOT answer. What's missing? What knowledge gaps exist?",
                               prompt);

        if(llm_resp != nil && llm_resp->success) {
            result = parse_llm_response(llm_resp->text, MIND_QUESTIONER, entry);
            free(prompt);
            llm_response_free(llm_resp);
            return result;
        }

        free(prompt);
        if(llm_resp != nil)
            llm_response_free(llm_resp);
    }

    /* Fallback */
    result = literal_extract(mind, entry);
    if(result != nil) {
        result->mind_type = MIND_QUESTIONER;

        result->nquestions = 3;
        result->questions = emalloc9p(result->nquestions * sizeof(char*));
        result->questions[0] = estrdup9p("What is the source of this information?");
        result->questions[1] = estrdup9p("Are there exceptions to these claims?");
        result->questions[2] = estrdup9p("What measurements support these facts?");
    }

    return result;
}

/*
 * Mind management functions
 */

Mind*
mind_create(MindType type)
{
    Mind *mind;

    mind = emalloc9p(sizeof(Mind));
    if(mind == nil)
        return nil;

    mind->type = type;
    mind->name = estrdup9p(mind_type_name(type));
    mind->description = estrdup9p(mind_type_description(type));
    mind->prompt = estrdup9p(mind_type_prompt(type));
    mind->enabled = 1;
    mind->llm = nil;
    mind->model_override = nil;
    mind->cleanup = nil;  /* No cleanup needed for basic minds */

    /* Set default confidence and extraction function based on type */
    switch(type) {
    case MIND_LITERAL:
        mind->default_confidence = 0.95;
        mind->extract = literal_extract;
        break;
    case MIND_SKEPTIC:
        mind->default_confidence = 0.40;
        mind->extract = skeptic_extract;
        break;
    case MIND_SYNTHESIZER:
        mind->default_confidence = 0.80;
        mind->extract = synthesizer_extract;
        break;
    case MIND_PATTERN_MATCHER:
        mind->default_confidence = 0.85;
        mind->extract = pattern_matcher_extract;
        break;
    case MIND_QUESTIONER:
        mind->default_confidence = 0.70;
        mind->extract = questioner_extract;
        break;
    default:
        mind->default_confidence = 0.5;
        mind->extract = nil;
        break;
    }

    return mind;
}

void
mind_free(Mind *mind)
{
    if(mind == nil)
        return;

    free(mind->name);
    free(mind->description);
    free(mind->prompt);
    free(mind->model_override);

    /* Free LLM backend if present */
    if(mind->llm != nil) {
        llm_free(mind->llm);
    }

    if(mind->cleanup != nil)
        mind->cleanup(mind);

    free(mind);
}

Mind*
mind_get_by_type(MindType type)
{
    return mind_create(type);
}

Mind**
mind_get_all(int *nminds)
{
    Mind **minds;
    int i;

    if(nminds != nil)
        *nminds = MIND_MAX;

    minds = emalloc9p(MIND_MAX * sizeof(Mind*));

    for(i = 0; i < MIND_MAX; i++) {
        minds[i] = mind_create((MindType)i);
    }

    return minds;
}

int
mind_set_enabled(Mind *mind, int enabled)
{
    if(mind == nil)
        return -1;

    mind->enabled = enabled ? 1 : 0;
    return 0;
}

/*
 * Mind extraction functions
 */

MindResult*
mind_extract(Mind *mind, Entry *entry)
{
    if(mind == nil || entry == nil)
        return nil;

    if(!mind->enabled)
        return nil;

    if(mind->extract == nil)
        return nil;

    return mind->extract(mind, entry);
}

MindResult**
mind_extract_all(Entry *entry, int *nresults)
{
    Mind **minds;
    MindResult **results;
    int i, j, n;

    if(entry == nil) {
        if(nresults != nil)
            *nresults = 0;
        return nil;
    }

    minds = mind_get_all(&n);
    if(minds == nil) {
        if(nresults != nil)
            *nresults = 0;
        return nil;
    }

    results = emalloc9p(n * sizeof(MindResult*));
    int count = 0;

    for(i = 0; i < n; i++) {
        MindResult *result = mind_extract(minds[i], entry);
        if(result != nil) {
            results[count++] = result;
        }
    }

    /* Free minds */
    for(i = 0; i < n; i++) {
        mind_free(minds[i]);
    }
    free(minds);

    if(nresults != nil)
        *nresults = count;

    /* Shrink array if some minds were disabled */
    if(count < n) {
        MindResult **shrunk = emalloc9p(count * sizeof(MindResult*));
        for(i = 0; i < count; i++) {
            shrunk[i] = results[i];
        }
        free(results);
        results = shrunk;
    }

    return results;
}

void
mind_result_free(MindResult *result)
{
    int i;

    if(result == nil)
        return;

    if(result->facts != nil) {
        for(i = 0; i < result->nfacts; i++) {
            mind_fact_free(result->facts[i]);
        }
        free(result->facts);
    }

    if(result->questions != nil) {
        for(i = 0; i < result->nquestions; i++) {
            free(result->questions[i]);
        }
        free(result->questions);
    }

    free(result);
}

void
mind_results_free(MindResult **results, int nresults)
{
    int i;

    if(results == nil)
        return;

    for(i = 0; i < nresults; i++) {
        mind_result_free(results[i]);
    }

    free(results);
}

/*
 * Mind fact functions
 */

MindFact*
mind_fact_create(char *subject, char *predicate, char *object)
{
    MindFact *fact;

    if(subject == nil || predicate == nil || object == nil)
        return nil;

    fact = emalloc9p(sizeof(MindFact));
    fact->subject = estrdup9p(subject);
    fact->predicate = estrdup9p(predicate);
    fact->object = estrdup9p(object);
    fact->confidence = 0.5;
    fact->flags = 0;
    fact->explanation = nil;
    fact->evidence = nil;

    return fact;
}

void
mind_fact_free(MindFact *fact)
{
    if(fact == nil)
        return;

    free(fact->subject);
    free(fact->predicate);
    free(fact->object);
    free(fact->explanation);
    free(fact->evidence);
    free(fact);
}

int
mind_fact_match(MindFact *a, MindFact *b)
{
    if(a == nil || b == nil)
        return 0;

    return strcmp(a->subject, b->subject) == 0 &&
           strcmp(a->predicate, b->predicate) == 0 &&
           strcmp(a->object, b->object) == 0;
}

char*
mind_fact_key(MindFact *fact)
{
    if(fact == nil)
        return nil;

    return smprint("%s:%s:%s", fact->subject, fact->predicate, fact->object);
}

/*
 * Mind type utilities
 */

char*
mind_type_name(MindType type)
{
    switch(type) {
    case MIND_LITERAL:        return "literal";
    case MIND_SKEPTIC:        return "skeptic";
    case MIND_SYNTHESIZER:    return "synthesizer";
    case MIND_PATTERN_MATCHER: return "pattern_matcher";
    case MIND_QUESTIONER:     return "questioner";
    default:                  return "unknown";
    }
}

char*
mind_type_description(MindType type)
{
    switch(type) {
    case MIND_LITERAL:
        return "Extracts every factual claim exactly as stated. Trusts the source.";
    case MIND_SKEPTIC:
        return "Questions everything. Flags claims needing verification.";
    case MIND_SYNTHESIZER:
        return "Finds relationships and connections between concepts.";
    case MIND_PATTERN_MATCHER:
        return "Identifies underlying principles and patterns.";
    case MIND_QUESTIONER:
        return "Identifies gaps and generates questions.";
    default:
        return "Unknown mind type";
    }
}

char*
mind_type_prompt(MindType type)
{
    switch(type) {
    case MIND_LITERAL:        return mind_get_prompt_literal();
    case MIND_SKEPTIC:        return mind_get_prompt_skeptic();
    case MIND_SYNTHESIZER:    return mind_get_prompt_synthesizer();
    case MIND_PATTERN_MATCHER: return mind_get_prompt_pattern_matcher();
    case MIND_QUESTIONER:     return mind_get_prompt_questioner();
    default:                  return "";
    }
}
