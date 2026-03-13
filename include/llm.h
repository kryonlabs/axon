#ifndef _LLM_H_
#define _LLM_H_

/*
 * AXON LLM Backend Module
 * Provides interface to external LLM APIs (LM Studio, OpenAI, z.ai, etc.)
 */

#include <lib9.h>

/*
 * LLM Provider types
 */
typedef enum LLMProvider {
    LLM_PROVIDER_NONE = 0,
    LLM_PROVIDER_LM_STUDIO,      /* Local LM Studio server */
    LLM_PROVIDER_OPENAI,         /* OpenAI API */
    LLM_PROVIDER_Z_AI,           /* z.ai service */
    LLM_PROVIDER_OPENAI_COMPAT,  /* Generic OpenAI-compatible API */
    LLM_PROVIDER_MAX
} LLMProvider;

/*
 * LLM Configuration
 */
typedef struct LLMConfig {
    LLMProvider provider;
    char *endpoint;        /* API endpoint URL */
    char *api_key;         /* API key (can be empty for local) */
    char *model;           /* Model name */
    double temperature;    /* Temperature for generation (0.0-1.0) */
    int timeout_ms;        /* Request timeout in milliseconds */
    int max_tokens;        /* Maximum tokens in response */
} LLMConfig;

/*
 * LLM Backend state
 */
typedef struct LLMBackend {
    LLMConfig config;
    int connected;         /* Connection status */
    vlong last_request_ms; /* Time of last request */
    int requests_total;    /* Total requests made */
    int errors_total;      /* Total errors encountered */
    char *last_error;      /* Last error message */
} LLMBackend;

/*
 * LLM Response
 */
typedef struct LLMResponse {
    char *text;            /* Response text */
    int success;           /* Whether request succeeded */
    int status_code;       /* HTTP status code */
    char *error;           /* Error message if failed */
    vlong duration_ms;     /* Request duration */
} LLMResponse;

/*
 * Provider configuration functions
 */

/* Get default configuration for LM Studio */
LLMConfig* lm_studio_config(void);

/* Get default configuration for OpenAI */
LLMConfig* openai_config(char *api_key);

/* Get default configuration for z.ai */
LLMConfig* z_ai_config(char *api_key);

/* Get generic OpenAI-compatible configuration */
LLMConfig* openai_compat_config(char *endpoint, char *model, char *api_key);

/*
 * LLM Backend management
 */

/* Create an LLM backend with given configuration */
LLMBackend* llm_create(LLMConfig *config);

/* Free an LLM backend */
void llm_free(LLMBackend *llm);

/* Test connection to LLM provider */
LLMResponse* llm_test(LLMBackend *llm);

/*
 * LLM Completion functions
 */

/* Send completion request to LLM */
LLMResponse* llm_complete(LLMBackend *llm, char *system_prompt, char *user_prompt);

/* Send completion with custom temperature */
LLMResponse* llm_complete_with_temp(LLMBackend *llm, char *system_prompt,
                                    char *user_prompt, double temperature);

/*
 * LLM Response management
 */

/* Free an LLM response */
void llm_response_free(LLMResponse *resp);

/* Check if response was successful */
int llm_response_success(LLMResponse *resp);

/*
 * Configuration management
 */

/* Load LLM configuration from file */
int llm_config_load(char *path, LLMConfig **configs, int *nconfigs);

/* Save LLM configuration to file */
int llm_config_save(char *path, LLMConfig **configs, int nconfigs);

/* Create default configuration file */
int llm_config_create_default(char *path);

/* Free configuration array */
void llm_config_free_all(LLMConfig **configs, int nconfigs);

/*
 * Utility functions
 */

/* Get provider name */
char* llm_provider_name(LLMProvider provider);

/* Parse provider from string */
LLMProvider llm_provider_parse(char *name);

/* Copy configuration */
LLMConfig* llm_config_copy(LLMConfig *src);

/* Free configuration */
void llm_config_free(LLMConfig *config);

#endif /* _LLM_H_ */
