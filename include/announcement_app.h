/**
 * Device announcement setup.
 *
 * Copyright Thinnect Inc. 2019
 * @license MIT
 * @author Raido Pahtma
 */
#ifndef ANNOUNCEMENT_APP_H_
#define ANNOUNCEMENT_APP_H_

#include <stdint.h>

/*
* Initialize announcement module for the specified radio and default announcement period.
*
* @param radio The comms layer to do announcements with.
* @param manage_radio The app should also give radio sleep / wakeup suggestions.
* @param period_s Standard announcement period in seconds.
* @return 0 on success.
*/
int announcement_app_init(comms_layer_t * radio, bool manage_radio, uint16_t period_s);

#endif//ANNOUNCEMENT_APP_H_
