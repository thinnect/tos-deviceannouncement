/**
 * Device feature storage in a linked-list with user-provided list elements.
 *
 * Copyright Thinnect 2019.
 * @author Raido Pahtma
 * @license MIT
 */

#include "device_features.h"

#include "loglevels.h"
#define __MODUUL__ "DevF"
#define __LOG_LEVEL__ ( LOG_LEVEL_devicefeatures & BASE_LOG_LEVEL )
#include "log.h"

static device_feature_t* m_features;
static uint8_t m_count;

void devf_init() {
	m_features = NULL;
	m_count = 0;
}

uint8_t devf_count() {
	return m_count;
}

uint32_t devf_hash() { // Really simple for now, just a sum of bytes
	uint32_t hash = 0;
	device_feature_t* f = m_features;
	while(f != NULL) {
		for(uint8_t i=0;i<sizeof(nx_uuid_t);i++) {
			hash += ((uint8_t*)&(f->uuid))[i];
		}
		f = f->next;
	}
	return hash;
}

bool devf_get_feature(uint8_t fnum, nx_uuid_t* ftr) {
	device_feature_t* f = m_features;
	for(uint8_t i=0; i<fnum; i++) {
		if(f == NULL) {
			return false;
		}
		f = f->next;
	}
	if(f != NULL) {
		memcpy(ftr, &(f->uuid), sizeof(nx_uuid_t));
		return true;
	}
	return false;
}

bool devf_add_feature(device_feature_t* ftr, nx_uuid_t* feature) {
	device_feature_t* f = m_features;
	while(f != NULL) {
		// Features must not be added multiple times
		debug1("%d %p %p %p %p %d", (int)m_count, ftr, feature, f, &(f->uuid), sizeof(nx_uuid_t));
		if((f == ftr)||(memcmp(&(f->uuid), feature, sizeof(nx_uuid_t)) == 0)) {
			warnb1("dup %p %p", feature, sizeof(nx_uuid_t), ftr, f);
			return false;
		}
		if(f->next == NULL) {
			break;
		}
		else {
			f = ftr->next;
		}
	}

	if(f == NULL) {
		m_features = ftr;
	}
	else {
		f->next = ftr; // Append to the end
	}

	memcpy(&(ftr->uuid), feature, sizeof(nx_uuid_t));
	ftr->next = NULL;
	m_count++;
	return true;
}

bool devf_remove_feature(device_feature_t* ftr) {
	if(m_features != NULL) {
		if(m_features == ftr) {
			m_features = m_features->next;
			m_count--;
			return true;
		}
		else {
			device_feature_t* f = m_features;
			while(f->next != NULL) {
				if(f->next == ftr) {
					f->next = f->next->next;
					m_count--;
					return true;
				}
				f = ftr->next;
			}
		}
	}
	return false;
}
