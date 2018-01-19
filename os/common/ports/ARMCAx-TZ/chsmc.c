/*
    ChibiOS - Copyright (C) 2006..2016 Giovanni Di Sirio.

    This file is part of ChibiOS.

    ChibiOS is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    ChibiOS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * @file    chsmc.c
 * @brief   smc call module code.
 *
 * @addtogroup SMC
 * @{
 */
//#include <string.h>

#include "ch.h"
#include "chsmc.h"

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Module exported variables.                                                */
/*===========================================================================*/
thread_reference_t _ns_thread;

/*===========================================================================*/
/* Module local types.                                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/
static memory_pool_t svcs_pool;

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

static bool isAddrspaceValid(void *addr, uint32_t len)
{
  (void) addr;
  (void) len;
  return TRUE;
}

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   XXX Module initialization.
 *
 * @notapi
 */
void smcInit(void) {
  chPoolObjectInit(&svcs_pool, sizeof (smc_service_t),
                   chCoreAllocAlignedI);
}

registered_object_t *smcFindService(const char *name, uint32_t namelen) {
  (void) name;
  (void) namelen;
  return 0;
}

/**
 * @brief   The trusted service call entry point.
 * @post    A request is passed to the thread registered for the service.
 * @post    The service thread is resumed.
 *
 * @param[in] svc_handle  the handle of the service to be invoked
 * @param[in] svc_data    service request data, often a reference to a more complex structure
 * @param[in] svc_datalen size of the svc_data memory area
 *
 * @return              a value defined by the service.
 * @retval MSG_OK       a success value.
 * @retval MSG_RESET    if the service is unavailable.
 *
 * @notapi
 */
msg_t smcEntry(smc_service_t *svc_handle, smc_params_area_t svc_data, uint32_t svc_datalen) {
  registered_object_t *rop;
  msg_t r;

  asm("bkpt #0\n\t");
  if (!isAddrspaceValid(svc_data, svc_datalen))
    return MSG_RESET;
  rop = chFactoryFindObjectByPointer(svc_handle);
  if (rop == NULL)
    return MSG_RESET;
  svc_handle->svc_data = svc_data;
  svc_handle->svc_datalen = svc_datalen;
  chSysLock();
  chThdResumeS(&svc_handle->svct, MSG_OK);
  r = chThdSuspendS(&_ns_thread);
  chSysUnlock();
  chFactoryReleaseObject(rop);
  return r;
}

/**
 * @brief   Register the calling thread as a manager of the given service @p svc_name.
 * @post    The current thread is registered as manager for @p svc_name service.
 *          Only a thread will be the manager of the service @p svc_name.
 *
 * @param[in] svc_name    the name of the service.
 *
 * @return                a registered smc service object.
 * @retval NULL           if @p svc_name failed to be registered.
 *
 * @notapi
 */
registered_object_t *smcRegisterMeAsService(const char *svc_name)
{
  registered_object_t *rop;

  smc_service_t *svcp = chPoolAlloc(&svcs_pool);
  rop = chFactoryRegisterObject(svc_name, svcp);
  if (rop == NULL) {
    chPoolFree(&svcs_pool, svcp);
    return NULL;
  }
  return rop;
}

/**
 * @brief   The calling thread is a service and wait the arrival of a request.
 * @post    the service object is filled with the parameters of the requestor.
 *
 * @param[in] svcp          the service object reference.
 *
 * @return                  the reason of the awakening
 * @retval MSG_OK           a success value.
 * @retval MSG_TIMEOUT      a success value.
 *
 * @notapi
 */
msg_t smcServiceWaitRequest(smc_service_t *svcp)
{
  msg_t r;

  chDbgCheck(svcp != NULL);

  chSysLock();
  r = chThdSuspendTimeoutS(&svcp->svct, TIME_INFINITE);
  chSysUnlock();
  return r;
}

/** @} */
