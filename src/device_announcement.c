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
#include "node_coordinates.h"

#include "endianness.h"

#include "cmsis_os2_ext.h"

#include "mist_comm.h"
#include "mist_comm_am.h"
#include "mist_comm_pool.h"

#include "loglevels.h"
#define __MODUUL__ "DevA"
#define __LOG_LEVEL__ ( LOG_LEVEL_device_announcement & BASE_LOG_LEVEL )
#include "log.h"
#include "sys_panic.h"

#define DEVA_MIN_PERIOD_S 10
#define DEVA_MAX_PERIOD_S (365*24*3600)

/**
 * A structure for communicating data or actions from radio thread
 * into the announcement thread.
 **/
typedef struct announcement_action {
	device_announcer_t * p_anc;

	enum DeviceAnnouncementHeaderEnum action;

	comms_msg_t * p_msg; // For incoming messages with context that might need to be forwarded

	// For requests about this device
	struct {
		am_addr_t address;
		uint8_t version;
		uint8_t offset;
	} request;
} announcement_action_t;


#define ANNC_FLAG_SNT (1 << 0)
#define ANNC_FLAG_RCV (1 << 1)
#define ANNC_FLAG_NEW (1 << 2)
#define ANNC_FLAGS    (   0x7)


// How often the announcement module is polled
#define DEVICE_ANNOUNCEMENT_POLL_PERIOD_S 60

extern uint8_t radio_channel (void); // TODO header

static comms_msg_t * handle_action (const announcement_action_t * aa);
static comms_msg_t * announce (device_announcer_t * an, uint8_t version, am_addr_t destination);

static void radio_status_changed (comms_layer_t * comms, comms_status_t status, void * user);
static void radio_send_done (comms_layer_t * comms, comms_msg_t * msg, comms_error_t result, void * user);
static void radio_receive (comms_layer_t * comms, const comms_msg_t * msg, void * user);


static device_announcer_t * mp_announcers;

static time_t m_boot_time;

static osMutexId_t m_mutex;
static osThreadId_t m_thread_id;

static osMessageQueueId_t m_action_queue;

static comms_pool_t * mp_pool;
static comms_msg_t * mp_msg;


static void nx_uuid_application (nx_uuid_t * uuid)
{
	memcpy(uuid, UUID_APPLICATION_BYTES, 16);
}


static uint32_t next_announcement (uint32_t announcements, uint16_t period)
{
	uint32_t next = period;
	if (announcements == 0)
	{
		next = period / 10;
	}
	else if (announcements < 5)
	{
		next = period / 5;
	}
	if (0 == next)
	{
		return 1;
	}
	return next;
}


static void update_boot_time (void)
{
	if (m_boot_time == ((time_t)-1)) // Adjust boot time only when it is not known
	{
		time_t now = time(NULL);
		if (((time_t)-1) != now)
		{
			m_boot_time = now - osCounterGetSecond();
		}
	}
}


static void allow_sleep (void)
{
	device_announcer_t * p_anc = mp_announcers;
	while (NULL != p_anc)
	{
		if (NULL != p_anc->comms_ctrl)
		{
			comms_sleep_allow(p_anc->comms_ctrl);
		}
		p_anc = p_anc->next;
	}
}


static device_announcer_t * get_pending_announcer (uint32_t * timeout_s)
{
	uint32_t now = osCounterGetSecond();
	device_announcer_t * p_anc = mp_announcers;
	while (NULL != p_anc)
	{
		// Limit period to "reasonable" values to not break calculations
		if ((p_anc->period >= DEVA_MIN_PERIOD_S) && (p_anc->period <= DEVA_MAX_PERIOD_S))
		{
		    uint32_t next = p_anc->last + next_announcement(p_anc->announcements, p_anc->period);
		    if (next <= now)
		    {
				return p_anc;
		    }
		    else
		    {
		    	uint32_t remaining = next - now;
		    	if (remaining < *timeout_s)
		    	{
		    		*timeout_s = remaining;
		    	}
		    }
		}
		p_anc = p_anc->next;
	}
	return NULL;
}


