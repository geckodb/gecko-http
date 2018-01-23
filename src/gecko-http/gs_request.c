// Copyright (C) 2017 Marcus Pinnecke
//
// This program is free software: you can redistribute it and/or modify it under the terms of the
// GNU General Public License as published by the Free Software Foundation, either user_port 3 of the License, or
// (at your option) any later user_port.
//
// This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with this program.
// If not, see .

// ---------------------------------------------------------------------------------------------------------------------
// I N C L U D E S
// ---------------------------------------------------------------------------------------------------------------------

#include <apr_strings.h>
#include <apr_tables.h>

#include <gecko-commons/containers/gs_hash.h>

#include <gecko-http/gs_request.h>
#include <gecko-http/gs_response.h>

// ---------------------------------------------------------------------------------------------------------------------
// D A T A T Y P E S
// ---------------------------------------------------------------------------------------------------------------------

typedef enum gs_request_body_e {
    GS_BODY_UNKNOWN,
    GS_MULTIPART
} gs_request_body_e;

typedef struct gs_request_t {
    apr_pool_t          *pool;
    char                *original;
    gs_hash_t           *fields;
    gs_hash_t           *form_data;
    gs_http_method_e     method;
    char                *resource;
    char                *content;
    bool                 is_valid;
    bool                 is_multipart;
    char                *boundary;
    gs_request_body_e    body_type;
} gs_request_t;

// ---------------------------------------------------------------------------------------------------------------------
// H E L P E R   P R O T O T Y P E S
// ---------------------------------------------------------------------------------------------------------------------

void parse_request(gs_request_t *request, int socket_desc);

// ---------------------------------------------------------------------------------------------------------------------
// I N T E R F A C E  I M P L E M E N T A T I O N
// ---------------------------------------------------------------------------------------------------------------------

GS_DECLARE(gs_status_t) gs_request_create(gs_request_t **request, int socket_desc)
{
    gs_request_t *result = GS_REQUIRE_MALLOC(sizeof(gs_request_t));
    apr_pool_create(&result->pool, NULL);
    gs_hash_create(&result->fields, 10, GS_STRING_COMP);
    gs_hash_create(&result->form_data, 10, GS_STRING_COMP);
    result->is_valid = false;
    result->is_multipart = false;
    result->content = NULL;
    result->body_type = GS_BODY_UNKNOWN;
    result->method = GS_HTTP_UNKNOWN;

    parse_request(result, socket_desc);

    *request = result;
    return GS_SUCCESS;
}

GS_DECLARE(gs_status_t) gs_request_dispose(gs_request_t **request_ptr)
{
    GS_REQUIRE_NONNULL(request_ptr)
    GS_REQUIRE_NONNULL(*request_ptr)
    gs_request_t *request = *request_ptr;
    apr_pool_destroy(request->pool);
    gs_hash_dispose(request->form_data);
    gs_hash_dispose(request->fields);
    free (request);
    *request_ptr = NULL;
    return GS_SUCCESS;
}

GS_DECLARE(gs_status_t) gs_request_raw(char **original, const gs_request_t *request)
{
    GS_REQUIRE_NONNULL(request);
    GS_REQUIRE_NONNULL(original);
    *original = request->original;
    return GS_SUCCESS;
}

GS_DECLARE(gs_status_t) gs_request_has_field(const gs_request_t *request, const char *key)
{
    GS_REQUIRE_NONNULL(request);
    GS_REQUIRE_NONNULL(key);
    return (gs_hash_get(request->fields, key, strlen(key)) != NULL);
}

GS_DECLARE(gs_status_t) gs_request_field_by_name(char const ** value, const gs_request_t *request, const char *key)
{
    GS_REQUIRE_NONNULL(request);
    GS_REQUIRE_NONNULL(key);
    GS_REQUIRE_NONNULL(value);
    *value = gs_hash_get(request->fields, key, strlen(key));
    return GS_SUCCESS;
}

GS_DECLARE(gs_status_t) gs_request_has_form(const gs_request_t *request, const char *key)
{
    GS_REQUIRE_NONNULL(request);
    GS_REQUIRE_NONNULL(key);
    return (gs_hash_get(request->form_data, key, strlen(key)) != NULL);
}

GS_DECLARE(gs_status_t) gs_request_form_by_name(char const **value, const gs_request_t *request, const char *key)
{
    GS_REQUIRE_NONNULL(request);
    GS_REQUIRE_NONNULL(key);
    GS_REQUIRE_NONNULL(value);
    *value = gs_hash_get(request->form_data, key, strlen(key));
    return GS_SUCCESS;
}

