/**
 * Device feature storage in a linked-list with user-provided list elements.
 *
 * Copyright Thinnect Inc. 2019
 * @author Raido Pahtma
 * @license MIT
 */
#ifndef DEVICE_FEATURES_H_
#define DEVICE_FEATURES_H_

#include <stdbool.h>
#include <stdint.h>

#include "UniversallyUniqueIdentifier.h"

typedef struct device_feature device_feature_t;

/**
 * Initialize the device features module. Call it once after boot.
 */
void devf_init();

/**
 * Get total feature count.
 * @return Feature count.
 */
uint8_t devf_count();

/**
 * Calculate a hash of device features.
 * @return Hash of device features.
 */
uint32_t devf_hash();

/**
 * Get device feature based on sequence number.
 *
 * Changes during iteration are not handled.
 *
 * @param fnum Feature sequence number 0...255.
 * @param ftr  UUID struct to store requested feature UUID.
 * @return true if a feature was returned.
 */
bool devf_get_feature(uint8_t fnum, nx_uuid_t* ftr);

/**
 * Add a device feature.
 *
 * Changes during iteration are not handled.
 *
 * @param ftr Memory for feature storage.
 * @param feature Feature UUID to store.
 * @return true if a feature was stored, false if duplicate.
 */
bool devf_add_feature(device_feature_t* ftr, nx_uuid_t* feature);

/**
 * Remove feature based on storage pointer.
 *
 * @param ftr Feature memory pointer previously added with add_feature.
 * @return true if a feature was removed and the memory is free.
 */
bool devf_remove_feature(device_feature_t* ftr);

/*----------------------------------------------------------------------------*/
/**
 * DeviceFeatures linked-list storage element.
 * Module will initialize the structures on add, user should not touch them.
*/
struct device_feature {
	nx_uuid_t uuid;
	device_feature_t* next;
};

#endif//DEVICE_FEATURES_H_
