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

#include "globus_common.h"
#include "globus_scheduler_event_generator.h"
#include "globus_scheduler_event_generator_app.h"
#include "version.h"

#include "ltdl.h"

static
int
globus_l_seg_activate(void);

static
int
globus_l_seg_deactivate(void);

globus_module_descriptor_t globus_i_scheduler_event_generator_module =
{
    "globus_scheduler_event_generator",
    globus_l_seg_activate,
    globus_l_seg_deactivate,
    NULL,
    NULL,
    &local_version,
    NULL
};

static lt_dlhandle                      globus_l_seg_scheduler_handle;
static globus_module_descriptor_t *     globus_l_seg_scheduler_module;
static time_t                           globus_l_seg_timestamp;
static globus_mutex_t                   globus_l_seg_mutex;
static globus_scheduler_event_generator_fault_handler_t
                                        globus_l_seg_fault_handler;
static void *                           globus_l_seg_fault_arg;
static globus_scheduler_event_generator_event_handler_t
                                        globus_l_seg_event_handler;
static void *                           globus_l_seg_event_arg;
static
int
globus_l_seg_activate(void)
{
    int                                 rc;

    rc = lt_dlinit();
    if (rc != 0)
    {
        goto error;
    }
    rc = globus_module_activate(GLOBUS_COMMON_MODULE);

    if (rc != GLOBUS_SUCCESS)
    {
        goto dlexit_error;
    }

    globus_l_seg_scheduler_handle = NULL;
    globus_l_seg_scheduler_module = NULL;
    globus_l_seg_timestamp = 0;
    globus_l_seg_fault_handler = NULL;
    globus_l_seg_fault_arg = NULL;
    globus_l_seg_event_handler = NULL;
    globus_l_seg_event_arg = NULL;

    globus_mutex_init(&globus_l_seg_mutex, NULL);
    return 0;

dlexit_error:
    lt_dlexit();
error:
    return 1;
}
/* globus_l_seg_activate() */

static
int
globus_l_seg_deactivate(void)
{
    if (globus_l_seg_scheduler_module)
    {
        globus_module_deactivate(globus_l_seg_scheduler_module);
    }

    if (globus_l_seg_scheduler_handle)
    {
        lt_dlclose(globus_l_seg_scheduler_handle);
    }

    globus_mutex_destroy(&globus_l_seg_mutex);
    globus_module_deactivate(GLOBUS_COMMON_MODULE);

    lt_dlexit();
    return 0;
}
/* globus_l_seg_deactivate() */

/**
 * Send an arbitrary SEG notification.
 * @ingroup seg_api
 *
 * @param format
 *     Printf-style format of the SEG notification message
 * @param ...
 *     Varargs which will be interpreted as per format.
 *
 * @retval GLOBUS_SUCCESS
 *     Scheduler message sent or queued.
 * @retval GLOBUS_SEG_ERROR_NULL
 *     Null format.
 * @retval GLOBUS_SEG_ERROR_INVALID_FORMAT
 *     Unable to determine length of formatted string.
 */
globus_result_t
globus_scheduler_event(
    const char * format,
    ...)
{
    globus_result_t                     result = GLOBUS_SUCCESS;
    char *                              buf;
    va_list                             ap;
    int                                 length;
    globus_scheduler_event_t            event;

    if (format == NULL)
    {
        result = GLOBUS_SEG_ERROR_NULL;

        goto error;
    }

    va_start(ap, format);
    length = globus_libc_vprintf_length(format, ap);
    va_end(ap);

    if (length <= 0)
    {
        result = GLOBUS_SEG_ERROR_INVALID_FORMAT(format);

        goto error;
    }

    buf = globus_libc_malloc(length+1);
    if (buf == NULL)
    {
        result = GLOBUS_SEG_ERROR_OUT_OF_MEMORY;

        goto error;
    }

    va_start(ap, format);
    vsprintf(buf, format, ap);
    va_end(ap);

    event.raw.event_type = GLOBUS_SCHEDULER_EVENT_RAW;
    event.raw.raw_event = buf;

    if (globus_l_seg_event_handler != NULL)
    {
        (*globus_l_seg_event_handler)(
                globus_l_seg_event_arg,
                &event);
    }

    free(buf);

error:
    return result;
}
/* globus_scheduler_event() */

