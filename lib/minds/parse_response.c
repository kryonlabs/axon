/*
 * AXON LLM Response Parser
 * Parses LLM text output into structured MindFact objects
 */

#include <lib9.h>
#include <ctype.h>
#include "minds.h"
#include "llm.h"

/*
 * Parse a line in format: "subject | predicate | object | confidence"
 * or "subject:predicate:object" format
 */
static MindFact*
parse_fact_line(char *line, MindType mind_type)
{
    MindFact *fact;
    char *subject, *predicate, *object, *confidence_str;
    char *sep1, *sep2, *sep3, *newline;
    double confidence;

    if(line == nil || strlen(line) == 0)
        return nil;

    /* Remove trailing newline */
    newline = strchr(line, '\n');
    if(newline != nil)
        *newline = '\0';

    /* Trim whitespace */
    while(isspace((uchar)*line))
        line++;

    /* Skip empty lines or comments */
    if(*line == '\0' || *line == '#')
        return nil;

    /* Try pipe-separated format: "subject | predicate | object | confidence" */
    sep1 = strchr(line, '|');
    if(sep1 != nil) {
        *sep1 = '\0';
        subject = trim_whitespace(line);

        sep2 = strchr(sep1 + 1, '|');
        if(sep2 != nil) {
            *sep2 = '\0';
            predicate = trim_whitespace(sep1 + 1);

            sep3 = strchr(sep2 + 1, '|');
            if(sep3 != nil) {
                *sep3 = '\0';
                object = trim_whitespace(sep2 + 1);
                confidence_str = trim_whitespace(sep3 + 1);

                confidence = atof(confidence_str);
                if(confidence <= 0.0)
                    confidence = 0.5;

                fact = mind_fact_create(subject, predicate, object);
                if(fact != nil) {
                    fact->confidence = confidence;

                    /* Set flags based on mind type */
                    switch(mind_type) {
                    case MIND_SKEPTIC:
                        fact->flags |= MFLAG_NEEDS_VERIFICATION;
                        break;
                    case MIND_SYNTHESIZER:
                        fact->flags |= MFLAG_RELATIONSHIP;
                        break;
                    case MIND_PATTERN_MATCHER:
                        fact->flags |= MFLAG_PRINCIPLE;
                        break;
                    case MIND_QUESTIONER:
                        fact->flags |= MFLAG_GAP;
                        break;
                    default:
                        break;
                    }

                    fact->explanation = estrdup9p("Extracted by LLM");
                    fact->evidence = estrdup9p(line);
                }

                return fact;
            }
        }
    }

    /* Try colon-separated format: "subject:predicate:object" */
    sep1 = strchr(line, ':');
    if(sep1 != nil) {
        *sep1 = '\0';
        subject = trim_whitespace(line);

        sep2 = strchr(sep1 + 1, ':');
        if(sep2 != nil) {
            *sep2 = '\0';
            predicate = trim_whitespace(sep1 + 1);
            object = trim_whitespace(sep2 + 1);

            fact = mind_fact_create(subject, predicate, object);
            if(fact != nil) {
                fact->confidence = 0.7;  /* Default confidence */

                /* Set flags based on mind type */
                switch(mind_type) {
                case MIND_SKEPTIC:
                    fact->flags |= MFLAG_NEEDS_VERIFICATION;
                    fact->confidence = 0.4;
                    break;
                case MIND_SYNTHESIZER:
                    fact->flags |= MFLAG_RELATIONSHIP;
                    break;
                case MIND_PATTERN_MATCHER:
                    fact->flags |= MFLAG_PRINCIPLE;
                    break;
                case MIND_QUESTIONER:
                    fact->flags |= MFLAG_GAP;
                    break;
                default:
                    break;
                }

                fact->explanation = estrdup9p("Extracted by LLM");
                fact->evidence = estrdup9p(line);
            }

            return fact;
        }
    }

    return nil;
}

/*
 * Parse LLM response text into MindResult
 */
MindResult*
parse_llm_response(char *response_text, MindType mind_type, Entry *source_entry)
{
    MindResult *result;
    char *line, *line_start;
    int nlines, i;

    if(response_text == nil)
        return nil;

    result = emalloc9p(sizeof(MindResult));
    result->mind_type = mind_type;
    result->source_entry = source_entry;
    result->facts = nil;
    result->nfacts = 0;
    result->questions = nil;
    result->nquestions = 0;
    result->avg_confidence = 0.0;
    result->processing_time_ms = 0;

    /* Count lines */
    nlines = 1;
    for(i = 0; response_text[i] != '\0'; i++) {
        if(response_text[i] == '\n')
            nlines++;
    }

    /* Parse each line as a potential fact */
    line_start = response_text;
    i = 0;

    while(*line_start != '\0' && i < nlines) {
        char *newline = strchr(line_start, '\n');
        if(newline != nil) {
            *newline = '\0';
        }

        MindFact *fact = parse_fact_line(line_start, mind_type);
        if(fact != nil) {
            result->nfacts++;
            result->facts = erealloc9p(result->facts,
                result->nfacts * sizeof(MindFact*));
            result->facts[result->nfacts - 1] = fact;
        }

        if(newline != nil) {
            line_start = newline + 1;
        } else {
            break;
        }

        i++;
    }

    /* Calculate average confidence */
    if(result->nfacts > 0) {
        double sum = 0.0;
        for(i = 0; i < result->nfacts; i++) {
            sum += result->facts[i]->confidence;
        }
        result->avg_confidence = sum / result->nfacts;
    }

    /* Generate questions for Questioner mind */
    if(mind_type == MIND_QUESTIONER && result->nfacts == 0) {
        /* If no facts extracted, generate questions from response */
        result->nquestions = 3;
        result->questions = emalloc9p(result->nquestions * sizeof(char*));
        result->questions[0] = estrdup9p("What sources support these claims?");
        result->questions[1] = estrdup9p("What measurements or data are available?");
        result->questions[2] = estrdup9p("What are the limitations of this knowledge?");
    }

    return result;
}

/*
 * Format entry content for LLM prompt
 */
char*
format_entry_for_prompt(Entry *entry)
{
    char *formatted;

    if(entry == nil)
        return estrdup9p("");

    formatted = smprint(
        "Title: %s\n\n"
        "Content:\n%s\n",
        entry->title != nil ? entry->title : "(no title)",
        entry->content != nil ? entry->content : "(no content)"
    );

    return formatted;
}

/*
 * Build extraction prompt for a mind
 */
char*
build_extraction_prompt(Mind *mind, Entry *entry)
{
    char *entry_text, *prompt;

    entry_text = format_entry_for_prompt(entry);

    prompt = smprint(
        "%s\n\n"
        "Extract facts from the following text in format: "
        "\"subject | predicate | object | confidence\"\n"
        "where confidence is 0.0-1.0.\n\n"
        "Text to analyze:\n%s",
        mind->prompt != nil ? mind->prompt : "Extract facts.",
        entry_text
    );

    free(entry_text);

    return prompt;
}
