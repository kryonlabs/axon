/*
 * AXON LLM Configuration Filesystem Interface
 * Exposes LLM configuration via 9P filesystem
 */

#include <lib9.h>
#include <9p.h>
#include "axon.h"
#include "axonfs.h"
#include "llm_config.h"
#include "minds.h"

/*
 * LLM configuration state
 */
typedef struct LLMConfigState {
    LLMConfig **configs;      /* Available provider configs */
    int nconfigs;
    LLMProvider default_provider;
    char **mind_models;       /* Model assignment per mind [MIND_MAX] */
    LLMBackend **mind_backends; /* LLM backend per mind */
} LLMConfigState;

static LLMConfigState *llm_state = nil;

/*
 * Initialize LLM configuration state
 */
void
llm_config_init(char *config_path)
{
    int i;

    if(llm_state != nil)
        return;

    llm_state = emalloc9p(sizeof(LLMConfigState));

    /* Load configurations from file or use defaults */
    if(config_path != nil) {
        llm_config_load(config_path, &llm_state->configs, &llm_state->nconfigs);
    } else {
        llm_state->configs = emalloc9p(sizeof(LLMConfig*));
        llm_state->configs[0] = lm_studio_config();
        llm_state->nconfigs = 1;
    }

    llm_state->default_provider = LLM_PROVIDER_LM_STUDIO;

    /* Initialize mind model assignments */
    llm_state->mind_models = emalloc9p(MIND_MAX * sizeof(char*));
    llm_state->mind_backends = emalloc9p(MIND_MAX * sizeof(LLMBackend*));

    for(i = 0; i < MIND_MAX; i++) {
        llm_state->mind_models[i] = estrdup9p("lm_studio");
        llm_state->mind_backends[i] = nil;
    }
}

/*
 * Get LLM backend for a specific mind
 */
LLMBackend*
llm_get_mind_backend(MindType mind_type)
{
    LLMConfig *config;
    char *provider_name;
    int i;

    if(llm_state == nil)
        return nil;

    /* Check if backend already created */
    if(llm_state->mind_backends[mind_type] != nil)
        return llm_state->mind_backends[mind_type];

    /* Find provider config */
    provider_name = llm_state->mind_models[mind_type];
    for(i = 0; i < llm_state->nconfigs; i++) {
        if(strcmp(llm_provider_name(llm_state->configs[i]->provider),
                  provider_name) == 0) {
            config = llm_state->configs[i];
            break;
        }
    }

    if(i >= llm_state->nconfigs)
        return nil;

    /* Create backend */
    llm_state->mind_backends[mind_type] = llm_create(config);
    return llm_state->mind_backends[mind_type];
}

/*
 * Set model for a specific mind
 */
int
llm_set_mind_model(MindType mind_type, char *provider_name)
{
    if(llm_state == nil || provider_name == nil)
        return -1;

    /* Validate provider exists */
    int i;
    for(i = 0; i < llm_state->nconfigs; i++) {
        if(strcmp(llm_provider_name(llm_state->configs[i]->provider),
                  provider_name) == 0) {
            /* Free old backend if exists */
            if(llm_state->mind_backends[mind_type] != nil) {
                llm_free(llm_state->mind_backends[mind_type]);
                llm_state->mind_backends[mind_type] = nil;
            }

            /* Update model assignment */
            free(llm_state->mind_models[mind_type]);
            llm_state->mind_models[mind_type] = estrdup9p(provider_name);
            return 0;
        }
    }

    return -1;
}

/*
 * Get global LLM status
 */
