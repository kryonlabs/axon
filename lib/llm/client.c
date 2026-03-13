/*
 * AXON LLM HTTP Client
 * Handles HTTP communication with LLM APIs
 */

#include <lib9.h>
#include <ctype.h>
#include "llm.h"

/*
 * HTTP request helpers
 */

static char*
http_escape(char *s)
{
    char *escaped;
    int i, j, len;

    if(s == nil)
        return nil;

    len = strlen(s);
    escaped = emalloc9p(len * 3 + 1);

    j = 0;
    for(i = 0; i < len; i++) {
        if(isalnum((uchar)s[i]) || s[i] == '-' || s[i] == '_' ||
           s[i] == '.' || s[i] == '~') {
            escaped[j++] = s[i];
        } else {
            snprint(escaped + j, 4, "%%02X", (uchar)s[i]);
            j += 3;
        }
    }
    escaped[j] = '\0';

    return escaped;
}

static int
parse_url(char *url, char **host, int *port, char **path, int *ssl)
{
    char *slash, *colon;

    *ssl = 0;
    *port = 80;

    if(strncmp(url, "https://", 8) == 0) {
        *ssl = 1;
        *port = 443;
        url += 8;
    } else if(strncmp(url, "http://", 7) == 0) {
        url += 7;
    } else {
        return -1;
    }

    slash = strchr(url, '/');
    if(slash != nil) {
        *path = estrdup9p(slash);
        colon = strchr(url, ':');
        if(colon != nil && colon < slash) {
            *host = emalloc9p(colon - url + 1);
            memcpy(*host, url, colon - url);
            (*host)[colon - url] = '\0';
            *port = atoi(colon + 1);
        } else {
            *host = emalloc9p(slash - url + 1);
            memcpy(*host, url, slash - url);
            (*host)[slash - url] = '\0';
        }
    } else {
        *path = estrdup9p("/");
        colon = strchr(url, ':');
        if(colon != nil) {
            *host = emalloc9p(colon - url + 1);
            memcpy(*host, url, colon - url);
            (*host)[colon - url] = '\0';
            *port = atoi(colon + 1);
        } else {
            *host = estrdup9p(url);
        }
    }

    return 0;
}

/*
 * Simple JSON handling (minimal implementation for our needs)
 */

static char*
json_escape(char *s)
{
    char *escaped;
    int i, j, len;

    if(s == nil)
        return estrdup9p("");

    len = strlen(s);
    escaped = emalloc9p(len * 2 + 1);

    j = 0;
    for(i = 0; i < len; i++) {
        if(s[i] == '"' || s[i] == '\\' || s[i] == '\n' || s[i] == '\r' ||
           s[i] == '\t' || s[i] == '\b') {
            escaped[j++] = '\\';
            switch(s[i]) {
            case '"':  escaped[j++] = '"'; break;
            case '\\': escaped[j++] = '\\'; break;
            case '\n': escaped[j++] = 'n'; break;
            case '\r': escaped[j++] = 'r'; break;
            case '\t': escaped[j++] = 't'; break;
            case '\b': escaped[j++] = 'b'; break;
            }
        } else {
            escaped[j++] = s[i];
        }
    }
    escaped[j] = '\0';

    return escaped;
}

static char*
build_request_body(LLMBackend *llm, char *system_prompt, char *user_prompt,
                   double temperature)
{
    char *escaped_system, *escaped_user, *body;

    escaped_system = json_escape(system_prompt);
    escaped_user = json_escape(user_prompt);

    body = smprint(
        "{\"model\":\"%s\",\"messages\":[{\"role\":\"system\",\"content\":\"%s\"},"
        "{\"role\":\"user\",\"content\":\"%s\"}],\"temperature\":%.1f,"
        "\"max_tokens\":%d}",
        llm->config.model, escaped_system, escaped_user,
        temperature, llm->config.max_tokens
    );

    free(escaped_system);
    free(escaped_user);

    return body;
}

