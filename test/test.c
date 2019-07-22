/**
 * Hand-crafted tests for announcement module.
 *
 * Copyright Thinnect 2019.
 * @license MIT
 */
#include <string.h>
#include <inttypes.h>

#include "loglevels.h"
#define __MODUUL__ "test"
#define __LOG_LEVEL__ ( LOG_LEVEL_test & BASE_LOG_LEVEL )
#include "log.h"

#include "DeviceSignature.h"
#include "SignatureAreaFile.h"
#include "mist_comm.h"
#include "mist_comm_am.h"
#include "device_announcement.h"

#include "nesc_to_c_compat.h"

uint32_t test_errors = 0;
uint8_t packets_sent = 0;

// Mocking time functions
uint32_t fake_localtime = 0;

uint32_t localtime_sec() {
	return fake_localtime;
}
uint32_t lifetime_sec() {
	return localtime_sec() + 100;
}
uint32_t lifetime_boots() {
	return 1;
}
time64_t realtimeclock() {
	return localtime_sec() + 1000000;
}

// Mocking radio
comms_msg_t mmsg1;
comms_msg_t* mcopy1 = &mmsg1;
comms_msg_t* _msg1 = NULL;
comms_send_done_f* _sdf1 = NULL;
void* _user1 = NULL;

comms_error_t fake_comms_send1(comms_layer_iface_t* comms, comms_msg_t* msg, comms_send_done_f* sdf, void* user) {
	comms_layer_t* c = (comms_layer_t*)comms;
	uint8_t length = comms_get_payload_length(c, msg);
	uint8_t* payload = (uint8_t*)comms_get_payload(c, msg, length);
	debugb1("send1", payload, length);
	packets_sent++;

	if(fake_localtime == 1) {
		uint8_t ref[] =
		"\x00\x02" // hdr-version
		"\x88\x77\x66\x55\x44\x33\x22\x11" // guid
		"\x00\x00\x00\x01" // boot_number
		"\x00\x00\x00\x00\x00\x0f\x42\x40" // boot_time
		"\x00\x00\x00\x01" // uptime
		"\x00\x00\x00\x65" // lifetime
		"\x00\x00\x00\x00" // announcement
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // UUID
		"\x55" // position type
		"\x00\x00\x00\x00" // lat
		"\x00\x00\x00\x00" // lon
		"\x00\x00\x00\x00" // elevation
		"\x01\x00" // radio tech+channel
		"\x01\x02\x03\x04\x05\x06\x07\x08" // IDENT_TIMESTAMP
		"\x00\x00\x00\x00"; // feature hash
		if(memcmp(ref, payload, length) != 0) {
			err1("payload mismatch");
			test_errors++;
		}
	}

	if(fake_localtime == 59) {
		uint8_t ref[] =
		"\x00\x02" // hdr-version
		"\x88\x77\x66\x55\x44\x33\x22\x11" // guid
		"\x00\x00\x00\x01" // boot_number
		"\x00\x00\x00\x00\x00\x0F\x42\x40" // boot_time
		"\x00\x00\x00\x3B" // uptime
		"\x00\x00\x00\x9F" // lifetime
		"\x00\x00\x00\x09" // announcement
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // UUID
		"\x55" // position type
		"\x00\x00\x00\x00" // lat
		"\x00\x00\x00\x00" // lon
		"\x00\x00\x00\x00" // elevation
		"\x01\x00" // radio tech+channel
		"\x01\x02\x03\x04\x05\x06\x07\x08" // IDENT_TIMESTAMP
		"\x00\x00\x00\x00"; // feature hash
		if(memcmp(ref, payload, length) != 0) {
			err1("payload mismatch");
			test_errors++;
		}
	}

	if(_sdf1 == NULL) {
		_msg1 = msg;
		_sdf1 = sdf;
		_user1 = user;
		return COMMS_SUCCESS;
	}
	return COMMS_EBUSY;
}

#define PACKETS_EXPECTED 10

int main() {
	debug1("test start");

	uint8_t r1[512];
	comms_layer_t* radio1 = (comms_layer_t*)r1;
	comms_error_t err = comms_am_create(radio1, 1, &fake_comms_send1);
	printf("create radio1=%d\n", err);

	sigAreaInit("fakesignature.bin");
	sigInit();

	device_announcer_t announcer;
	deva_init();
	deva_add_announcer(&announcer, radio1, 10);

	for(uint8_t i=0;i<60;i++) {
		deva_poll();
		fake_localtime++;
		if(_sdf1 != NULL) {
			_sdf1(radio1, _msg1, COMMS_SUCCESS, _user1);
			_sdf1 = NULL;
		}
	}

	debug1("test end");

	if(test_errors > 0) {
		err1("ERRORS: %"PRIu32, test_errors);
		return 1;
	}

	if(packets_sent != PACKETS_EXPECTED) {
		err1("FAIL %d != %d", packets_sent, PACKETS_EXPECTED);
		return 1;
	}

	info1("SUCCESS?");
	return 0;
}