static uint32_t process_announcements (uint32_t flags, uint32_t current_timeout_s)
{
	uint32_t timeout_s = current_timeout_s;
	device_announcer_t * p_anc = NULL;

	update_boot_time();

	if (ANNC_FLAG_SNT & flags)
	{
		comms_pool_put(mp_pool, mp_msg);
		mp_msg = NULL;
	}

	if (NULL != mp_msg)
	{
		return timeout_s;
	}

	while (osOK != osMutexAcquire(m_mutex, osWaitForever));

	if (NULL == mp_msg)
	{
		announcement_action_t aa;
		if (osOK == osMessageQueueGet(m_action_queue, &aa, NULL, 0))
		{
			// Check that the announcer is valid (has not been removed for example)
			p_anc = mp_announcers;
			while (NULL != p_anc)
			{
				if (p_anc == aa.p_anc)
				{
					break;
				}
				p_anc = p_anc->next;
			}

			if (NULL != p_anc)
			{
				mp_msg = handle_action(&aa);
				if (NULL != aa.p_msg)
				{
					comms_pool_put(mp_pool, aa.p_msg); // Release the message
				}
			}
			else
			{
				err1("p %p", aa.p_anc);
			}
		}
	}

	if (NULL == mp_msg)
	{
		p_anc = get_pending_announcer(&timeout_s);
		if (NULL != p_anc)
		{
			debug1("annc %p", p_anc);
			mp_msg = announce(p_anc, DEVICE_ANNOUNCEMENT_VERSION, AM_BROADCAST_ADDR);
			if (NULL != mp_msg)
			{
				p_anc->last = osCounterGetSecond();
				p_anc->announcements++;
			}
			else
			{
				warn1("msg");
			}
			timeout_s = DEVICE_ANNOUNCEMENT_POLL_PERIOD_S;
		}
	}

	if (NULL != mp_msg)
	{
		if (NULL != p_anc->comms_ctrl)
		{
			comms_sleep_block(p_anc->comms_ctrl);
			while (COMMS_STARTED != comms_status(p_anc->comms));
			timeout_s = 1; // Leave comms on for 1 second
		}

		comms_error_t err = comms_send(p_anc->comms, mp_msg, radio_send_done, NULL);
		logger(COMMS_SUCCESS == err ? LOG_DEBUG1: LOG_WARN1, "snd=%u", err);
		if (COMMS_SUCCESS != err)
		{
			comms_pool_put(mp_pool, mp_msg);
			mp_msg = NULL;
		}
	}
	else if (0 == flags) // A timeout just happened and we have nothing to do
	{
		allow_sleep();
		debug1("sleep %"PRIu32, timeout_s);
	}
	else
	{
		debug1("flgs %"PRIx32" sleep %"PRIu32, flags, timeout_s);
	}

	osMutexRelease(m_mutex);

	return timeout_s;
}


static void announcement_loop (void * arg)
{
	uint32_t timeout_s = DEVICE_ANNOUNCEMENT_POLL_PERIOD_S;

	for (;;)
	{
		uint32_t flags = osThreadFlagsWait(ANNC_FLAGS, osFlagsWaitAny, timeout_s * 1000UL + 1);

		if (osFlagsErrorTimeout == flags)
		{
			flags = 0;
		}

		timeout_s = process_announcements(flags, timeout_s);
	}
}