/**
 * Send a job pending event to the JobSchedulerMonitor implementation.
 * @ingroup seg_api
 *
 * @param timestamp
 *        Timestamp to use for the event. If set to 0, the time which
 *        this function was called is used.
 * @param jobid
 *        String indicating the scheduler-specific name of the job.
 *
 * @retval GLOBUS_SUCCESS
 *     Scheduler message sent or queued.
 * @retval GLOBUS_SEG_ERROR_NULL
 *     Null jobid.
 * @retval GLOBUS_SEG_ERROR_INVALID_FORMAT
 *     Unable to determine length of formatted string.
 */
globus_result_t
globus_scheduler_event_pending(
    time_t                              timestamp,
    const char *                        jobid)
{
    globus_scheduler_event_t            event;

    if (jobid == NULL)
    {
        return GLOBUS_SEG_ERROR_NULL;
    }

    event.pending.event_type = GLOBUS_SCHEDULER_EVENT_PENDING;
    event.pending.job_id = jobid;
    event.pending.timestamp = timestamp;
    
    if (globus_l_seg_event_handler != NULL)
    {
        (*globus_l_seg_event_handler)(
                globus_l_seg_event_arg,
                &event);
    }

    return GLOBUS_SUCCESS;
}
/* globus_scheduler_event_pending() */

/**
 * Send a job active event to the JobSchedulerMonitor implementation.
 * @ingroup seg_api
 *
 * @param timestamp
 *        Timestamp to use for the event. If set to 0, the time which
 *        this function was called is used.
 * @param jobid
 *        String indicating the scheduler-specific name of the job.
 *
 * @retval GLOBUS_SUCCESS
 *     Scheduler message sent or queued.
 * @retval GLOBUS_SEG_ERROR_NULL
 *     Null jobid.
 * @retval GLOBUS_SEG_ERROR_INVALID_FORMAT
 *     Unable to determine length of formatted string.
 */
globus_result_t
globus_scheduler_event_active(
    time_t                              timestamp,
    const char *                        jobid)
{
    globus_scheduler_event_t            event;

    if (jobid == NULL)
    {
        return GLOBUS_SEG_ERROR_NULL;
    }

    event.active.event_type = GLOBUS_SCHEDULER_EVENT_ACTIVE;
    event.active.job_id = jobid;
    event.active.timestamp = timestamp;
    
    if (globus_l_seg_event_handler != NULL)
    {
        (*globus_l_seg_event_handler)(
                globus_l_seg_event_arg,
                &event);
    }
    return GLOBUS_SUCCESS;
}

/**
 * Send a job failed event to the JobSchedulerMonitor implementation.
 * @ingroup seg_api
 *
 * @param timestamp
 *        Timestamp to use for the event. If set to 0, the time which
 *        this function was called is used.
 * @param jobid
 *        String indicating the scheduler-specific name of the job.
 * @param failure_code
 *        Failure code of the process if known.
 *
 * @retval GLOBUS_SUCCESS
 *     Scheduler message sent or queued.
 * @retval GLOBUS_SEG_ERROR_NULL
 *     Null jobid.
 * @retval GLOBUS_SEG_ERROR_INVALID_FORMAT
 *     Unable to determine length of formatted string.
 */
globus_result_t
globus_scheduler_event_failed(
    time_t                              timestamp,
    const char *                        jobid,
    int                                 failure_code)
{
    globus_scheduler_event_t            event;

    if (jobid == NULL)
    {
        return GLOBUS_SEG_ERROR_NULL;
    }

    event.failed.event_type = GLOBUS_SCHEDULER_EVENT_FAILED;
    event.failed.job_id = jobid;
    event.failed.timestamp = timestamp;
    event.failed.failure_code = failure_code;
    
    if (globus_l_seg_event_handler != NULL)
    {
        (*globus_l_seg_event_handler)(
                globus_l_seg_event_arg,
                &event);
    }
    return GLOBUS_SUCCESS;
}

/**
 * Send a job done event to the JobSchedulerMonitor implementation.
 * @ingroup seg_api
 *
 * @param timestamp
 *        Timestamp to use for the event. If set to 0, the time which
 *        this function was called is used.
 * @param jobid
 *        String indicating the scheduler-specific name of the job.
 * @param exit_code
 *        Exit code of the process if known.
 *
 * @retval GLOBUS_SUCCESS
 *     Scheduler message sent or queued.
 * @retval GLOBUS_SEG_ERROR_NULL
 *     Null jobid.
 * @retval GLOBUS_SEG_ERROR_INVALID_FORMAT
 *     Unable to determine length of formatted string.
 */