GS_DECLARE(gs_status_t) gs_request_has_content(const gs_request_t *request)
{
    return (request != NULL ? (request->content != NULL ? GS_TRUE : GS_FALSE) : GS_ILLEGALARG);
}

GS_DECLARE(gs_status_t) gs_request_get_content(char const **value, const gs_request_t *request)
{
    GS_REQUIRE_NONNULL(value);
    GS_REQUIRE_NONNULL(request);
    if (gs_request_has_content(request) == GS_TRUE) {
        *value = request->content;
        return GS_SUCCESS;
    } else {
        *value = NULL;
        return GS_FAILED;
    }

}

GS_DECLARE(gs_status_t) gs_request_method(gs_http_method_e *method, const gs_request_t *request)
{
    GS_REQUIRE_NONNULL(request);
    GS_REQUIRE_NONNULL(method);
    *method = request->method;
    return GS_SUCCESS;
}

GS_DECLARE(gs_status_t) gs_request_is_method(const gs_request_t *request, gs_http_method_e method)
{
    GS_REQUIRE_NONNULL(request);
    return (request->method == method);
}

GS_DECLARE(gs_status_t) gs_request_resource(char **resource, const gs_request_t *request)
{
    GS_REQUIRE_NONNULL(request);
    GS_REQUIRE_NONNULL(resource);
    *resource = request->resource;
    return GS_SUCCESS;
}

GS_DECLARE(gs_status_t) gs_request_is_valid(const gs_request_t *request)
{
    GS_REQUIRE_NONNULL(request);
    return (request->is_valid);
}