bool deva_init (comms_pool_t * p_pool)
{
	mp_announcers = NULL;
	m_boot_time = ((time_t)-1);

	mp_pool = p_pool;
	mp_msg = NULL;

	const osMutexAttr_t annc_mutex_attr = { "annc", osMutexPrioInherit, NULL, 0U };
	m_mutex = osMutexNew(&annc_mutex_attr);
	if (NULL == m_mutex)
	{
		return false;
	}

	m_action_queue = osMessageQueueNew(1, sizeof(announcement_action_t), NULL);
	if (NULL == m_action_queue)
	{
    	osMutexDelete(m_mutex);
    	m_mutex = NULL;
		return false;
	}

    const osThreadAttr_t annc_thread_attr = { .name = "annc", .stack_size = 1536 };
    m_thread_id = osThreadNew(announcement_loop, NULL, &annc_thread_attr);
    if (NULL == m_thread_id)
    {
    	osMessageQueueDelete(m_action_queue);
    	m_action_queue = NULL;
    	osMutexDelete(m_mutex);
    	m_mutex = NULL;
    	return false;
    }
    return true;
}


bool deva_add_announcer (device_announcer_t * p_anc,
	comms_layer_t * p_comms, comms_sleep_controller_t * p_rctrl,
	uint32_t period_s)
{
	p_anc->comms = p_comms;
	p_anc->comms_ctrl = p_rctrl;
	p_anc->period = period_s;
	p_anc->last = osCounterGetSecond() - (rand() % next_announcement(0, p_anc->period));
	p_anc->announcements = 0;
	p_anc->next = NULL;

	if (COMMS_SUCCESS != comms_register_recv(p_comms, &(p_anc->rcvr),
		radio_receive, p_anc, AMID_DEVICE_ANNOUNCEMENT))
	{
		err1("regr");
		return false;
	}

	if (NULL != p_rctrl)
	{
		if (COMMS_SUCCESS != comms_register_sleep_controller(p_comms, p_rctrl, radio_status_changed, NULL))
		{
			err1("rctrl");
		}
	}

	if ((period_s >= DEVA_MIN_PERIOD_S) && (period_s <= DEVA_MAX_PERIOD_S))
	{
		debug1("annc %p nxt %d", p_anc, p_anc->last + next_announcement(0, p_anc->period) - osCounterGetSecond());
	}
	else
	{
		debug1("annc %p nvr", p_anc);
	}


	while (osOK != osMutexAcquire(m_mutex, osWaitForever));

	device_announcer_t** pp_announcers = &mp_announcers;
	while (NULL != *pp_announcers)
	{
		pp_announcers = &((*pp_announcers)->next);
	}
	*pp_announcers = p_anc;

	osMutexRelease(m_mutex);

	osThreadFlagsSet(m_thread_id, ANNC_FLAG_NEW);

	return true;
}


bool deva_remove_announcer (device_announcer_t * p_anc)
{
	while (osOK != osMutexAcquire(m_mutex, osWaitForever));

	device_announcer_t** pp_announcers = &mp_announcers;
	while (NULL != *pp_announcers)
	{
		if (p_anc == *pp_announcers)
		{
			*pp_announcers = (*pp_announcers)->next;

			osMutexRelease(m_mutex); // Removed, rest of teardown is independent

			if (NULL != p_anc->comms_ctrl)
			{
				if (COMMS_SUCCESS != comms_deregister_sleep_controller(p_anc->comms, p_anc->comms_ctrl))
				{
					sys_panic("drc");
				}
			}

			if (COMMS_SUCCESS != comms_deregister_recv(p_anc->comms, &(p_anc->rcvr)))
			{
				sys_panic("drr");
			}

			return true;
		}
		pp_announcers = &((*pp_announcers)->next);
	}

	osMutexRelease(m_mutex);
	return false;
}


