/**
 * IEEE EUI-64 definitions header.
 *
 * Copyright Thinnect Inc. 2020
 * @license MIT
 */
#ifndef EUI64_H_
#define EUI64_H_

#include <stdbool.h>
#include <stdint.h>

#define EUI64_LENGTH 8
#define IEEE_EUI64_LENGTH 8

typedef struct ieee_eui64 {
	uint8_t data[IEEE_EUI64_LENGTH];
} ieee_eui64_t;

#endif// EUI64_H_
