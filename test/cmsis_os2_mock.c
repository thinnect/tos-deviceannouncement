/*
 * A minimal cmsis_os2 mock to complete the device announcement unit tests.
 *
 * WARNING: Neither complete nor sane in some cases!
 *
 * Copyright Thinnect Inc. 2021
 * @license MIT
 */

#include "cmsis_os2.h"

#include <stdlib.h>
#include <string.h>

// Mocking time functions
extern uint32_t fake_localtime;

uint32_t osCounterGetSecond (void)
{
	return fake_localtime;
}

osMutexId_t osMutexNew (const osMutexAttr_t * attr)
{
	return (osMutexId_t)1;
}

osStatus_t osMutexAcquire (osMutexId_t mutex_id, uint32_t timeout)
{
	return osOK;
}

osStatus_t osMutexRelease (osMutexId_t mutex_id)
{
	return osOK;
}

osStatus_t osMutexDelete (osMutexId_t mutex_id)
{
	return osOK;
}


// Single thread flags emulation -----------------------------------------------

static uint32_t m_flags = 0;

osThreadId_t osThreadNew (osThreadFunc_t func, void *argument, const osThreadAttr_t *attr)
{
	return (osThreadId_t)1;
}

uint32_t osThreadFlagsWait (uint32_t flags, uint32_t options, uint32_t timeout)
{
	uint32_t fl = m_flags;
	m_flags = 0;
	return fl;
}

uint32_t osThreadFlagsSet (osThreadId_t thread_id, uint32_t flags)
{
	m_flags |= flags;
	return m_flags;
}


// Single-element single-queue emulation ---------------------------------------

static uint32_t m_mq_elem_size;
static void * m_mq_elem = NULL;


osMessageQueueId_t osMessageQueueNew (uint32_t msg_count, uint32_t msg_size, const osMessageQueueAttr_t *attr)
{
	m_mq_elem_size = msg_size;
	return (osMessageQueueId_t)1;
}

osStatus_t osMessageQueueDelete (osMessageQueueId_t mq_id)
{
	m_mq_elem_size = 0;
	return osOK;
}

osStatus_t osMessageQueueGet (osMessageQueueId_t mq_id, void *msg_ptr, uint8_t *msg_prio, uint32_t timeout)
{
	if (NULL != m_mq_elem)
	{
		memcpy(msg_ptr, m_mq_elem, m_mq_elem_size);
		free(m_mq_elem);
		m_mq_elem = NULL;
		return osOK;
	}
	return osErrorTimeout;
}

osStatus_t osMessageQueuePut (osMessageQueueId_t mq_id, const void *msg_ptr, uint8_t msg_prio, uint32_t timeout)
{
	if (NULL == m_mq_elem)
	{
		m_mq_elem = malloc(m_mq_elem_size);
		memcpy(m_mq_elem, msg_ptr, m_mq_elem_size);
		return osOK;
	}
	return osErrorTimeout;
}

// Timers do nothing

osTimerId_t osTimerNew (osTimerFunc_t func, osTimerType_t type, void *argument, const osTimerAttr_t *attr)
{
	return osOK;
}

osStatus_t osTimerStart (osTimerId_t timer_id, uint32_t ticks)
{
	return osOK;
}

osStatus_t osTimerStop (osTimerId_t timer_id)
{
	return osOK;
}

osStatus_t osTimerDelete (osTimerId_t timer_id)
{
	return osOK;
}

osStatus_t osDelay (uint32_t ticks)
{
	return osOK;
}
