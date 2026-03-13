#ifndef _LLM_CONFIG_H_
#define _LLM_CONFIG_H_

/*
 * AXON LLM Configuration Interface
 * External interface for LLM configuration management
 */

#include <lib9.h>
#include "llm.h"
#include "minds.h"

/*
 * Initialize LLM configuration from file
 */
void llm_config_init(char *config_path);

/*
 * Get LLM backend for a specific mind
 */
LLMBackend* llm_get_mind_backend(MindType mind_type);

/*
 * Set model for a specific mind
 */
int llm_set_mind_model(MindType mind_type, char *provider_name);

/*
 * Get global LLM status
 */
char* llm_get_status(void);

/*
 * Get per-mind status
 */
char* llm_get_mind_status(MindType mind_type);

/*
 * Get configuration file content
 */
char* llm_get_config(void);

/*
 * Test connection to current provider
 */
int llm_test_connection(void);

/*
 * Cleanup LLM configuration state
 */
void llm_config_cleanup(void);

#endif /* _LLM_CONFIG_H_ */
