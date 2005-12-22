/*
 * Portions of this file Copyright 1999-2005 University of Chicago
 * Portions of this file Copyright 1999-2005 The University of Southern California.
 *
 * This file or a portion of this file is licensed under the
 * terms of the Globus Toolkit Public License, found at
 * http://www.globus.org/toolkit/download/license.html.
 * If you redistribute this file, with or without
 * modifications, you must include this notice in the file.
 */


#if !defined(GLOBUS_GRIDFTP_SERVER_EMBED_H)
#define GLOBUS_GRIDFTP_SERVER_EMBED_H 1

#include "globus_gridftp_server.h"

typedef struct globus_l_gfs_embed_handle_s * globus_gfs_embed_handle_t;

typedef enum
{
    GLOBUS_GFS_EMBED_EVENT_CLOSED = 1,
    GLOBUS_GFS_EMBED_EVENT_CONNECTED,
    GLOBUS_GFS_EMBED_EVENT_STOPPED
} globus_gfs_embed_event_t;

typedef globus_bool_t
(*globus_gfs_embed_event_cb_t)(
    globus_gfs_embed_handle_t           handle,
    globus_result_t                     result,
    globus_gfs_embed_event_t            event,
    void *                              user_arg);

/*
 *   start up an embedded gridftp server
 *
 *   conf_table
 *      this is a hash table filled with key=value pairs.  the keys
 *      and values are defined as they are in gridftp config file.
 */
globus_result_t
globus_gridftp_server_embed_start(
    globus_gfs_embed_handle_t *         handle,
    char *                              args[],
    globus_gfs_embed_event_cb_t         event_cb,
    void *                              user_arg);

/*
 *  stop the running embedded server.  calling this function will start
 *  the processes of shutting down the embedded server.  When it is
 *  completely shut down the event callback will be called with
 *  the GLOBUS_GRIDFTP_SERVER_EMEB_EVENT_STOPPED event.
 */
 
void
globus_gridftp_server_embed_stop(
    globus_gfs_embed_handle_t           handle);


#endif
