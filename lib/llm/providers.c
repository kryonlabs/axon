/*
 * AXON LLM Provider Configurations
 * Default configurations for various LLM providers
 */

#include <lib9.h>
#include "llm.h"

/*
 * Create a basic configuration structure
 */
static LLMConfig*
create_config(LLMProvider provider, char *endpoint, char *model)
{
    LLMConfig *config;

    config = emalloc9p(sizeof(LLMConfig));
    if(config == nil)
        return nil;

    config->provider = provider;
    config->endpoint = estrdup9p(endpoint);
    config->api_key = estrdup9p("");
    config->model = estrdup9p(model);
    config->temperature = 0.7;
    config->timeout_ms = 30000;
    config->max_tokens = 4096;

    return config;
}

/*
 * Get default configuration for LM Studio
 * Local server at localhost:1234
 */
LLMConfig*
lm_studio_config(void)
{
    return create_config(
        LLM_PROVIDER_LM_STUDIO,
        "http://localhost:1234/v1",
        "llama-3-8b-instruct"
    );
}

/*
 * Get default configuration for OpenAI
 */
LLMConfig*
openai_config(char *api_key)
{
    LLMConfig *config;

    config = create_config(
        LLM_PROVIDER_OPENAI,
        "https://api.openai.com/v1",
        "gpt-4o-mini"
    );

    if(config != nil && api_key != nil) {
        free(config->api_key);
        config->api_key = estrdup9p(api_key);
    }

    return config;
}

/*
 * Get default configuration for z.ai
 * Note: Assuming OpenAI-compatible format based on industry standards
 */
LLMConfig*
z_ai_config(char *api_key)
{
    LLMConfig *config;

    config = create_config(
        LLM_PROVIDER_Z_AI,
        "https://api.z.ai/v1",
        "zai-default"
    );

    if(config != nil && api_key != nil) {
        free(config->api_key);
        config->api_key = estrdup9p(api_key);
    }

    return config;
}

/*
 * Get generic OpenAI-compatible configuration
 */
LLMConfig*
openai_compat_config(char *endpoint, char *model, char *api_key)
{
    LLMConfig *config;

    if(endpoint == nil || model == nil)
        return nil;

    config = create_config(
        LLM_PROVIDER_OPENAI_COMPAT,
        endpoint,
        model
    );

    if(config != nil && api_key != nil) {
        free(config->api_key);
        config->api_key = estrdup9p(api_key);
    }

    return config;
}

/*
 * Get provider name from enum
 */
char*
llm_provider_name(LLMProvider provider)
{
    switch(provider) {
    case LLM_PROVIDER_LM_STUDIO:
        return "lm_studio";
    case LLM_PROVIDER_OPENAI:
        return "openai";
    case LLM_PROVIDER_Z_AI:
        return "z_ai";
    case LLM_PROVIDER_OPENAI_COMPAT:
        return "openai_compat";
    case LLM_PROVIDER_NONE:
    default:
        return "none";
    }
}

/*
 * Parse provider from string
 */
LLMProvider
llm_provider_parse(char *name)
{
    if(name == nil)
        return LLM_PROVIDER_NONE;

    if(strcmp(name, "lm_studio") == 0)
        return LLM_PROVIDER_LM_STUDIO;
    if(strcmp(name, "openai") == 0)
        return LLM_PROVIDER_OPENAI;
    if(strcmp(name, "z_ai") == 0)
        return LLM_PROVIDER_Z_AI;
    if(strcmp(name, "openai_compat") == 0)
        return LLM_PROVIDER_OPENAI_COMPAT;

    return LLM_PROVIDER_NONE;
}

/*
 * Copy configuration
 */
LLMConfig*
llm_config_copy(LLMConfig *src)
{
    LLMConfig *dst;

    if(src == nil)
        return nil;

    dst = emalloc9p(sizeof(LLMConfig));
    if(dst == nil)
        return nil;

    dst->provider = src->provider;
    dst->endpoint = estrdup9p(src->endpoint);
    dst->api_key = estrdup9p(src->api_key);
    dst->model = estrdup9p(src->model);
    dst->temperature = src->temperature;
    dst->timeout_ms = src->timeout_ms;
    dst->max_tokens = src->max_tokens;

    return dst;
}

/*
 * Free configuration
 */
void
llm_config_free(LLMConfig *config)
{
    if(config == nil)
        return;

    free(config->endpoint);
    free(config->api_key);
    free(config->model);
    free(config);
}

/*
 * Free configuration array
 */
void
llm_config_free_array(LLMConfig **configs, int nconfigs)
{
    int i;

    if(configs == nil)
        return;

    for(i = 0; i < nconfigs; i++) {
        llm_config_free(configs[i]);
    }

    free(configs);
}
