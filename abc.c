/*
* Copyright (c) Dish Technologies
* All Rights Reserved
* Confidential Property of Dish Technologies
*
* THE MATERIAL CONTAINED HEREIN MAY ONLY BE USED SUBJECT TO AN EXECUTED
* AGREEMENT BETWEEN THE USER AND DISH TECHNOLOGIES
* DISCLOSURE OF THIS MATERIAL IS STRICTLY PROHIBITED EXCEPT SUBJECT TO
* THE TERMS OF SUCH AN AGREEMENT.
*/
/**
* \file   error_detection.c
* \author Rakesh Ramesh
* \date   01-12-2026
*
* \brief  player health error detection
*/
#include <pthread.h>

#include "player_health.h"
#ifdef RTR_SUPPORT
#include "rtr_alerts.h"
#include "esosal_realtime_tracker.h"
#endif /* RTR_SUPPORT */

#define MAX_WORKLOADS                                 (36)
#define MAX_WORKFORCE_THREADS                         (4)      /* At anytime only 4 decoders are active */
#define ERROR_DETECTION_INTERVAL_SEC                  (1)      /* The error detection runs on polling mechanism once every specified seconds*/
#define WORKLOAD_WAIT_TIMEOUT_SEC                     (5)      /* If there is no workload for the threads the wait timesout at 5s.
                                                                  This is done to prevent a hang. The wait starts again post the timeout. */

typedef struct
{
   pthread_mutex_t            workload_mutex;            /* Synchronize workload queue access. */
   pthread_cond_t             new_work_added;            /* Signals any worker threads that are waiting for work. */
   size_t                     num_workloads;             /* Number of workloads queued right now. */
   size_t                     pop_index;                 /* Pop the next workload from this index. */
   size_t                     push_index;                /* Push a new workload to this index. */
   ERROR_DETECTION_WORKLOAD   workloads[MAX_WORKLOADS];  /* Fixed size, circular workload queue. */
} WORKFORCE_X;

static WORKFORCE_X gx_workforce =
{
   .workload_mutex   = PTHREAD_MUTEX_INITIALIZER,
   .new_work_added   = PTHREAD_COND_INITIALIZER
};

static void error_detection_handle_wait_expired(HAL_PLAY_OUTPUT output);

/**
 *  \brief Obtains the next availiable workload.
 *
 *  \param px_workload Points to the workload to populate.
 *
 *  \return PLAYER_HEALTH_ERROR_E
 *
 *  \author Kyle Kibiloski
 *
 *  \details If no workloads are availiable, the thread will wait.
 */
static PLAYER_HEALTH_ERROR_E error_detection_get_workload(ERROR_DETECTION_WORKLOAD *px_workload)
{
   PLAYER_HEALTH_ERROR_E e_ret_val = ePLAYER_HEALTH_FAILURE;
   struct timespec timeout;
   int i_ret;

   if (NULL == px_workload)
   {
      LOG_ERROR("%s(): Invalid argument passed", __func__);
   }
   else
   {
      if (0 == pthread_mutex_lock(&gx_workforce.workload_mutex))
      {
         while (0 == gx_workforce.num_workloads)
         {
            clock_gettime(CLOCK_REALTIME, &timeout);
            timeout.tv_sec += WORKLOAD_WAIT_TIMEOUT_SEC;
            i_ret = pthread_cond_timedwait(&gx_workforce.new_work_added, &gx_workforce.workload_mutex, &timeout);

            if (ETIMEDOUT == i_ret)
            {
               LOG_1("%s(): Timeout waiting for workload", __func__);
            }
            else if (0 != i_ret)
            {
               LOG_ERROR("%s(): pthread_cond_timedwait error: %d", __func__, i_ret);
            }
         }
         /* Pop the next workload from the queue. */
         *px_workload = gx_workforce.workloads[gx_workforce.pop_index];
         /* This is a circular queue, so the index will wrap to the start if it reaches MAX_WORKLOADS */
         gx_workforce.pop_index = (gx_workforce.pop_index + 1) % MAX_WORKLOADS;
         gx_workforce.num_workloads--;
         e_ret_val = ePLAYER_HEALTH_SUCCESS;

         pthread_mutex_unlock(&gx_workforce.workload_mutex);
      }
      else
      {
         LOG_ERROR("%s(): Unrecoverable mutex error occured!", __func__);
      }
   }
   return e_ret_val;
}