void parse_request(gs_request_t *request, int socket_desc)
{
    char message_buffer[10240];

    // Read the request from the client socket
    recv(socket_desc, message_buffer, sizeof(message_buffer), 0);
    request->original = apr_pstrdup(request->pool, message_buffer);

    // Parse line by line
    char *last_line;
    char *line = apr_strtok(apr_pstrdup(request->pool, message_buffer), "\r\n", &last_line );

    enum parsing_modus { PARSE_HTTP_METHOD, PARSE_HTTP_FIELDS, PARSE_HTTP_CONTENT }
            parsing_modus = PARSE_HTTP_METHOD;

    while(line != NULL) {
        // Example request
        //      POST / HTTP/1.1
        //      Host: localhost:35497
        //      User-Agent: curl/7.43.0
        //      Accept: */*
        //      Content-Length: 190
        //      Expect: 100-continue
        //      Content-Type: multipart/form-data; boundary=------------------------5c30c9ccb5c18fb1
        //      (empty line)

        // until an empty line is reached, the header is being parsed
        if(strlen(line)) {
            switch (parsing_modus) {
                case PARSE_HTTP_METHOD: {
                    // length of the sub string before the first whitespace, i.e., the method "POST", "GET",...
                    int method_ends_at = strcspn(line, " ");
                    if (method_ends_at > 0) {
                        char *method_str = apr_pstrndup(request->pool, line, method_ends_at);
                        if (!strcmp(method_str, "OPTIONS"))
                            request->method = GS_HTTP_OPTIONS;
                        else if (!strcmp(method_str, "GET"))
                            request->method = GS_HTTP_GET;
                        else if (!strcmp(method_str, "HEAD"))
                            request->method = GS_HTTP_HEAD;
                        else if (!strcmp(method_str, "POST"))
                            request->method = GS_HTTP_POST;
                        else if (!strcmp(method_str, "PUT"))
                            request->method = GS_HTTP_PUT;
                        else if (!strcmp(method_str, "DELETE"))
                            request->method = GS_HTTP_DELETE;
                        else if (!strcmp(method_str, "TRACE"))
                            request->method = GS_HTTP_TRACE;
                        else if (!strcmp(method_str, "CONNECT"))
                            request->method = GS_HTTP_CONNECT;
                        else
                            request->method = GS_HTTP_UNKNOWN;

                        // length of the sub string before the next white space after method first whitespace, i.e., "<RESOURCE>"
                        int resource_ends_at = strcspn(line + method_ends_at + 1, " ");
                        if (resource_ends_at > 0) {
                            request->resource = apr_pstrndup(request->pool, line + method_ends_at + 1,
                                                             resource_ends_at);
                            request->is_valid = true;
                        }
                    }
                    parsing_modus = PARSE_HTTP_FIELDS;
                } break;
                case PARSE_HTTP_FIELDS: {
                    // length of the sub string before the assignment, i.e., fields "Host", "User-Agent",...
                    int assignment_at = strcspn(line, ":");
                    if (assignment_at > 0) {
                        char *field_name = apr_pstrndup(request->pool, line, assignment_at);
                        char *field_value = apr_pstrndup(request->pool, line + assignment_at + 2,
                                                         strlen(line) + 2 - assignment_at);
                        gs_hash_set(request->fields, field_name, strlen(field_name), field_value);
                    }
                } break;
                case PARSE_HTTP_CONTENT: {
                    // The double new line for the content part is reached. Thus, parse content
                    request->content = (request->content == NULL) ?
                                       apr_pstrdup(request->pool, line) :
                                       apr_pstrcat(request->pool, request->content, line, NULL);
                } break;
                default: panic("Unknown request parsing mode %d!", parsing_modus);
            }
        }
        if (strstr(last_line, "\n\r\n") == last_line) {
            parsing_modus = PARSE_HTTP_CONTENT;
        }
        line = apr_strtok( NULL, "\r\n",  &last_line);
    }

    // in case the request requires an additional response to proceed, send this response
    const char *expect_key = "Expect";
    const char *expect_value = gs_hash_get(request->fields, expect_key, strlen(expect_key));
    if (request->is_valid && expect_value && strlen(expect_value)) {
        int response_code_expected_at = strcspn(expect_value, "-");
        if (response_code_expected_at > 0 &&
            !strcmp(apr_pstrndup(request->pool, expect_value, response_code_expected_at), "100")) {

            // send 100 continue
            gs_response_t response;
            gs_response_create(&response);
            gs_response_end(&response, HTTP_STATUS_CODE_100_CONTINUE);
            char *response_text = gs_response_pack(&response);
            write(socket_desc, response_text, strlen(response_text));
            free (response_text);

            // read response, and set current line pointer to that content
            recv(socket_desc, message_buffer, sizeof(message_buffer), 0);
            line = apr_strtok(apr_pstrdup(request->pool, message_buffer), "\r\n", &last_line );
        }
    }

    // in case the request contains form-data resp. is multipart, read the subsequent data
    const char *content_type_key = "Content-Type";
    const char *content_type_value = gs_hash_get(request->fields, content_type_key, strlen(content_type_key));
    if (request->is_valid && content_type_value && strlen(content_type_value)) {
        // if the content is multipart, then there is a boundary definition which is starts after ";"
        if (strstr(content_type_value, ";")) {
            int multipart_at = strcspn(content_type_value, ";");
            if (multipart_at > 0 &&
                !strcmp(apr_pstrndup(request->pool, content_type_value, multipart_at), "multipart/form-data")) {
                // this request is multi-part
                int boundary_def_at = strcspn(content_type_value, "=");
                if (boundary_def_at > 0) {
                    request->is_multipart = true;
                    int boundary_def_prefix = boundary_def_at + 1;
                    request->boundary = apr_pstrndup(request->pool, content_type_value + boundary_def_prefix,
                                                     strlen(content_type_value) - boundary_def_prefix);
                    request->body_type = GS_MULTIPART;
                }
            }
        }
    }

    // parse body part
    switch (request->body_type) {
        case GS_MULTIPART: {
            bool read_name = true;
            char *attribute_name = NULL, *attribute_value = NULL;
            while(line != NULL) {
                if (!strstr(line, request->boundary)) {
                    if (read_name) {
                        if (strstr(line, "form-data; name=")) {
                            // read form name
                            int name_starts_at = strcspn(line, "\"") + 1;
                            int name_end_at = strcspn(line + name_starts_at, "\"");
                            if (name_end_at < strlen(line)) {
                                attribute_name = apr_pstrndup(request->pool, line + name_starts_at, name_end_at);
                                read_name = false;
                            }
                        }
                    } else {
                        // read form data
                        attribute_value = apr_pstrdup(request->pool, line);
                        read_name = true;
                    }
                }
                if (attribute_name != NULL && attribute_value != NULL) {
                    gs_hash_set(request->form_data, attribute_name, strlen(attribute_name), attribute_value);
                    attribute_name = attribute_value = NULL;
                }
                line = apr_strtok( NULL, "\r\n",  &last_line);
            }
        } break;
        default:
        warn("Unknown or empty body type for request: '%s'", request->original);
            break;
    }
}