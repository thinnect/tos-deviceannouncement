/**
 * Device announcement setup.
 *
 * Use a timer to poll the announcement module.
 *
 * Copyright Thinnect Inc. 2019
 * @license MIT
 * @author Raido Pahtma
 */
#include "device_announcement.h"
#include "device_features.h"

#include "cmsis_os2_ext.h"

#include "time64.h"

#include "loglevels.h"
#define __MODUUL__ "aapp"
#define __LOG_LEVEL__ (LOG_LEVEL_announcement_app & BASE_LOG_LEVEL)
#include "log.h"

// How often the announcement module is polled
#define DEVICE_ANNOUNCEMENT_POLL_PERIOD_S 60

static device_announcer_t m_announcer;
static comms_sleep_controller_t m_radio_ctrl;


static void announcement_loop(void * arg)
{
	for(;;)
	{
		osDelay(DEVICE_ANNOUNCEMENT_POLL_PERIOD_S*1000UL);
		if (deva_poll())
		{
			debug1("snt");
		}
	}
}

int announcement_app_init(comms_layer_t * radio, bool manage_radio, uint16_t period_s)
{
	debug1("deva init");
	deva_init();
	if(manage_radio) {
		deva_add_announcer(&m_announcer, radio, &m_radio_ctrl, period_s);
	}
	else {
		deva_add_announcer(&m_announcer, radio, NULL, period_s);
	}

    const osThreadAttr_t annc_thread_attr = { .name = "annc"};
    osThreadNew(announcement_loop, NULL, &annc_thread_attr);

	return 0;
}

// Handle current tangling "dependencies"
//extern uint32_t osCounterMilliGet();

uint32_t localtime_sec() {
	return osCounterGetSecond();
}

uint32_t lifetime_sec() {
	return localtime_sec() + 100;
}

uint32_t lifetime_boots() {
	return 1;
}

time64_t realtimeclock() {
	return (time64_t)(-1);
}

uint8_t radio_channel() {
	return DEFAULT_RADIO_CHANNEL;
}

void nx_uuid_application(nx_uuid_t* uuid) {
	memcpy(uuid, UUID_APPLICATION_BYTES, 16);
}