static comms_msg_t * announce (device_announcer_t * an, uint8_t version, am_addr_t destination)
{
	comms_msg_t * msg = comms_pool_get(mp_pool, 0);
	if (NULL != msg)
	{
		uint8_t length = 0;
		comms_init_message(an->comms, msg);
		if (1 == version)
		{
			device_announcement_v1_t * anc = (device_announcement_v1_t*)comms_get_payload(an->comms, msg, sizeof(device_announcement_v1_t));
			if (NULL != anc)
			{
				coordinates_geo_t geo;

				anc->header = DEVA_ANNOUNCEMENT;
				anc->version = DEVICE_ANNOUNCEMENT_VERSION;
				sigGetEui64((uint8_t*)anc->guid);
				anc->boot_number = hton32(node_lifetime_boots());

				anc->boot_time = hton64(m_boot_time);
				anc->uptime = hton32(osCounterGetSecond());
				anc->lifetime = hton32(node_lifetime_seconds());
				anc->announcement = hton32(an->announcements);

				nx_uuid_application(&(anc->uuid));

				if (node_coordinates_get(&geo))
				{
					anc->latitude = hton32(geo.latitude);
					anc->longitude = hton32(geo.longitude);
					anc->elevation = hton32(geo.elevation);
				}
				else
				{
					anc->latitude = 0;
					anc->longitude = 0;
					anc->elevation = 0;
				}

				anc->ident_timestamp = hton64(IDENT_TIMESTAMP);
				anc->feature_list_hash = hton32(devf_hash());

				length = sizeof(device_announcement_v1_t);
			}
		}
		else
		{
			device_announcement_v2_t * anc = (device_announcement_v2_t*)comms_get_payload(an->comms, msg, sizeof(device_announcement_v2_t));
			if (NULL != anc)
			{
				coordinates_geo_t geo;

				anc->header = DEVA_ANNOUNCEMENT;
				anc->version = DEVICE_ANNOUNCEMENT_VERSION;
				sigGetEui64((uint8_t*)anc->guid);
				anc->boot_number = hton32(node_lifetime_boots());

				anc->boot_time = hton64(m_boot_time);
				anc->uptime = hton32(osCounterGetSecond());
				anc->lifetime = hton32(node_lifetime_seconds());
				anc->announcement = hton32(an->announcements);

				nx_uuid_application(&(anc->uuid));
				if (node_coordinates_get(&geo))
				{
					anc->position_type = geo.type;
					anc->latitude = hton32(geo.latitude);
					anc->longitude = hton32(geo.longitude);
					anc->elevation = hton32(geo.elevation);
				}
				else
				{
					anc->position_type = 'U';
					anc->latitude = 0;
					anc->longitude = 0;
					anc->elevation = 0;
				}

				anc->radio_tech = 1; // Always 802.15.4 ... for now
				anc->radio_channel = radio_channel(); // FIXME

				anc->ident_timestamp = hton64(IDENT_TIMESTAMP);
				anc->feature_list_hash = hton32(devf_hash());

				length = sizeof(device_announcement_v2_t);
			}
		}

		if (length > 0)
		{
			comms_set_packet_type(an->comms, msg, AMID_DEVICE_ANNOUNCEMENT);
			comms_am_set_destination(an->comms, msg, destination);
			comms_set_payload_length(an->comms, msg, length);
			return msg;
		}
		comms_pool_put(mp_pool, msg);
	}
	else warn1("pool");

	return NULL;
}

