/**
 * Device announcement protocol implementation.
 *
 * Copyright Thinnect Inc. 2019
 * @author Raido Pahtma
 * @license MIT
 **/
#include "DeviceAnnouncementProtocol.h"
#include "device_announcement.h"
#include "device_features.h"
#include "DeviceSignature.h"

#include <time.h>
#include <string.h>
#include <inttypes.h>

#include "node_lifetime.h"

#include "endianness.h"

#include "cmsis_os2_ext.h"

#include "mist_comm.h"
#include "mist_comm_am.h"

#include "loglevels.h"
#define __MODUUL__ "DevA"
#define __LOG_LEVEL__ ( LOG_LEVEL_deviceannouncement & BASE_LOG_LEVEL )
#include "log.h"

// How often the announcement module is polled
#define DEVICE_ANNOUNCEMENT_POLL_PERIOD_S 60

extern uint8_t radio_channel(); // TODO header

static bool unguarded_deva_poll();

static void radio_status_changed (comms_layer_t* comms, comms_status_t status, void* user);
static void radio_send_done (comms_layer_t *comms, comms_msg_t *msg, comms_error_t result, void *user);
static void radio_recv (comms_layer_t* comms, const comms_msg_t *msg, void *user);

static device_announcer_t* m_announcers;
static time_t m_boot_time;
static bool m_busy;
static uint8_t m_announcements; // TODO should be announcer specific

static comms_msg_t m_msg_pool;
static comms_msg_t* m_msg_pool_msg = &m_msg_pool;

static osMutexId_t m_mutex;

static void nx_uuid_application(nx_uuid_t* uuid) {
	memcpy(uuid, UUID_APPLICATION_BYTES, 16);
}

static comms_msg_t* messagepool_get() {
	comms_msg_t* msg = m_msg_pool_msg;
	m_msg_pool_msg = NULL;
	return msg;
}
static bool messagepool_put(comms_msg_t* msg) {
	if(m_msg_pool_msg != NULL) {
		return false;
	}
	m_msg_pool_msg = msg;
	return true;
}

static uint32_t next_announcement(uint32_t announcements, uint16_t period) {
	if(announcements == 0) {
		return period / 10;
	}
	else if(announcements < 5) {
		return period / 5;
	}
	return period;
}

static void update_boot_time() {
	if(m_boot_time == ((time_t)-1)) { // Adjust boot time only when it is not known
		time_t now = time(NULL);
		if(((time_t)-1) != now) {
			m_boot_time = now - osCounterGetSecond();
		}
	}
}

static void announcement_loop(void * arg) {
	for(;;) {
		osDelay(DEVICE_ANNOUNCEMENT_POLL_PERIOD_S*1000UL);

		while(osOK != osMutexAcquire(m_mutex, osWaitForever));
		if (unguarded_deva_poll()) {
			debug1("snt");
		}
		osMutexRelease(m_mutex);
	}
}

bool deva_init() {
	m_announcers = NULL;
	m_boot_time = ((time_t)-1);
	m_busy = false;
	m_announcements = 0;
	update_boot_time();

	m_mutex = osMutexNew(NULL);

    const osThreadAttr_t annc_thread_attr = { .name = "annc", .stack_size = 1536 };
    osThreadId_t thrd = osThreadNew(announcement_loop, NULL, &annc_thread_attr);
    if (NULL == thrd)
    {
    	return false;
    }
    return true;
}