globus_result_t
globus_scheduler_event_done(
    time_t                              timestamp,
    const char *                        jobid,
    int                                 exit_code)
{
    globus_scheduler_event_t            event;

    if (jobid == NULL)
    {
        return GLOBUS_SEG_ERROR_NULL;
    }

    event.done.event_type = GLOBUS_SCHEDULER_EVENT_DONE;
    event.done.job_id = jobid;
    event.done.timestamp = timestamp;
    event.done.exit_code = exit_code;
    
    if (globus_l_seg_event_handler != NULL)
    {
        (*globus_l_seg_event_handler)(
                globus_l_seg_event_arg,
                &event);
    }
    return GLOBUS_SUCCESS;
}

/**
 * Get the timestamp for the earliest event an SEG module should send.
 * @ingroup seg_api
 *
 * @param timestamp
 *     Pointer to a time_t which will be set to the timestamp passed to the
 *     SEG executable. The module should not send any events which occur prior
 *     to this timestamp.
 * @retval GLOBUS_SEG_ERROR_NULL
 *     Null timestamp.
 * @retval GLOBUS_SUCCESS
 *     Timestamp value updated. If the timestamp was not set on the SEG
 *     command-line, then the value pointed to by @a timestamp will be set to
 *     0.
 */
globus_result_t
globus_scheduler_event_generator_get_timestamp(
    time_t *                            timestamp)
{
    if (timestamp == NULL)
    {
        return GLOBUS_SEG_ERROR_NULL;
    }
    *timestamp = globus_l_seg_timestamp;

    return GLOBUS_SUCCESS;
}
/* globus_scheduler_event_generator_get_timestamp() */


globus_result_t
globus_scheduler_event_generator_set_timestamp(
    time_t                              timestamp)
{
    globus_result_t                     result = GLOBUS_SUCCESS;

    globus_mutex_lock(&globus_l_seg_mutex);
    if (globus_l_seg_timestamp != 0)
    {
        result = GLOBUS_SEG_ERROR_ALREADY_SET;
        goto error;
    }
    globus_l_seg_timestamp = timestamp;

    globus_mutex_unlock(&globus_l_seg_mutex);

    return GLOBUS_SUCCESS;

error:
    globus_mutex_unlock(&globus_l_seg_mutex);
    return result;
}
/* globus_scheduler_event_generator_set_timestamp() */

/**
 * Load a scheduler event generator module.
 *
 * If the module name begins with "/" then it is interpreted as a path name;
 * otherwise, the name is constructed using the convention:
 * @code $GLOBUS_LOCATION/lib/libglobus_seg_$MODULE_NAME_$FLAVOR.so@endcode,
 * where $GLOBUS_LOCATION is determined from the environment, $MODULE_NAME is
 * determined from the @a module_name parameter to the function, and $FLAVOR
 * is determined at compile time.
 *
 * @param module_name
 *     Name of the shared object to load.
 * @retval GLOBUS_SEG_ERROR_ALREADY_SET
 *     Already loaded a module.
 * @retval GLOBUS_SEG_ERROR_LOADING_MODULE
 *     Unable to load shared module.
 * @retval GLOBUS_SEG_ERROR_INVALID_MODULE
 *     Unable to find the loaded module's globus_module_descriptor_t
 */
