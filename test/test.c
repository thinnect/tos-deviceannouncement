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
#include "device_features.h"

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

//------------------------------------------------------------------------------
comms_error_t fake_comms_send1(comms_layer_iface_t* comms, comms_msg_t* msg, comms_send_done_f* sdf, void* user) {
	comms_layer_t* c = (comms_layer_t*)comms;
	uint8_t length = comms_get_payload_length(c, msg);
	uint8_t* payload = (uint8_t*)comms_get_payload(c, msg, length);
	debugb1("send1", payload, length);
	packets_sent++;

	if(fake_localtime == 1) {
		uint8_t ref[] =
		"\x00\x02" // hdr-version
		"\x88\x77\x66\x55\x44\x33\x22\x11" // EUI64
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
		"\x00\x00\x00\x00" // feature hash
		;
		if(memcmp(ref, payload, length) != 0) {
			err1("payload mismatch");
			test_errors++;
		}
	}

	if(fake_localtime == 59) {
		uint8_t ref[] =
		"\x00\x02" // hdr-version
		"\x88\x77\x66\x55\x44\x33\x22\x11" // EUI64
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
		"\x00\x00\x00\x00" // feature hash
		;
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

int testPeriodicAnnouncements() {
	// Test setup
	fake_localtime = 0;
	packets_sent = 0;
	test_errors = 0;
	//-----------

	uint8_t r1[512];
	comms_layer_t* radio = (comms_layer_t*)r1;
	comms_error_t err = comms_am_create(radio, 1, &fake_comms_send1);
	printf("create radio=%d\n", err);

	sigAreaInit("fakesignature.bin");
	sigInit();

	device_announcer_t announcer;
	deva_init();
	deva_add_announcer(&announcer, radio, 10);

	for(uint8_t i=0;i<60;i++) {
		deva_poll();
		fake_localtime++;
		if(_sdf1 != NULL) {
			_sdf1(radio, _msg1, COMMS_SUCCESS, _user1);
			_sdf1 = NULL;
		}
	}

	if(test_errors > 0) {
		err1("testPeriodicAnnouncements - errors: %"PRIu32, test_errors);
		return 1;
	}
	if(packets_sent != PACKETS_EXPECTED) {
		err1("testPeriodicAnnouncements - packet count: %d != %d", packets_sent, PACKETS_EXPECTED);
		return 1;
	}
	return 0;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
comms_error_t fake_comms_send2(comms_layer_iface_t* comms, comms_msg_t* msg, comms_send_done_f* sdf, void* user) {
	comms_layer_t* c = (comms_layer_t*)comms;
	uint8_t length = comms_get_payload_length(c, msg);
	uint8_t* payload = (uint8_t*)comms_get_payload(c, msg, length);
	debugb1("send2", payload, length);
	packets_sent++;

	uint8_t ref[] =
	"\x01\x02" // hdr-version
	"\x88\x77\x66\x55\x44\x33\x22\x11" // EUI64
	"\x00\x00\x00\x01" // boot_number
	"\xec\xee\x9d\xf7\x73\x6b\x4c\x10\xaf\xed\xde\x06\x05\xe5\x53\xf3" // Platform UUID
	"\x08\x09\x0a" // HW version
	"\x22\xb9\x39\x35\xd6\x30\x47\xe2\xb5\xfc\x30\xb3\xc7\x2b\xae\x5a" // Manufacturer UUID
	"\x00\x00\x00\x00\x5d\x35\xd0\x2f" // production timestamp
	"\x01\x02\x03\x04\x05\x06\x07\x08" // IDENT_TIMESTAMP
	"\x01\x02\x03" // firmware version
	;

	if(memcmp(ref, payload, length) != 0) {
		err1("payload mismatch");
		test_errors++;
	}

	if(_sdf1 == NULL) {
		_msg1 = msg;
		_sdf1 = sdf;
		_user1 = user;
		return COMMS_SUCCESS;
	}
	return COMMS_EBUSY;
}

int testDescriptionResponse() {
	// Test setup
	fake_localtime = 0;
	packets_sent = 0;
	test_errors = 0;
	//-----------

	uint8_t r1[512];
	comms_layer_t* radio = (comms_layer_t*)r1;
	comms_error_t err = comms_am_create(radio, 1, &fake_comms_send2);
	printf("create radio=%d\n", err);

	sigAreaInit("fakesignature.bin");
	sigInit();

	device_announcer_t announcer;
	deva_init();
	deva_add_announcer(&announcer, radio, 0);

	for(uint8_t i=0;i<60;i++) {
		deva_poll();
		fake_localtime++;
		if(_sdf1 != NULL) {
			_sdf1(radio, _msg1, COMMS_SUCCESS, _user1);
			_sdf1 = NULL;
		}
		if(i == 30) {
			comms_msg_t msg;
			comms_init_message(radio, &msg);
			comms_set_packet_type(radio, &msg, 0xDA);
			memcpy(comms_get_payload(radio, &msg, 2), "\x11\x02", 2);
			comms_set_payload_length(radio, &msg, 2);
			comms_am_set_destination(radio, &msg, 0xFFFF);
			comms_am_set_source(radio, &msg, 0x1234);
			comms_deliver(radio, &msg);
		}
	}

	if(packets_sent != 1) {
		err1("testDescriptionResponse - packet count: %d != %d", packets_sent, 1);
		return 1;
	}
	if(test_errors > 0) {
		err1("testDescriptionResponse - errors: %"PRIu32, test_errors);
		return 1;
	}

	return 0;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
comms_error_t fake_comms_send3(comms_layer_iface_t* comms, comms_msg_t* msg, comms_send_done_f* sdf, void* user) {
	comms_layer_t* c = (comms_layer_t*)comms;
	uint8_t length = comms_get_payload_length(c, msg);
	uint8_t* payload = (uint8_t*)comms_get_payload(c, msg, length);
	debugb1("send3", payload, length);
	packets_sent++;

	if(packets_sent == 1) {
		uint8_t ref[] =
		"\x00\x02" // hdr-version
		"\x88\x77\x66\x55\x44\x33\x22\x11" // EUI64
		"\x00\x00\x00\x01" // boot_number
		"\x00\x00\x00\x00\x00\x0f\x42\x40" // boot_time
		"\x00\x00\x00\x06" // uptime
		"\x00\x00\x00\x6a" // lifetime
		"\x00\x00\x00\x00" // announcement
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00" // UUID
		"\x55" // position type
		"\x00\x00\x00\x00" // lat
		"\x00\x00\x00\x00" // lon
		"\x00\x00\x00\x00" // elevation
		"\x01\x00" // radio tech+channel
		"\x01\x02\x03\x04\x05\x06\x07\x08" // IDENT_TIMESTAMP
		"\x00\x00\x06\xd8" // feature hash
		;
		if(memcmp(ref, payload, length) != 0) {
			err1("payload mismatch");
			test_errors++;
		}
	}

	if(packets_sent == 2) {
		uint8_t ref[] =
		"\x02\x02" // hdr-version
		"\x88\x77\x66\x55\x44\x33\x22\x11" // EUI64
		"\x00\x00\x00\x01" // boot_number
		"\x03" // Total
		"\x00" // Offset
		"\x01\x02\x03\x04\x05\x06\x07\x08\x09\x10\x11\x12\x13\x14\x15\x16" // Feature 1
		"\x17\x18\x19\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x30\x31\x32" // Feature 2
		"\x33\x34\x35\x36\x37\x38\x39\x40\x41\x42\x43\x44\x45\x46\x47\x48" // Feature 3
		;
		if(memcmp(ref, payload, length) != 0) {
			err1("payload mismatch");
			test_errors++;
		}
	}

	if(packets_sent == 3) {
		uint8_t ref[] =
		"\x02\x02" // hdr-version
		"\x88\x77\x66\x55\x44\x33\x22\x11" // EUI64
		"\x00\x00\x00\x01" // boot_number
		"\x03" // Total
		"\x01" // Offset
		"\x17\x18\x19\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x30\x31\x32" // Feature 2
		"\x33\x34\x35\x36\x37\x38\x39\x40\x41\x42\x43\x44\x45\x46\x47\x48" // Feature 3
		;
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

int testListFeaturesResponse() {
	// Test setup
	fake_localtime = 0;
	packets_sent = 0;
	test_errors = 0;
	//-----------

	uint8_t r1[512];
	comms_layer_t* radio = (comms_layer_t*)r1;
	comms_error_t err = comms_am_create(radio, 1, &fake_comms_send3);
	printf("create radio=%d\n", err);

	sigAreaInit("fakesignature.bin");
	sigInit();

	devf_init();
	device_feature_t dftrs[3];
	devf_add_feature(&dftrs[0], (nx_uuid_t*)"\x01\x02\x03\x04\x05\x06\x07\x08\x09\x10\x11\x12\x13\x14\x15\x16");
	devf_add_feature(&dftrs[1], (nx_uuid_t*)"\x17\x18\x19\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x30\x31\x32");
	devf_add_feature(&dftrs[2], (nx_uuid_t*)"\x33\x34\x35\x36\x37\x38\x39\x40\x41\x42\x43\x44\x45\x46\x47\x48");

	device_announcer_t announcer;
	deva_init();
	deva_add_announcer(&announcer, radio, 0);

	for(uint8_t i=0;i<30;i++) {
		deva_poll();
		fake_localtime++;
		if(_sdf1 != NULL) {
			_sdf1(radio, _msg1, COMMS_SUCCESS, _user1);
			_sdf1 = NULL;
		}
		if(i == 5) {
			comms_msg_t msg;
			comms_init_message(radio, &msg);
			comms_set_packet_type(radio, &msg, 0xDA);
			memcpy(comms_get_payload(radio, &msg, 2), "\x10\x02", 2);
			comms_set_payload_length(radio, &msg, 2);
			comms_am_set_destination(radio, &msg, 0xFFFF);
			comms_am_set_source(radio, &msg, 0x1234);
			comms_deliver(radio, &msg);
		}
		if(i == 10) {
			comms_msg_t msg;
			comms_init_message(radio, &msg);
			comms_set_packet_type(radio, &msg, 0xDA);
			memcpy(comms_get_payload(radio, &msg, 3), "\x12\x02\x00", 3);
			comms_set_payload_length(radio, &msg, 3);
			comms_am_set_destination(radio, &msg, 0xFFFF);
			comms_am_set_source(radio, &msg, 0x1234);
			comms_deliver(radio, &msg);
		}
		if(i == 20) {
			comms_msg_t msg;
			comms_init_message(radio, &msg);
			comms_set_packet_type(radio, &msg, 0xDA);
			memcpy(comms_get_payload(radio, &msg, 3), "\x12\x02\x01", 3);
			comms_set_payload_length(radio, &msg, 3);
			comms_am_set_destination(radio, &msg, 0xFFFF);
			comms_am_set_source(radio, &msg, 0x1234);
			comms_deliver(radio, &msg);
		}
	}

	if(packets_sent != 3) {
		err1("testDescriptionResponse - packet count: %d != %d", packets_sent, 3);
		return 1;
	}
	if(test_errors > 0) {
		err1("testDescriptionResponse - errors: %"PRIu32, test_errors);
		return 1;
	}

	return 0;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
int testFeatureManagement() {
	devf_init();
	device_feature_t dftrs[4];

	devf_add_feature(&dftrs[0], (nx_uuid_t*)"\x01\x02\x03\x04\x05\x06\x07\x08\x09\x10\x11\x12\x13\x14\x15\x16");
	if(devf_count() != 1) {
		return 1;
	}
	if(devf_hash() != 0xb2) {
		return 2;
	}

	devf_add_feature(&dftrs[1], (nx_uuid_t*)"\x17\x18\x19\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x30\x31\x32");
	if(devf_count() != 2) {
		return 1;
	}
	if(devf_hash() != 0x2fa) {
		return 2;
	}

	devf_add_feature(&dftrs[2], (nx_uuid_t*)"\x33\x34\x35\x36\x37\x38\x39\x40\x41\x42\x43\x44\x45\x46\x47\x48");
	if(devf_count() != 3) {
		return 1;
	}
	if(devf_hash() != 0x6d8) {
		return 2;
	}

	devf_add_feature(&dftrs[3], (nx_uuid_t*)"\x49\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x60\x61\x62\x63\x64");
	if(devf_count() != 4) {
		return 1;
	}
	if(devf_hash() != 0xc58) {
		return 2;
	}

	devf_remove_feature(&dftrs[1]); // Remove second element
	if(devf_count() != 3) {
		return 1;
	}

	devf_remove_feature(&dftrs[0]); // Remove first element
	if(devf_count() != 2) {
		return 1;
	}

	devf_remove_feature(&dftrs[3]); // Remove last element
	if(devf_count() != 1) {
		return 1;
	}

	devf_remove_feature(&dftrs[2]); // Remove the only remaining element
	if(devf_count() != 0) {
		return 1;
	}

	return 0;
}
//------------------------------------------------------------------------------

int main() {
	int results = 0;
	debug1("tests start");

	results += testPeriodicAnnouncements();
	results += testDescriptionResponse();
	results += testListFeaturesResponse();
	results += testFeatureManagement();

	if(results != 0) {
		err1("%d failures", results);
		return 1;
	}
	info1("SUCCESS?");
	return 0;
}