bool deva_add_announcer(device_announcer_t* announcer, comms_layer_t* comms, comms_sleep_controller_t* rctrl, uint32_t period_s) {

	while(osOK != osMutexAcquire(m_mutex, osWaitForever));

	device_announcer_t* an = m_announcers;
	announcer->comms = comms;
	announcer->comms_ctrl = rctrl;
	announcer->period = period_s;
	announcer->busy = false;
	announcer->last = osCounterGetSecond(); // TODO introduce initial random here
	announcer->announcements = 0;
	announcer->next = NULL;

	comms_register_recv(comms, &(announcer->rcvr), radio_recv, announcer, AMID_DEVICE_ANNOUNCEMENT);

	if(NULL != rctrl) {
		if(COMMS_SUCCESS != comms_register_sleep_controller(comms, rctrl, radio_status_changed, NULL)) {
			err1("rctrl");
		}
	}

	if(an == NULL) {
		m_announcers = announcer;
	}
	else {
		while(an->next != NULL) {
			an = an->next;
		}
		an->next = announcer;
	}

	osMutexRelease(m_mutex);

	return true;
}

bool deva_remove_announcer(device_announcer_t* announcer) {
	// TODO implement removal
	return false;
}

static bool announce(device_announcer_t* an, uint8_t version, am_addr_t destination) {
	if(!m_busy) {
		comms_msg_t* msg = messagepool_get();
		if(msg != NULL) {
			uint8_t length = 0;
			comms_init_message(an->comms, msg);
			if(version == 1) {
				device_announcement_v1_t* anc = (device_announcement_v1_t*)comms_get_payload(an->comms, msg, sizeof(device_announcement_v1_t));
				if(anc != NULL) {
					//coordinates_geo_t geo;

					anc->header = DEVA_ANNOUNCEMENT;
					anc->version = DEVICE_ANNOUNCEMENT_VERSION;
					sigGetEui64((uint8_t*)anc->guid);
					anc->boot_number = hton32(node_lifetime_boots());

					anc->boot_time = hton64(m_boot_time);
					anc->uptime = hton32(osCounterGetSecond());
					anc->lifetime = hton32(node_lifetime_seconds());
					anc->announcement = hton32(m_announcements);

					nx_uuid_application(&(anc->uuid));

					//call GetGeo.get(&geo);
					//anc->latitude = geo.latitude;
					//anc->longitude = geo.longitude;
					//anc->elevation = geo.elevation;
					anc->latitude = hton32(0);
					anc->longitude = hton32(0);
					anc->elevation = hton32(0);

					anc->ident_timestamp = hton64(IDENT_TIMESTAMP);
					anc->feature_list_hash = hton32(devf_hash());

					length = sizeof(device_announcement_v1_t);
				}
			}
			else {
				device_announcement_v2_t* anc = (device_announcement_v2_t*)comms_get_payload(an->comms, msg, sizeof(device_announcement_v2_t));
				if(anc != NULL) {
					//coordinates_geo_t geo;

					anc->header = DEVA_ANNOUNCEMENT;
					anc->version = DEVICE_ANNOUNCEMENT_VERSION;
					sigGetEui64((uint8_t*)anc->guid);
					anc->boot_number = hton32(node_lifetime_boots());

					anc->boot_time = hton64(m_boot_time);
					anc->uptime = hton32(osCounterGetSecond());
					anc->lifetime = hton32(node_lifetime_seconds());
					anc->announcement = hton32(m_announcements);

					nx_uuid_application(&(anc->uuid));

					//call GetGeo.get(&geo);
					//anc->position_type = geo.type;
					//anc->latitude = geo.latitude;
					//anc->longitude = geo.longitude;
					//anc->elevation = geo.elevation;
					anc->position_type = 'U';
					anc->latitude = hton32(0);
					anc->longitude = hton32(0);
					anc->elevation = hton32(0);

					anc->radio_tech = 1; // Always 802.15.4 ... for now
					anc->radio_channel = radio_channel(); // call RadioChannel.getChannel();

					anc->ident_timestamp = hton64(IDENT_TIMESTAMP);
					anc->feature_list_hash = hton32(devf_hash());

					length = sizeof(device_announcement_v2_t);
				}
			}

			if(length > 0) {
				comms_error_t err;
				comms_set_packet_type(an->comms, msg, AMID_DEVICE_ANNOUNCEMENT);
				comms_am_set_destination(an->comms, msg, destination);
				comms_set_payload_length(an->comms, msg, length);

				if(NULL != an->comms_ctrl) {
					comms_sleep_block(an->comms_ctrl);
					while (COMMS_STARTED != comms_status(an->comms));
				}

				err = comms_send(an->comms, msg, radio_send_done, an);

				logger(err == COMMS_SUCCESS ? LOG_DEBUG1: LOG_WARN1, "snd=%u", err);
				if(err == COMMS_SUCCESS) {
					m_busy = true;
					return true;
				}
			}
			messagepool_put(msg);
		}
	}
	else warn1("bsy");
	return false;
}