globus_result_t
globus_scheduler_event_generator_load_module(
    const char *                        module_name)
{
    globus_result_t                     result;
    int                                 rc;
    const char *                        flavor_name = GLOBUS_FLAVOR_NAME;
    const char *                        symbol_name
            = "globus_scheduler_event_module_ptr";
    char *                              globus_loc = NULL;
    char *                              module_path = NULL;

    globus_mutex_lock(&globus_l_seg_mutex);
    if (globus_l_seg_scheduler_handle != NULL)
    {
        result = GLOBUS_SEG_ERROR_ALREADY_SET;

        goto unlock_error;
    }

    if (module_name[0] != '/')
    {
        result = globus_location(&globus_loc);

        if (result != GLOBUS_SUCCESS)
        {
            result = GLOBUS_SEG_ERROR_OUT_OF_MEMORY;

            goto unlock_error;
        }

        module_path = globus_libc_malloc(strlen(globus_loc) +
                strlen("%s/lib/libglobus_seg_%s_%s.la") + strlen(module_name) +
                strlen(flavor_name));

        if (module_path == NULL)
        {
            result = GLOBUS_SEG_ERROR_OUT_OF_MEMORY;

            goto free_globus_location_error;
        }

        sprintf(module_path, "%s/lib/libglobus_seg_%s_%s.la",
                globus_loc, module_name, flavor_name);
        globus_l_seg_scheduler_handle = lt_dlopen(module_path);
    }
    else
    {
        globus_l_seg_scheduler_handle = lt_dlopen(module_name);
    }

    if (globus_l_seg_scheduler_handle == NULL)
    {
        result = GLOBUS_SEG_ERROR_LOADING_MODULE(module_name,
                lt_dlerror());

        goto free_module_path_error;
    }
    globus_l_seg_scheduler_module = (globus_module_descriptor_t *)
            lt_dlsym(globus_l_seg_scheduler_handle, symbol_name);

    if (globus_l_seg_scheduler_module == NULL)
    {
        result = GLOBUS_SEG_ERROR_INVALID_MODULE(module_name, lt_dlerror());

        goto dlclose_error;
    }

    rc = globus_module_activate(globus_l_seg_scheduler_module);

    if (rc != 0)
    {
        result = GLOBUS_SEG_ERROR_INVALID_MODULE(
                module_name,
                "activation failed");

        goto dlclose_error;
    }

    globus_mutex_unlock(&globus_l_seg_mutex);

    globus_libc_free(globus_loc);
    globus_libc_free(module_path);

    return GLOBUS_SUCCESS;

dlclose_error:
    lt_dlclose(globus_l_seg_scheduler_handle);
    globus_l_seg_scheduler_handle = NULL;
    globus_l_seg_scheduler_module = NULL;
free_module_path_error:
    if (module_path != NULL)
    {
        globus_libc_free(module_path);
    }
free_globus_location_error:
    if (globus_loc != NULL)
    {
        globus_libc_free(globus_loc);
    }
unlock_error:
    globus_mutex_unlock(&globus_l_seg_mutex);
    return result;
}
/* globus_scheduler_event_generator_load_module() */

globus_result_t
globus_scheduler_event_generator_set_fault_handler(
    globus_scheduler_event_generator_fault_handler_t
                                        fault_handler,
    void *                              user_arg)
{
    globus_result_t                     result = GLOBUS_SUCCESS;

    globus_mutex_lock(&globus_l_seg_mutex);

    if (globus_l_seg_fault_handler != NULL)
    {
        result = GLOBUS_SEG_ERROR_ALREADY_SET;

        goto unlock_error;
    }

    globus_l_seg_fault_handler = fault_handler;
    globus_l_seg_fault_arg = user_arg;

    globus_mutex_unlock(&globus_l_seg_mutex);

    return result;

unlock_error:
    globus_mutex_unlock(&globus_l_seg_mutex);

    return result;
}
/* globus_scheduler_event_generator_set_fault_handler() */

globus_result_t
globus_scheduler_event_generator_set_event_handler(
    globus_scheduler_event_generator_event_handler_t
                                        event_handler,
    void *                              user_arg)
{
    globus_result_t                     result = GLOBUS_SUCCESS;

    globus_mutex_lock(&globus_l_seg_mutex);

    if (globus_l_seg_event_handler != NULL)
    {
        result = GLOBUS_SEG_ERROR_ALREADY_SET;

        goto unlock_error;
    }

    globus_l_seg_event_handler = event_handler;
    globus_l_seg_event_arg = user_arg;

    globus_mutex_unlock(&globus_l_seg_mutex);

    return result;

unlock_error:
    globus_mutex_unlock(&globus_l_seg_mutex);

    return result;
}
/* globus_scheduler_event_generator_set_event_handler() */

void
globus_scheduler_event_generator_fault(
    globus_result_t                     result)
{
    if (globus_l_seg_fault_handler != NULL)
    {
        (*globus_l_seg_fault_handler)(globus_l_seg_fault_arg, result);
    }
}
/* globus_scheduler_event_generator_fault() */
