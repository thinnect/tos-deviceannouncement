#ifndef DEVICE_ANNOUNCEMENT_H_
#define DEVICE_ANNOUNCEMENT_H_

#include <stdbool.h>

#include "mist_comm.h"

typedef struct device_announcer device_announcer_t;

void deva_init();
bool deva_poll();

bool deva_add_announcer(device_announcer_t* announcer, comms_layer_t* comms, uint32_t period_s);
bool deva_remove_announcer(device_announcer_t* announcer);

//bool deva_add_listener(device_announcement_listener_t* listener, callback);
//bool deva_remove_listener(device_announcement_listener_t* listener);

struct device_announcer {
	comms_layer_t* comms;
	comms_receiver_t rcvr;
	uint16_t period; // minutes, 0 for never
	bool busy;
	uint32_t last;
	uint32_t announcements;
	device_announcer_t* next;
};

#endif//DEVICE_ANNOUNCEMENT_H_