static comms_msg_t * describe(device_announcer_t* an, uint8_t version, am_addr_t destination)
{
	comms_msg_t * msg = comms_pool_get(mp_pool, 0);
	if (NULL != msg)
	{
		uint8_t length = 0;
		comms_init_message(an->comms, msg);
		if (version == 1)
		{
			device_description_v1_t* anc = (device_description_v1_t*)comms_get_payload(an->comms, msg, sizeof(device_description_v1_t));
			if (NULL != anc)
			{
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
		else
		{
			device_description_v2_t* anc = (device_description_v2_t*)comms_get_payload(an->comms, msg, sizeof(device_description_v2_t));
			if (NULL != anc)
			{
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

		if (length > 0)
		{
			comms_set_packet_type(an->comms, msg, AMID_DEVICE_ANNOUNCEMENT);
			comms_am_set_destination(an->comms, msg, destination);
			comms_set_payload_length(an->comms, msg, length);
			return msg;
		}

		comms_pool_put(mp_pool, msg);
	}
	else warn1("pool");

	return NULL;
}

static comms_msg_t * list_features(device_announcer_t* an, am_addr_t destination, uint8_t offset)
{
	comms_msg_t * msg = comms_pool_get(mp_pool, 0);
	if (NULL != msg)
	{
		uint8_t space = (comms_get_payload_max_length(an->comms) - sizeof(device_features_t)) / sizeof(uuid_t);

		comms_init_message(an->comms, msg);

		device_features_t * anc = (device_features_t*)comms_get_payload(an->comms, msg, sizeof(device_features_t) + space*sizeof(uuid_t));
		if (NULL != anc)
		{
			uint8_t total_features = devf_count();
			uint8_t ftrs = 0;
			uint8_t skip = 0;

			anc->header = DEVA_FEATURES;
			anc->version = DEVICE_ANNOUNCEMENT_VERSION;
			sigGetEui64((uint8_t*)anc->guid);
			anc->boot_number = hton32(node_lifetime_boots());

			anc->total = total_features;
			anc->offset = offset;

			for (;(ftrs<space)&&(offset+skip+ftrs<total_features);)
			{
				if (devf_get_feature(offset+ftrs, &(anc->features[ftrs])))
				{
					ftrs++;
				}
				else // Feature disabled ... or problematic?
				{
					skip++;
				}
			}

			debugb1("ftrs %u total %u", anc, sizeof(device_features_t)+ftrs*sizeof(nx_uuid_t), ftrs, anc->total);

			comms_set_packet_type(an->comms, msg, AMID_DEVICE_ANNOUNCEMENT);
			comms_am_set_destination(an->comms, msg, destination);
			comms_set_payload_length(an->comms, msg, sizeof(device_features_t) + ftrs*sizeof(uuid_t));

			return msg;
		}
		else warn1("pl");

		comms_pool_put(mp_pool, msg);
	}
	else warn1("pool");

	return NULL;
}


static void radio_status_changed (comms_layer_t * comms, comms_status_t status, void * user)
{
    // Actual status change is checked by polling, but the callback is mandatory
}


static void radio_send_done (comms_layer_t * comms, comms_msg_t * msg, comms_error_t result, void * user)
{
	logger(result == COMMS_SUCCESS ? LOG_DEBUG1: LOG_WARN1, "snt(%d)", (int)result);
	osThreadFlagsSet(m_thread_id, ANNC_FLAG_SNT);
}


/**
 * Parse incoming messaages. When the message contains a request, pass on only
 * the request through the request fields of announcement_action_t. If it contains
 * info about another node, copy the entire message and pass that to the
 * announcement thread.
 **/
static void radio_receive (comms_layer_t * comms, const comms_msg_t * msg, void * user)
{
	uint8_t len = comms_get_payload_length(comms, msg);
	uint8_t * payload = comms_get_payload(comms, msg, len);
	am_addr_t source = comms_am_get_source(comms, msg);

	debugb1("c %p rcv %04"PRIX16, payload, len, comms, source);
	if (len >= 2)
	{
		announcement_action_t aa;
		aa.p_anc = (device_announcer_t*)user;
		aa.action = ((uint8_t*)payload)[0];
		aa.p_msg = NULL;
		switch (aa.action)
		{
			case DEVA_ANNOUNCEMENT:
			case DEVA_DESCRIPTION:
			case DEVA_FEATURES:
				aa.p_msg = comms_pool_get(mp_pool, 0);
				if (NULL != aa.p_msg)
				{
					memcpy(aa.p_msg, msg, sizeof(comms_msg_t));
					if (osOK != osMessageQueuePut(m_action_queue, &aa, 0, 0))
					{
						warn1("qb"); // Queue has overflowed
						comms_pool_put(mp_pool, aa.p_msg);
					}
					else
					{
						osThreadFlagsSet(m_thread_id, ANNC_FLAG_RCV);
					}
				}
				else
				{
					warn1("mb"); // No messages available for copy
				}
			break;

			// All 3 requests handled similarly, but features has an extra argument
			case DEVA_LIST_FEATURES:
				if (len >= 3)
				{
					aa.request.offset = ((uint8_t*)payload)[2];
					if (osOK != osMessageQueuePut(m_action_queue, &aa, 0, 0))
					{
						warn1("qb"); // Queue has overflowed
						comms_pool_put(mp_pool, aa.p_msg);
					}
					else
					{
						osThreadFlagsSet(m_thread_id, ANNC_FLAG_RCV);
					}
				}
			break;

			case DEVA_DESCRIBE:
			case DEVA_QUERY:
				aa.request.address = source;
				aa.request.version = ((uint8_t*)payload)[1];

				if (osOK != osMessageQueuePut(m_action_queue, &aa, 0, 0))
				{
					warn1("qb"); // Queue has overflowed
				}
				else
				{
					osThreadFlagsSet(m_thread_id, ANNC_FLAG_RCV);
				}
			break;

			default:
				warnb1("dflt %d", payload, len, (int)aa.action);
			break;
		}
	}
	else
	{
		err1("len %d", (int)len);
	}
}


static uint8_t adjust_version (uint8_t version)
{
	if (version > DEVICE_ANNOUNCEMENT_VERSION) // Downgrade version for most cases
	{
		return DEVICE_ANNOUNCEMENT_VERSION;
	}
	return version;
}


static comms_msg_t * handle_action (const announcement_action_t * aa)
{
	switch (aa->action)
	{
		case DEVA_ANNOUNCEMENT:
		{
			uint8_t len = comms_get_payload_length(aa->p_anc->comms, aa->p_msg);
			uint8_t * payload = comms_get_payload(aa->p_anc->comms, aa->p_msg, len);
			am_addr_t source = comms_am_get_source(aa->p_anc->comms, aa->p_msg);
			uint8_t version = ((uint8_t*)payload)[1];

			if (version == DEVICE_ANNOUNCEMENT_VERSION)
			{ // version 2
				if (len >= sizeof(device_announcement_v2_t))
				{
					device_announcement_v2_t* da = (device_announcement_v2_t*)payload;
					infob1("anc %"PRIu32":%"PRIu32, da->guid, 8,
						(uint32_t)(da->boot_number), (uint32_t)(da->uptime));
					(void)da;
					//signal DeviceAnnouncement.received(call AMPacket.source[iface](msg), da); // TODO a proper event?
				}
			}
			else if (version == 1)
			{ // version 1 - upgrade to current version structure (2 currently)
				if (len >= sizeof(device_announcement_v1_t))
				{
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
			else
			{
				warn1("%04"PRIX16" ver %u", source, (unsigned int)((uint8_t*)payload)[1]); // Unknown version ... what to do?
			}
		}
		break;

		case DEVA_QUERY:
			info1("qry v%d %04"PRIX16, (int)adjust_version(aa->request.version), aa->request.address);
			return announce(aa->p_anc, adjust_version(aa->request.version), aa->request.address);
		case DEVA_DESCRIBE:
			info1("dsc %04"PRIX16, aa->request.address);
			return describe(aa->p_anc, adjust_version(aa->request.version), aa->request.address);
		case DEVA_LIST_FEATURES:
			info1("lst %04"PRIX16, aa->request.address);
			return list_features(aa->p_anc, aa->request.address, aa->request.offset);

		default:
			warn1("dflt %d", (int)aa->action);
		break;
	}

	return NULL;
}


#ifdef UNITTEST

#include "device_announcement_test.h"

uint32_t unittest_process_announcements (uint32_t flags, uint32_t current_timeout_s)
{
	return process_announcements(flags, current_timeout_s);
}

#endif//UNITTEST
