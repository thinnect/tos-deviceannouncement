/**
 * Device announcement setup.
 *
 * Copyright Thinnect Inc. 2019
 * @license MIT
 * @author Raido Pahtma
 */

#include <stdint.h>

/*
* Initialize announcement module for the specified radio and default announcement period.
*
* @param radio The comms layer to do announcements with.
* @param period_s Standard announcement period in seconds.
* @return 0 on success.
*/
int announcement_app_init(comms_layer_t * radio, uint16_t period_s);