static bool describe(device_announcer_t* an, uint8_t version, am_addr_t destination) {
	if(!m_busy) {
		comms_msg_t* msg = messagepool_get();
		if(msg != NULL) {
			uint8_t length = 0;
			comms_init_message(an->comms, msg);
			if(version == 1) {
				device_description_v1_t* anc = (device_description_v1_t*)comms_get_payload(an->comms, msg, sizeof(device_description_v1_t));
				if(anc != NULL) {
					anc->header = DEVA_DESCRIPTION;
					anc->version = DEVICE_ANNOUNCEMENT_VERSION;
					sigGetEui64((uint8_t*)anc->guid);
					anc->boot_number = hton32(node_lifetime_boots());

					sigGetPlatformUUID((uint8_t*)&(anc->platform));

					sigGetBoardManufacturerUUID((uint8_t*)&(anc->manufacturer));
					anc->production = hton64(sigGetPlatformProductionTime());

					anc->ident_timestamp = hton64(IDENT_TIMESTAMP);
					anc->sw_major_version = SW_MAJOR_VERSION;
					anc->sw_minor_version = SW_MINOR_VERSION;
					anc->sw_patch_version = SW_PATCH_VERSION;

					length = sizeof(device_description_v1_t);
				}
			}
			else {
				device_description_v2_t* anc = (device_description_v2_t*)comms_get_payload(an->comms, msg, sizeof(device_description_v2_t));
				if(anc != NULL) {
					semver_t hwv = sigGetPlatformVersion();

					anc->header = DEVA_DESCRIPTION;
					anc->version = DEVICE_ANNOUNCEMENT_VERSION;
					sigGetEui64((uint8_t*)anc->guid);
					anc->boot_number = hton32(node_lifetime_boots());

					sigGetPlatformUUID((uint8_t*)&(anc->platform));

					anc->hw_major_version = hwv.major;
					anc->hw_minor_version = hwv.minor;
					anc->hw_assem_version = hwv.patch;

					sigGetBoardManufacturerUUID((uint8_t*)&(anc->manufacturer));
					anc->production = hton64(sigGetPlatformProductionTime());

					anc->ident_timestamp = hton64(IDENT_TIMESTAMP);
					anc->sw_major_version = SW_MAJOR_VERSION;
					anc->sw_minor_version = SW_MINOR_VERSION;
					anc->sw_patch_version = SW_PATCH_VERSION;

					length = sizeof(device_description_v2_t);
				}
			}

			if(length > 0) {
				comms_error_t err;
				comms_set_packet_type(an->comms, msg, AMID_DEVICE_ANNOUNCEMENT);
				comms_am_set_destination(an->comms, msg, destination);
				comms_set_payload_length(an->comms, msg, length);

				if(NULL != an->comms_ctrl) {
					comms_sleep_block(an->comms_ctrl);
					while (COMMS_STARTED != comms_status(an->comms));
				}

				err = comms_send(an->comms, msg, radio_send_done, an);

				logger(err == COMMS_SUCCESS ? LOG_DEBUG1: LOG_WARN1, "snd=%u", err);
				if(err == COMMS_SUCCESS) {
					m_busy = true;
					return true;
				}
			}
			messagepool_put(msg);
		}
	}
	else warn1("bsy");
	return false;
}

