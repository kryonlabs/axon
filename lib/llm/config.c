/*
 * AXON LLM Configuration Management
 * Handles loading and saving LLM configurations
 */

#include <lib9.h>
#include <fcall.h>
#include "llm.h"

#define MAX_CONFIGS 10
#define MAX_LINE_LENGTH 1024

/*
 * Configuration file format (INI-style):
 *
 * [global]
 * default_provider = lm_studio
 *
 * [provider.lm_studio]
 * endpoint = http://localhost:1234/v1
 * api_key =
 * model = llama-3-8b-instruct
 * temperature = 0.7
 * timeout_ms = 30000
 *
 * [provider.openai]
 * endpoint = https://api.openai.com/v1
 * api_key = sk-your-key-here
 * model = gpt-4o-mini
 * temperature = 0.7
 * timeout_ms = 30000
 *
 * [minds]
 * literal = lm_studio
 * skeptic = openai
 * synthesizer = lm_studio
 * pattern_matcher = lm_studio
 * questioner = openai
 */

static char*
trim_whitespace(char *s)
{
    char *end;

    while(isspace((uchar)*s))
        s++;

    if(*s == '\0')
        return s;

    end = s + strlen(s) - 1;
    while(end > s && isspace((uchar)*end))
        end--;

    *(end + 1) = '\0';

    return s;
}

static int
parse_key_value(char *line, char **key, char **value)
{
    char *equals;

    equals = strchr(line, '=');
    if(equals == nil)
        return -1;

    *equals = '\0';
    *key = trim_whitespace(line);
    *value = trim_whitespace(equals + 1);

    return 0;
}

static LLMConfig*
find_or_create_config(LLMConfig **configs, int *nconfigs, char *provider_name)
{
    int i;
    LLMProvider provider;

    provider = llm_provider_parse(provider_name);

    /* Check if config already exists */
    for(i = 0; i < *nconfigs; i++) {
        if(configs[i]->provider == provider)
            return configs[i];
    }

    /* Create new config */
    if(*nconfigs >= MAX_CONFIGS)
        return nil;

    switch(provider) {
    case LLM_PROVIDER_LM_STUDIO:
        configs[*nconfigs] = lm_studio_config();
        break;
    case LLM_PROVIDER_OPENAI:
        configs[*nconfigs] = openai_config(nil);
        break;
    case LLM_PROVIDER_Z_AI:
        configs[*nconfigs] = z_ai_config(nil);
        break;
    case LLM_PROVIDER_OPENAI_COMPAT:
        configs[*nconfigs] = openai_compat_config("", "", nil);
        break;
    default:
        return nil;
    }

    (*nconfigs)++;
    return configs[*nconfigs - 1];
}

int
llm_config_load(char *path, LLMConfig **configs, int *nconfigs)
{
    Biobuf *bp;
    char *line, *key, *value, *section;
    LLMConfig *current_config;
    int i;

    if(path == nil || configs == nil || nconfigs == nil)
        return -1;

    *nconfigs = 0;

    bp = Bopen(path, OREAD);
    if(bp == nil) {
        /* File doesn't exist, use defaults */
        *configs = emalloc9p(sizeof(LLMConfig*));
        (*configs)[0] = lm_studio_config();
        *nconfigs = 1;
        return 0;
    }

    section = nil;
    current_config = nil;

    while((line = Brdline(bp, '\n')) != nil) {
        line[Blinelen(bp) - 1] = '\0';
        line = trim_whitespace(line);

        /* Skip empty lines and comments */
        if(*line == '\0' || *line == '#')
            continue;

        /* Section header */
        if(*line == '[') {
            char *end = strchr(line, ']');
            if(end != nil) {
                *end = '\0';
                if(section != nil) free(section);
                section = estrdup9p(line + 1);
                current_config = nil;
            }
            continue;
        }

        /* Key value pair */
        if(parse_key_value(line, &key, &value) == 0) {
            if(section != nil && strncmp(section, "provider.", 9) == 0) {
                char *provider_name = section + 9;
                current_config = find_or_create_config(configs, nconfigs,
                                                       provider_name);
                if(current_config != nil) {
                    if(strcmp(key, "endpoint") == 0) {
                        free(current_config->endpoint);
                        current_config->endpoint = estrdup9p(value);
                    } else if(strcmp(key, "api_key") == 0) {
                        free(current_config->api_key);
                        current_config->api_key = estrdup9p(value);
                    } else if(strcmp(key, "model") == 0) {
                        free(current_config->model);
                        current_config->model = estrdup9p(value);
                    } else if(strcmp(key, "temperature") == 0) {
                        current_config->temperature = atof(value);
                    } else if(strcmp(key, "timeout_ms") == 0) {
                        current_config->timeout_ms = atoi(value);
                    } else if(strcmp(key, "max_tokens") == 0) {
                        current_config->max_tokens = atoi(value);
                    }
                }
            }
        }
    }

    if(section != nil)
        free(section);

    Bterm(bp);

    /* If no configs loaded, create default */
    if(*nconfigs == 0) {
        *configs = emalloc9p(sizeof(LLMConfig*));
        (*configs)[0] = lm_studio_config();
        *nconfigs = 1;
    }

    return 0;
}

int
llm_config_save(char *path, LLMConfig **configs, int nconfigs)
{
    Biobuf *bp;
    int i;

    if(path == nil || configs == nil)
        return -1;

    bp = Bopen(path, OWRITE);
    if(bp == nil)
        return -1;

    Bprint(bp, "# AXON LLM Configuration\n");
    Bprint(bp, "# Generated automatically - do not edit directly\n\n");

    Bprint(bp, "[global]\n");
    Bprint(bp, "default_provider = lm_studio\n\n");

    for(i = 0; i < nconfigs; i++) {
        LLMConfig *config = configs[i];
        Bprint(bp, "[provider.%s]\n", llm_provider_name(config->provider));
        Bprint(bp, "endpoint = %s\n", config->endpoint);
        if(config->api_key != nil && strlen(config->api_key) > 0)
            Bprint(bp, "api_key = %s\n", config->api_key);
        else
            Bprint(bp, "api_key =\n");
        Bprint(bp, "model = %s\n", config->model);
        Bprint(bp, "temperature = %.1f\n", config->temperature);
        Bprint(bp, "timeout_ms = %d\n", config->timeout_ms);
        Bprint(bp, "max_tokens = %d\n\n", config->max_tokens);
    }

    Bprint(bp, "[minds]\n");
    Bprint(bp, "literal = lm_studio\n");
    Bprint(bp, "skeptic = lm_studio\n");
    Bprint(bp, "synthesizer = lm_studio\n");
    Bprint(bp, "pattern_matcher = lm_studio\n");
    Bprint(bp, "questioner = lm_studio\n");

    Bterm(bp);
    return 0;
}

int
llm_config_create_default(char *path)
{
    LLMConfig **configs;
    int nconfigs;

    configs = emalloc9p(sizeof(LLMConfig*));
    configs[0] = lm_studio_config();
    nconfigs = 1;

    int result = llm_config_save(path, configs, nconfigs);

    llm_config_free_all(configs, nconfigs);

    return result;
}
