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
// If not, see <http://www.gnu.org/licenses/>.

#pragma once

// ---------------------------------------------------------------------------------------------------------------------
// I N C L U D E S
// ---------------------------------------------------------------------------------------------------------------------

#include <gecko-commons/gs_error.h>
#include <gecko-commons/gecko-commons.h>

#include <gecko-http/gs_response.h>
#include <gecko-http/gs_request.h>
#include <gecko-http/gs_event.h>

// ---------------------------------------------------------------------------------------------------------------------
// C O N S T A N T S
// ---------------------------------------------------------------------------------------------------------------------

static char response[] =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html: charset=UTF-8\r\n\r\n"
        "<!doctype html>\r\n"
        "<html><head>MyTitle</head><body><h1>Hello World</h1></body></html>\r\n";

__BEGIN_DECLS

// ---------------------------------------------------------------------------------------------------------------------
// C O N F I G
// ---------------------------------------------------------------------------------------------------------------------

#define SERVER_SELECT_TIMEOUT 5

// ---------------------------------------------------------------------------------------------------------------------
// D A T A   T Y P E S
// ---------------------------------------------------------------------------------------------------------------------

typedef struct gs_server_t gs_server_t;

typedef struct gs_server_pool_t gs_server_pool_t;

typedef void (*router_t)(gs_system_t *system, const gs_request_t *req, gs_response_t *res);

// ---------------------------------------------------------------------------------------------------------------------
// I N T E R F A C E   F U N C T I O N S
// ---------------------------------------------------------------------------------------------------------------------

GS_DECLARE(gs_status_t) gs_server_create(gs_server_t **server, unsigned short port, gs_dispatcher_t *dispatcher);

GS_DECLARE(gs_status_t) gs_server_router_add(gs_server_t *server, const char *resource, router_t router);

GS_DECLARE(gs_status_t) gs_server_start(gs_server_t *server, gs_system_t *system, router_t catch);

GS_DECLARE(unsigned short) gs_server_port(const gs_server_t *server);

GS_DECLARE(u64) gs_server_num_requests(const gs_server_t *server);

GS_DECLARE(gs_status_t) gs_server_dispose(gs_server_t *server);

GS_DECLARE(gs_status_t) gs_server_shutdown(gs_server_t *server);

GS_DECLARE(gs_status_t) gs_server_pool_create(gs_server_pool_t **server_set, gs_dispatcher_t *dispatcher,
                                              unsigned short gateway_port, const char *gateway_resource, router_t router,
                                              size_t num_servers);

GS_DECLARE(gs_status_t) gs_server_pool_dispose(gs_server_pool_t *pool);

GS_DECLARE(gs_status_t) gs_server_pool_router_add(gs_server_pool_t *pool, const char *resource, router_t router);

GS_DECLARE(gs_status_t) gs_server_pool_start(gs_server_pool_t *pool, gs_system_t *system, router_t catch);

GS_DECLARE(gs_status_t) gs_server_pool_handle_events(const gs_event_t *event);

GS_DECLARE(gs_status_t) gs_server_pool_shutdown(gs_server_pool_t *pool);

GS_DECLARE(unsigned short) gs_server_pool_get_gateway_port(gs_server_pool_t *pool);

GS_DECLARE(size_t) gs_server_pool_get_num_of_servers(gs_server_pool_t *pool);

GS_DECLARE(gs_status_t) gs_server_pool_cpy_port_list(unsigned short *dst, const gs_server_pool_t *pool);

unsigned short gs_server_pool_next_port(gs_server_pool_t *pool);

GS_DECLARE(void) gs_server_pool_print(FILE *file, gs_server_pool_t *pool);

__END_DECLS