static bool list_features(device_announcer_t* an, am_addr_t destination, uint8_t offset) {
	if(!m_busy) {
		comms_msg_t* msg = messagepool_get();
		if(msg != NULL) {
			uint8_t space = (comms_get_payload_max_length(an->comms) - sizeof(device_features_t)) / sizeof(uuid_t);
			device_features_t* anc = (device_features_t*)comms_get_payload(an->comms, msg, sizeof(device_features_t) + space*sizeof(uuid_t));
			comms_init_message(an->comms, msg);
			if(anc != NULL) {
				uint8_t total_features = devf_count();
				comms_error_t err;
				uint8_t ftrs = 0;
				uint8_t skip = 0;

				anc->header = DEVA_FEATURES;
				anc->version = DEVICE_ANNOUNCEMENT_VERSION;
				sigGetEui64((uint8_t*)anc->guid);
				anc->boot_number = hton32(node_lifetime_boots());

				anc->total = total_features;
				anc->offset = offset;

				for(;(ftrs<space)&&(offset+skip+ftrs<total_features);) {
					if(devf_get_feature(offset+ftrs, &(anc->features[ftrs]))) {
						ftrs++;
					}
					else { // Feature disabled ... or problematic?
						skip++;
					}
				}

				debugb1("ftrs %u total %u", anc, sizeof(device_features_t)+ftrs*sizeof(nx_uuid_t), ftrs, anc->total);

				comms_set_packet_type(an->comms, msg, AMID_DEVICE_ANNOUNCEMENT);
				comms_am_set_destination(an->comms, msg, destination);
				comms_set_payload_length(an->comms, msg, sizeof(device_features_t) + ftrs*sizeof(uuid_t));

				if(NULL != an->comms_ctrl) {
					comms_sleep_block(an->comms_ctrl);
					while (COMMS_STARTED != comms_status(an->comms));
				}

				err = comms_send(an->comms, msg, radio_send_done, an);

				logger(err == COMMS_SUCCESS ? LOG_DEBUG1: LOG_WARN1, "snd=%u", err);
				if(err == COMMS_SUCCESS) {
					m_busy = true;
					return true;
				}
			}
			else warn1("pl");
			messagepool_put(msg);
		}
		else warn1("pool");
	}
	else warn1("bsy");
	return false;
}
/*
void scheduleNextAnnouncement() {
	uint32_t next = randomperiod(m_announcements);
	debug1("next %"PRIu32, next);
	call Timer.startOneShot(next);
}

event void Timer.fired() {
	m_announcements++;
	announce(DEVA_ANNOUNCEMENT_INTERFACE_ID, DEVICE_ANNOUNCEMENT_VERSION, AM_BROADCAST_ADDR); // Currently announcements only on first interface
	scheduleNextAnnouncement();
}
*/

static void radio_status_changed (comms_layer_t* comms, comms_status_t status, void* user) {
    // Actual status change is checked by polling, but the callback is mandatory
}

static void radio_send_done(comms_layer_t *comms, comms_msg_t *msg, comms_error_t result, void *user) {
	logger(result == COMMS_SUCCESS ? LOG_DEBUG1: LOG_WARN1, "snt(%d)", (int)result);

	while(osOK != osMutexAcquire(m_mutex, osWaitForever));

	device_announcer_t * an = (device_announcer_t*)user;
	if(NULL != an->comms_ctrl) {
		comms_sleep_allow(an->comms_ctrl);
	}

	messagepool_put(msg);
	m_busy = false;

	osMutexRelease(m_mutex);
}

