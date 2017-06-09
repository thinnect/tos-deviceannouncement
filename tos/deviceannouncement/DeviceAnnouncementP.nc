/**
 * Device announcement protocol implementation.
 *
 * @author Raido Pahtma
 * @license MIT
 **/
#include "sec_tmilli.h"
#include "DeviceAnnouncement.h"
generic module DeviceAnnouncementP(uint8_t ifaces, uint8_t total_features) {
	provides interface DeviceAnnouncement;
	uses {
		interface AMSend[uint8_t iface];
		interface AMPacket[uint8_t iface];
		interface Receive[uint8_t iface];
		interface LocalIeeeEui64;

		interface GetStruct<uuid_t> as PlatformUuid128;
		interface GetStruct<uuid_t> as ApplicationUuid128;
		interface GetStruct<uuid_t> as ManufacturerUuid128;

		interface GetStruct<uuid_t> as DeviceFeatureUuid128[uint8_t fidx];

		interface Get<time64_t> as ProductionTime;

		interface GetStruct<coordinates_geo_t> as GetGeo;

		interface RealTimeClock;

		interface Timer<TMilli>;
		interface Random;
		interface Crc;

		interface Get<uint32_t> as BootNumber;
		interface Get<uint32_t> as Lifetime;
		interface Get<uint32_t> as Uptime;
		interface Boot as BootStable; // Fired only after the boot is considered stable

		interface Pool<message_t> as MessagePool;
	}
}
implementation {

	#define __MODUUL__ "DevA"
	#define __LOG_LEVEL__ ( LOG_LEVEL_DeviceAnnouncementP & BASE_LOG_LEVEL )
	#include "log.h"

	bool m_busy = FALSE;

	time64_t m_boot_time = -1;
	uint32_t m_announcements = 0;

	uint32_t randomperiod(uint32_t announcements) {
		if(announcements == 0) {
			return call Random.rand16();
		}
		else if(announcements < 6) {
			return SEC_TMILLI(4*60) + call Random.rand16(); // + random up to ~65 seconds
		}
		return SEC_TMILLI(27.5*60)  + 5*(call Random.rand16()); // + random up to ~5*65 seconds
	}

	task void bootTimeTask() {
		if(m_boot_time == ((time64_t)-1)) { // Adjust boot time only when it is not known
			time64_t now = call RealTimeClock.time();
			if(now != ((time64_t)-1)) {
				m_boot_time = now - call Uptime.get();
			}
		}
	}

	event void BootStable.booted() {
		post bootTimeTask();
		call Timer.startOneShot(randomperiod(0));
	}

	uint32_t featureListHash() { // Really simple hashing for now
		uint16_t hash = 0;
		uint8_t ftrs;
		for(ftrs=0;ftrs<total_features;ftrs++) {
			uuid_t uuid;
			if(call DeviceFeatureUuid128.get[ftrs](&uuid) == SUCCESS) {
				hash = call Crc.seededCrc16(hash, &uuid, sizeof(uuid));
			}
		}
		return (((uint32_t)hash)<<16) + hash;
	}

	error_t announce(uint8_t iface, am_addr_t destination) {
		if(!m_busy) {
			message_t* msg = call MessagePool.get();
			if(msg != NULL) {
				device_announcement_t* anc = (device_announcement_t*)call AMSend.getPayload[iface](msg, sizeof(device_announcement_t));
				if(anc != NULL) {
					error_t err;
					ieee_eui64_t guid = call LocalIeeeEui64.getId();
					uuid_t uuid;
					coordinates_geo_t geo;

					anc->header = DEVA_ANNOUNCEMENT;
					anc->version = DEVICE_ANNOUNCEMENT_VERSION;
					memcpy(anc->guid, guid.data, sizeof(anc->guid));
					anc->boot_number = call BootNumber.get();

					anc->boot_time = m_boot_time;
					anc->uptime = call Uptime.get();
					anc->lifetime = call Lifetime.get();
					anc->announcement = m_announcements;

					call ApplicationUuid128.get(&uuid);
					hton_uuid(&(anc->uuid), &uuid);

					call GetGeo.get(&geo);
					anc->latitude = geo.latitude;
					anc->longitude = geo.longitude;
					anc->elevation = geo.elevation;

					anc->ident_timestamp = IDENT_TIMESTAMP;
					anc->feature_list_hash = featureListHash();

					err = call AMSend.send[iface](destination, msg, sizeof(device_announcement_t));
					logger(err == SUCCESS ? LOG_DEBUG1: LOG_WARN1, "snd=%u", err);
					if(err == SUCCESS) {
						m_busy = TRUE;
						return SUCCESS;
					}
				}
				call MessagePool.put(msg);
			}
		}
		warn1("bsy");
		return FAIL;
	}

	error_t describe(uint8_t iface, am_addr_t destination) {
		if(!m_busy) {
			message_t* msg = call MessagePool.get();
			if(msg != NULL) {
				device_description_t* anc = (device_description_t*)call AMSend.getPayload[iface](msg, sizeof(device_description_t));
				if(anc != NULL) {
					error_t err;
					ieee_eui64_t guid = call LocalIeeeEui64.getId();
					uuid_t uuid;

					anc->header = DEVA_DESCRIPTION;
					anc->version = DEVICE_ANNOUNCEMENT_VERSION;
					memcpy(anc->guid, guid.data, sizeof(anc->guid));
					anc->boot_number = call BootNumber.get();

					call PlatformUuid128.get(&uuid);
					hton_uuid(&(anc->platform), &uuid);

					call ManufacturerUuid128.get(&uuid);
					hton_uuid(&(anc->manufacturer), &uuid);

					anc->production = call ProductionTime.get();

					anc->ident_timestamp = IDENT_TIMESTAMP;
					anc->sw_major_version = SW_MAJOR_VERSION;
					anc->sw_minor_version = SW_MINOR_VERSION;
					anc->sw_patch_version = SW_PATCH_VERSION;

					err = call AMSend.send[iface](destination, msg, sizeof(device_description_t));
					logger(err == SUCCESS ? LOG_DEBUG1: LOG_WARN1, "snd=%u", err);
					if(err == SUCCESS) {
						m_busy = TRUE;
						return SUCCESS;
					}
				}
				call MessagePool.put(msg);
			}
		}
		warn1("bsy");
		return FAIL;
	}

	error_t list_features(uint8_t iface, am_addr_t destination, uint8_t offset) {
		if(!m_busy) {
			message_t* msg = call MessagePool.get();
			if(msg != NULL) {
				uint8_t space = (call AMSend.maxPayloadLength[iface]() - sizeof(device_features_t)) / sizeof(nx_uuid_t);
				device_features_t* anc = (device_features_t*)call AMSend.getPayload[iface](msg, sizeof(device_features_t) + space*sizeof(nx_uuid_t));
				if(anc != NULL) {
					error_t err;
					ieee_eui64_t guid = call LocalIeeeEui64.getId();
					uuid_t uuid;
					uint8_t ftrs = 0;

					anc->header = DEVA_FEATURES;
					anc->version = DEVICE_ANNOUNCEMENT_VERSION;
					memcpy(anc->guid, guid.data, sizeof(anc->guid));
					anc->boot_number = call BootNumber.get();

					anc->total = total_features;
					anc->offset = offset;

					for(ftrs=0;(ftrs<space)&&(offset+ftrs<total_features);ftrs++) {
						if(call DeviceFeatureUuid128.get[offset+ftrs](&uuid) == SUCCESS) {
							hton_uuid(&(anc->features[ftrs]), &uuid);
						}
						else {
							memset(&(anc->features[ftrs]), 0xFF, sizeof(nx_uuid_t)); // indicate problem
						}
					}

					debugb1("ftrs %u total %u", anc, sizeof(device_features_t)+ftrs*sizeof(nx_uuid_t), ftrs, total_features);

					err = call AMSend.send[iface](destination, msg, sizeof(device_features_t) + ftrs*sizeof(nx_uuid_t));
					logger(err == SUCCESS ? LOG_DEBUG1: LOG_WARN1, "snd=%u", err);
					if(err == SUCCESS) {
						m_busy = TRUE;
						return SUCCESS;
					}
				}
				call MessagePool.put(msg);
			}
		}
		warn1("bsy");
		return FAIL;
	}

	void scheduleNextAnnouncement() {
		uint32_t next = randomperiod(m_announcements);
		debug1("next %"PRIu32, next);
		call Timer.startOneShot(next);
	}

	event void Timer.fired() {
		m_announcements++;
		announce(DA_ANNOUNCEMENT_INTERFACE_ID, AM_BROADCAST_ADDR); // Currently announcements only on first interface
		scheduleNextAnnouncement();
	}

	event void AMSend.sendDone[uint8_t iface](message_t* msg, error_t err) {
		logger(err == SUCCESS ? LOG_DEBUG1: LOG_WARN1, "snt(%u)", err);
		call MessagePool.put(msg);
		m_busy = FALSE;
	}

	event message_t* Receive.receive[uint8_t iface](message_t* msg, void* payload, uint8_t len) {
		debugb1("rcv[%u]", payload, len, iface);
		if(len >= 2) {
			switch(((uint8_t*)payload)[0]) {
				case DEVA_ANNOUNCEMENT:
					if(((uint8_t*)payload)[1] == DEVICE_ANNOUNCEMENT_VERSION) {
						if(len >= sizeof(device_announcement_t)) {
							device_announcement_t* da = (device_announcement_t*)payload;
							infob1("anc %"PRIu32":%"PRIu32, da->guid, 8,
								(uint32_t)(da->boot_number), (uint32_t)(da->uptime));
							signal DeviceAnnouncement.received(call AMPacket.source[iface](msg), da); // TODO a proper event?
						}
					}
					else {
						warn1("%04X ver %u", call AMPacket.source[iface](msg),
							((uint8_t*)payload)[1]); // Unknown version ... what to do?
					}
				break;

				case DEVA_QUERY:
					info1("qry[%u] %04X", iface, call AMPacket.source[iface](msg));
					announce(iface, call AMPacket.source[iface](msg));
				break;

				case DEVA_DESCRIBE:
					info1("dsc[%u] %04X", iface, call AMPacket.source[iface](msg));
					describe(iface, call AMPacket.source[iface](msg));
				break;

				case DEVA_LIST_FEATURES:
					if(len >= 3) {
						info1("lst[%u] %04X", iface, call AMPacket.source[iface](msg));
						list_features(iface, call AMPacket.source[iface](msg), ((uint8_t*)payload)[2]);
					}
				break;

				default:
					warnb1("dflt", payload, len);
					break;
			}
		}
		return msg;
	}

	async event void RealTimeClock.changed(time64_t old, time64_t current) {
		post bootTimeTask();
	}

	default command error_t DeviceFeatureUuid128.get[uint8_t fidx](uuid_t* uuid) { return ELAST; }

	default event void DeviceAnnouncement.received(am_addr_t addr, device_announcement_t* announcement) { }

	default command error_t AMSend.send[uint8_t iface](am_addr_t addr, message_t* msg, uint8_t len) { return EINVAL; }
	default command void* AMSend.getPayload[uint8_t iface](message_t* msg, uint8_t len) { return NULL; }
	default command uint8_t AMSend.maxPayloadLength[uint8_t iface]() { return 0; }

	default command am_addr_t AMPacket.address[uint8_t iface]() { return 0; }
	default command am_addr_t AMPacket.destination[uint8_t iface](message_t* amsg) { return 0; }
	default command am_addr_t AMPacket.source[uint8_t iface](message_t* amsg) { return 0; }

}
