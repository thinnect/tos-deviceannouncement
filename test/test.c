
#include "loglevels.h"
#define __MODUUL__ "test"
#define __LOG_LEVEL__ ( LOG_LEVEL_test & BASE_LOG_LEVEL )
#include "log.h"

#include "mist_comm.h"
#include "mist_comm_am.h"
#include "device_announcement.h"

#include "nesc_to_c_compat.h"

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

uint8_t packets_sent = 0;

comms_msg_t mmsg1;
comms_msg_t* mcopy1 = &mmsg1;
comms_msg_t* _msg1 = NULL;
comms_send_done_f* _sdf1 = NULL;
void* _user1 = NULL;

comms_error_t fake_comms_send1(comms_layer_iface_t* comms, comms_msg_t* msg, comms_send_done_f* sdf, void* user) {
	debug1("send1");
	packets_sent++;
	if(_msg1 == NULL) {
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

	if(packets_sent == PACKETS_EXPECTED) {
		info1("SUCCESS?");
		return 0;
	}

	err1("FAIL %d != %d", packets_sent, PACKETS_EXPECTED);
	return 1;
}