static void radio_recv(comms_layer_t* comms, const comms_msg_t *msg, void *user) {
	uint8_t len = comms_get_payload_length(comms, msg);
	uint8_t* payload = comms_get_payload(comms, msg, len);
	debugb1("rcv[%p]", payload, len, user);

	while(osOK != osMutexAcquire(m_mutex, osWaitForever));

	device_announcer_t * an = (device_announcer_t*)user;
	if(len >= 2) {
		uint8_t version = ((uint8_t*)payload)[1];
		if(version > DEVICE_ANNOUNCEMENT_VERSION) {
			version = DEVICE_ANNOUNCEMENT_VERSION;
		}
		switch(((uint8_t*)payload)[0]) {
			case DEVA_ANNOUNCEMENT:
				if(version == DEVICE_ANNOUNCEMENT_VERSION) { // version 2
					if(len >= sizeof(device_announcement_v2_t)) {
						device_announcement_v2_t* da = (device_announcement_v2_t*)payload;
						infob1("anc %"PRIu32":%"PRIu32, da->guid, 8,
							(uint32_t)(da->boot_number), (uint32_t)(da->uptime));
						(void)da;
						//signal DeviceAnnouncement.received(call AMPacket.source[iface](msg), da); // TODO a proper event?
					}
				}
				else if(version == 1) { // version 1 - upgrade to current version structure
					if(len >= sizeof(device_announcement_v1_t)) {
						device_announcement_v1_t* da1 = (device_announcement_v1_t*)payload;
						device_announcement_v2_t da;

						da.header = da1->header;
						da.version = DEVICE_ANNOUNCEMENT_VERSION;
						memcpy(da.guid, da1->guid, 8);
						da.boot_number = da1->boot_number;

						da.boot_time = da1->boot_time;
						da.uptime = da1->uptime;
						da.lifetime = da1->lifetime;
						da.announcement = da1->announcement;

						da.uuid = da1->uuid;

						da.position_type = 'U'; // position type in V1 is not specified ... could be anything
						da.latitude = da1->latitude;
						da.longitude = da1->longitude;
						da.elevation = da1->elevation;

						da.radio_tech = 0; // 0:unknown(channel info invalid)
						da.radio_channel = 0;

						da.ident_timestamp = da1->ident_timestamp;

						da.feature_list_hash = da1->feature_list_hash;

						infob1("anc %"PRIu32":%"PRIu32, da.guid, 8,
							(uint32_t)(da.boot_number), (uint32_t)(da.uptime));
						//signal DeviceAnnouncement.received(call AMPacket.source[iface](msg), &da); // TODO a proper event?
					}
				}
				else {
					warn1("%04X ver %u", comms_am_get_source(comms, msg),
						((uint8_t*)payload)[1]); // Unknown version ... what to do?
				}
			break;

			case DEVA_QUERY: {
				info1("qry[%p] v%d %04X", comms, version, comms_am_get_source(comms, msg));
				announce(an, version, comms_am_get_source(comms, msg));
			}
			break;

			case DEVA_DESCRIBE:
				info1("dsc[%p] %04X", comms, comms_am_get_source(comms, msg));
				describe(an, version, comms_am_get_source(comms, msg));
			break;

			case DEVA_LIST_FEATURES:
				if(len >= 3) {
					info1("lst[%p] %04X", comms, comms_am_get_source(comms, msg));
					list_features(an, comms_am_get_source(comms, msg), ((uint8_t*)payload)[2]);
				}
			break;

			default:
				warnb1("dflt", payload, len);
				break;
		}
	}

	osMutexRelease(m_mutex);
}

static bool unguarded_deva_poll() {
	uint32_t now = osCounterGetSecond();
	device_announcer_t* an = m_announcers;
	debug1("poll %"PRIu32, now);
	if(m_busy) {
		warn1("busy");
		return false;
	}
	while(an != NULL) {
		if((an->period > 0)
		 &&(an->last + next_announcement(an->announcements, an->period) <= now)) {
			if(announce(an, DEVICE_ANNOUNCEMENT_VERSION, AM_BROADCAST_ADDR)) {
				an->last = now;
				an->announcements++;
				m_announcements++;
			}
			return true;
		}
		an = an->next;
	}
	return false;
}