static char*
extract_content_from_response(char *response)
{
    char *content_start, *content_end, *content;
    int in_content = 0;
    int depth = 0;
    int i;

    /* Find "content":" in the response */
    content_start = strstr(response, "\"content\":\"");
    if(content_start == nil)
        return estrdup9p("");

    content_start += 10; /* Skip "content":" */

    /* Parse the JSON string value */
    content = emalloc9p(strlen(content_start) + 1);
    int j = 0;

    for(i = 0; content_start[i] != '\0'; i++) {
        if(content_start[i] == '\\' && content_start[i+1] != '\0') {
            /* Handle escaped characters */
            switch(content_start[i+1]) {
            case 'n': content[j++] = '\n'; break;
            case 'r': content[j++] = '\r'; break;
            case 't': content[j++] = '\t'; break;
            case '"': content[j++] = '"'; break;
            case '\\': content[j++] = '\\'; break;
            default: content[j++] = content_start[i+1]; break;
            }
            i++;
        } else if(content_start[i] == '"') {
            /* End of content string */
            content[j] = '\0';
            return content;
        } else {
            content[j++] = content_start[i];
        }
    }

    content[j] = '\0';
    return content;
}

/*
 * Send HTTP POST request
 */
static char*
http_post(LLMBackend *llm, char *url, char *headers, char *body, int *status_code)
{
    char *host, *path, *request, *response;
    int port, ssl, fd;
    int content_length;
    vlong start, end;
    char *addr;

    *status_code = 0;

    if(parse_url(url, &host, &port, &path, &ssl) < 0) {
        if(llm != nil && llm->last_error != nil) free(llm->last_error);
        if(llm != nil) llm->last_error = estrdup9p("Failed to parse URL");
        return nil;
    }

    content_length = strlen(body);

    /* Build HTTP request */
    request = smprint(
        "POST %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %d\r\n"
        "%s"
        "\r\n"
        "%s",
        path, host, content_length,
        headers != nil ? headers : "",
        body
    );

    free(host);
    free(path);

    /* For now, only support HTTP (not HTTPS) for local LM Studio */
    /* In production, use TLS library for HTTPS */
    if(ssl) {
        free(request);
        /* TODO: Implement TLS for HTTPS connections */
        if(llm != nil && llm->last_error != nil) free(llm->last_error);
        if(llm != nil) llm->last_error = estrdup9p("HTTPS not yet implemented - use HTTP for local LM Studio");
        return estrdup9p("HTTPS not yet implemented - use HTTP for local LM Studio");
    }

    /* Connect to server */
    addr = smprint("tcp!%s!%d", host, port);
    fd = dial(addr, nil, nil, nil);
    free(addr);

    if(fd < 0) {
        if(llm != nil && llm->last_error != nil) free(llm->last_error);
        if(llm != nil) llm->last_error = estrdup9p("Failed to connect to LLM server");
        free(request);
        return nil;
    }

    /* Send request */
    start = nsec();
    if(write(fd, request, strlen(request)) < 0) {
        if(llm != nil && llm->last_error != nil) free(llm->last_error);
        if(llm != nil) llm->last_error = estrdup9p("Failed to send request");
        close(fd);
        free(request);
        return nil;
    }

    /* Read response */
    response = emalloc9p(65536);
    int n = read(fd, response, 65535);
    end = nsec();

    close(fd);
    free(request);

    if(n <= 0) {
        if(llm != nil && llm->last_error != nil) free(llm->last_error);
        if(llm != nil) llm->last_error = estrdup9p("Failed to read response");
        free(response);
        return nil;
    }
    response[n] = '\0';

    if(llm != nil) {
        llm->last_request_ms = (end - start) / 1000000;
    }

    /* Parse status code */
    char *status_line = response;
    char *status_end = strchr(status_line, '\r');
    if(status_end != nil) {
        *status_end = '\0';
        if(strncmp(status_line, "HTTP/1.1 200", 12) == 0 ||
           strncmp(status_line, "HTTP/1.0 200", 12) == 0) {
            *status_code = 200;
        } else if(strncmp(status_line, "HTTP/1.1", 8) == 0 ||
                  strncmp(status_line, "HTTP/1.0", 8) == 0) {
            *status_code = atoi(status_line + 9);
        }
    }

    /* Skip headers and get body */
    char *body_start = strstr(response, "\r\n\r\n");
    if(body_start != nil) {
        char *body_content = estrdup9p(body_start + 4);
        free(response);
        return body_content;
    }

    return response;
}