char*
llm_get_status(void)
{
    char *status;
    int i, connected_count, total_requests, total_errors;

    if(llm_state == nil)
        return estrdup9p("LLM not initialized\n");

    connected_count = 0;
    total_requests = 0;
    total_errors = 0;

    for(i = 0; i < MIND_MAX; i++) {
        if(llm_state->mind_backends[i] != nil) {
            if(llm_state->mind_backends[i]->connected)
                connected_count++;
            total_requests += llm_state->mind_backends[i]->requests_total;
            total_errors += llm_state->mind_backends[i]->errors_total;
        }
    }

    status = smprint(
        "Provider: %s\n"
        "Minds Configured: %d\n"
        "Connections Active: %d\n"
        "Total Requests: %d\n"
        "Total Errors: %d\n",
        llm_provider_name(llm_state->default_provider),
        MIND_MAX,
        connected_count,
        total_requests,
        total_errors
    );

    return status;
}

/*
 * Get per-mind status
 */
char*
llm_get_mind_status(MindType mind_type)
{
    LLMBackend *backend;
    char *status;

    if(llm_state == nil)
        return estrdup9p("LLM not initialized\n");

    backend = llm_state->mind_backends[mind_type];
    if(backend == nil) {
        status = smprint(
            "Mind: %s\n"
            "Provider: %s\n"
            "Status: Not initialized\n"
            "Model: %s\n",
            mind_type_name(mind_type),
            llm_state->mind_models[mind_type],
            llm_state->mind_models[mind_type]
        );
        return status;
    }

    status = smprint(
        "Mind: %s\n"
        "Provider: %s\n"
        "Model: %s\n"
        "Status: %s\n"
        "Requests: %d\n"
        "Errors: %d\n"
        "Last Request: %lld ms\n",
        mind_type_name(mind_type),
        llm_provider_name(backend->config.provider),
        backend->config.model,
        backend->connected ? "Connected" : "Disconnected",
        backend->requests_total,
        backend->errors_total,
        backend->last_request_ms
    );

    return status;
}

/*
 * Get configuration file content
 */
char*
llm_get_config(void)
{
    char *config;
    int i;

    if(llm_state == nil)
        return estrdup9p("LLM not initialized\n");

    config = smprint(
        "# LLM Configuration\n"
        "provider %s\n",
        llm_provider_name(llm_state->default_provider)
    );

    for(i = 0; i < llm_state->nconfigs; i++) {
        char *section;
        LLMConfig *cfg = llm_state->configs[i];

        section = smprint(
            "\n[provider.%s]\n"
            "endpoint = %s\n"
            "model = %s\n"
            "temperature = %.1f\n"
            "timeout = %d\n",
            llm_provider_name(cfg->provider),
            cfg->endpoint,
            cfg->model,
            cfg->temperature,
            cfg->timeout_ms
        );

        char *new_config = smprint("%s%s", config, section);
        free(config);
        free(section);
        config = new_config;
    }

    return config;
}

/*
 * Test connection to current provider
 */
int
llm_test_connection(void)
{
    LLMBackend *backend;
    LLMResponse *resp;
    int result;

    if(llm_state == nil || llm_state->nconfigs == 0)
        return -1;

    /* Test with default provider config */
    backend = llm_create(llm_state->configs[0]);
    if(backend == nil)
        return -1;

    resp = llm_test(backend);
    result = (resp != nil && resp->success) ? 0 : -1;

    if(resp != nil)
        llm_response_free(resp);

    llm_free(backend);

    return result;
}

/*
 * Cleanup LLM configuration state
 */
void
llm_config_cleanup(void)
{
    int i;

    if(llm_state == nil)
        return;

    /* Free configs */
    for(i = 0; i < llm_state->nconfigs; i++) {
        llm_config_free(llm_state->configs[i]);
    }
    free(llm_state->configs);

    /* Free mind assignments */
    for(i = 0; i < MIND_MAX; i++) {
        free(llm_state->mind_models[i]);
        if(llm_state->mind_backends[i] != nil)
            llm_free(llm_state->mind_backends[i]);
    }
    free(llm_state->mind_models);
    free(llm_state->mind_backends);

    free(llm_state);
    llm_state = nil;
}
