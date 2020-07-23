/**
 * Device announcement support functions.
 *
 * Copyright Thinnect Inc. 2019
 * @license MIT
 * @author Raido Pahtma
 */
#include "device_announcement.h"
#include "device_features.h"

#include "cmsis_os2_ext.h"

#include "time64.h"

uint32_t lifetime_sec() {
	return osCounterGetSecond() + 100;
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