/*
 * LLM Backend implementation
 */

LLMBackend*
llm_create(LLMConfig *config)
{
    LLMBackend *llm;

    if(config == nil)
        return nil;

    llm = emalloc9p(sizeof(LLMBackend));
    if(llm == nil)
        return nil;

    llm->config = *config;
    llm->config.endpoint = estrdup9p(config->endpoint);
    llm->config.api_key = estrdup9p(config->api_key);
    llm->config.model = estrdup9p(config->model);

    llm->connected = 0;
    llm->last_request_ms = 0;
    llm->requests_total = 0;
    llm->errors_total = 0;
    llm->last_error = nil;

    return llm;
}

void
llm_free(LLMBackend *llm)
{
    if(llm == nil)
        return;

    free(llm->config.endpoint);
    free(llm->config.api_key);
    free(llm->config.model);
    free(llm->last_error);
    free(llm);
}

LLMResponse*
llm_complete_with_temp(LLMBackend *llm, char *system_prompt, char *user_prompt,
                       double temperature)
{
    LLMResponse *resp;
    char *url, *headers, *body, *response_body;
    int status_code;

    if(llm == nil || system_prompt == nil || user_prompt == nil)
        return nil;

    resp = emalloc9p(sizeof(LLMResponse));
    resp->text = nil;
    resp->success = 0;
    resp->status_code = 0;
    resp->error = nil;
    resp->duration_ms = 0;

    /* Build API endpoint URL */
    url = smprint("%s/chat/completions", llm->config.endpoint);

    /* Build headers with API key if provided */
    if(llm->config.api_key != nil && strlen(llm->config.api_key) > 0) {
        headers = smprint("Authorization: Bearer %s\r\n", llm->config.api_key);
    } else {
        headers = nil;
    }

    /* Build request body */
    body = build_request_body(llm, system_prompt, user_prompt, temperature);

    /* Send request */
    vlong start = nsec();
    response_body = http_post(llm, url, headers, body, &status_code);
    vlong end = nsec();
    resp->duration_ms = (end - start) / 1000000;

    free(url);
    if(headers != nil) free(headers);
    free(body);

    if(response_body == nil) {
        resp->error = estrdup9p(llm->last_error != nil ? llm->last_error :
                               "Failed to connect to LLM server");
        llm->errors_total++;
        return resp;
    }

    resp->status_code = status_code;

    if(status_code == 200) {
        resp->text = extract_content_from_response(response_body);
        resp->success = 1;
        llm->connected = 1;
    } else {
        resp->error = estrdup9p(response_body);
        llm->errors_total++;
        if(llm->last_error != nil) free(llm->last_error);
        llm->last_error = estrdup9p(response_body);
    }

    free(response_body);

    llm->requests_total++;

    return resp;
}

LLMResponse*
llm_complete(LLMBackend *llm, char *system_prompt, char *user_prompt)
{
    return llm_complete_with_temp(llm, system_prompt, user_prompt,
                                  llm->config.temperature);
}

LLMResponse*
llm_test(LLMBackend *llm)
{
    return llm_complete(llm,
                       "You are a helpful assistant.",
                       "Hello! Please respond with 'OK' if you can hear me.");
}

void
llm_response_free(LLMResponse *resp)
{
    if(resp == nil)
        return;

    free(resp->text);
    free(resp->error);
    free(resp);
}

int
llm_response_success(LLMResponse *resp)
{
    return resp != nil && resp->success;
}
