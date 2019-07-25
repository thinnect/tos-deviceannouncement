/**
 * Device announcement module API.
 *
 * Copyright Thinnect Inc. 2019
 * @author Raido Pahtma
 * @license MIT
 */
#ifndef DEVICE_ANNOUNCEMENT_H_
#define DEVICE_ANNOUNCEMENT_H_

#include <stdbool.h>

#include "mist_comm.h"

typedef struct device_announcer device_announcer_t;

/**
 * Initialize the device announcement module. Call it once after boot.
 */
void deva_init();

/**
 * Device announcement module poll. Call it occasionally to have spontaneous
 * announcement sent out.
 * @return true if an announcement was sent.
 */
bool deva_poll();

/**
 * Add an announcer for the specified comms layer and with the specified period.
 * @param announcer Memory for an announcer, make sure it does not go out of scope!
 * @param comms A comms layer.
 * @param period_s Announcement period, set to 0 for no announcements.
 * @return true if an announcer was added.
 */
bool deva_add_announcer(device_announcer_t* announcer, comms_layer_t* comms, uint32_t period_s);

/**
 * Remove an announcer.
 * @param announcer A previously registered announcer.
 * @return true if an announcer was removed.
 */
bool deva_remove_announcer(device_announcer_t* announcer);

/**
 * Add a local listener for announcements made by other devices.
 * TODO
 */
//bool deva_add_listener(device_announcement_listener_t* listener, callback);

/**
 * Remove a previously registered local listener.
 * TODO
 */
//bool deva_remove_listener(device_announcement_listener_t* listener);

/**
 * You should not access this struct directly from the outside!
 */
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
