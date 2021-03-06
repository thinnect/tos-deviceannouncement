/**
 * Device announcement protocol implementation.
 *
 * @author Raido Pahtma
 * @license MIT
 **/
#include "sec_tmilli.h"
#include "Coordinates.h"
#include "DeviceSignature.h"
#include "DeviceAnnouncementProtocol.h"
generic module DeviceAnnouncementP(uint8_t ifaces, uint8_t total_features) {
	provides interface DeviceAnnouncement;
	uses {
		interface AMSend[uint8_t iface];
		interface AMPacket[uint8_t iface];
		interface Receive[uint8_t iface];
		interface LocalIeeeEui64;

		interface GetStruct<uuid_t> as ApplicationUuid128;

		interface GetStruct<uuid_t> as DeviceFeatureUuid128[uint8_t fidx];

		interface GetStruct<coordinates_geo_t> as GetGeo;

		interface RealTimeClock;

		interface RadioChannel;

		interface Timer<TMilli>;
		interface Random;
		interface Crc;

		// Compile-time UUID fallback for older device signatures
		interface GetStruct<uuid_t> as PlatformUuid128;

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

	uint8_t featureCount() {
		uint8_t ftrs = 0;
		uint8_t i;
		for(i=0;i<total_features;i++) {
			uuid_t uuid;
			if(call DeviceFeatureUuid128.get[i](&uuid) == SUCCESS) {
				ftrs++;
			}
		}
		return ftrs;
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

	error_t announce(uint8_t iface, uint8_t version, am_addr_t destination) {
		if(!m_busy) {
			message_t* msg = call MessagePool.get();
			if(msg != NULL) {
				uint8_t length = 0;
				if(version == 1) {
					device_announcement_v1_t* anc = (device_announcement_v1_t*)call AMSend.getPayload[iface](msg, sizeof(device_announcement_v1_t));
					if(anc != NULL) {
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

						length = sizeof(device_announcement_v1_t);
					}
				}
				else {
					device_announcement_v2_t* anc = (device_announcement_v2_t*)call AMSend.getPayload[iface](msg, sizeof(device_announcement_v2_t));
					if(anc != NULL) {
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
						anc->position_type = geo.type;
						anc->latitude = geo.latitude;
						anc->longitude = geo.longitude;
						anc->elevation = geo.elevation;

						anc->radio_tech = 1; // Always 802.15.4 ... for now
						anc->radio_channel = call RadioChannel.getChannel();

						anc->ident_timestamp = IDENT_TIMESTAMP;
						anc->feature_list_hash = featureListHash();

						length = sizeof(device_announcement_v2_t);
					}
				}

				if(length > 0) {
					error_t err = call AMSend.send[iface](destination, msg, length);
					logger(err == SUCCESS ? LOG_DEBUG1: LOG_WARN1, "snd=%u", err);
					if(err == SUCCESS) {
						m_busy = TRUE;
						return SUCCESS;
					}
				}
				call MessagePool.put(msg);
			}
		}
		else warn1("bsy");
		return FAIL;
	}

	error_t describe(uint8_t iface, uint8_t version, am_addr_t destination) {
		if(!m_busy) {
			message_t* msg = call MessagePool.get();
			if(msg != NULL) {
				uint8_t length = 0;
				ieee_eui64_t guid = call LocalIeeeEui64.getId();
				if(version == 1) {
					device_description_v1_t* anc = (device_description_v1_t*)call AMSend.getPayload[iface](msg, sizeof(device_description_v1_t));
					if(anc != NULL) {
						anc->header = DEVA_DESCRIPTION;
						anc->version = DEVICE_ANNOUNCEMENT_VERSION;
						memcpy(anc->guid, guid.data, sizeof(anc->guid));
						anc->boot_number = call BootNumber.get();

						if(SIG_GOOD != sigGetPlatformUUID((uint8_t*)&(anc->platform))) {
							uuid_t uuid;
							call PlatformUuid128.get(&uuid);
							hton_uuid(&(anc->platform), &uuid);
						}

						sigGetPlatformManufacturerUUID((uint8_t*)&(anc->manufacturer));

						anc->production = sigGetPlatformProductionTime();

						anc->ident_timestamp = IDENT_TIMESTAMP;
						anc->sw_major_version = SW_MAJOR_VERSION;
						anc->sw_minor_version = SW_MINOR_VERSION;
						anc->sw_patch_version = SW_PATCH_VERSION;

						length = sizeof(device_description_v1_t);
					}
				}
				else {
					device_description_v2_t* anc = (device_description_v2_t*)call AMSend.getPayload[iface](msg, sizeof(device_description_v2_t));
					if(anc != NULL) {
						semver_t hwv = sigGetPlatformVersion();

						anc->header = DEVA_DESCRIPTION;
						anc->version = DEVICE_ANNOUNCEMENT_VERSION;
						memcpy(anc->guid, guid.data, sizeof(anc->guid));
						anc->boot_number = call BootNumber.get();

						if(SIG_GOOD != sigGetPlatformUUID((uint8_t*)&(anc->platform))) {
							uuid_t uuid;
							call PlatformUuid128.get(&uuid);
							hton_uuid(&(anc->platform), &uuid);
						}

						anc->hw_major_version = hwv.major;
						anc->hw_minor_version = hwv.minor;
						anc->hw_assem_version = hwv.patch;

						sigGetPlatformManufacturerUUID((uint8_t*)&(anc->manufacturer));

						anc->production = sigGetPlatformProductionTime();

						anc->ident_timestamp = IDENT_TIMESTAMP;
						anc->sw_major_version = SW_MAJOR_VERSION;
						anc->sw_minor_version = SW_MINOR_VERSION;
						anc->sw_patch_version = SW_PATCH_VERSION;

						length = sizeof(device_description_v2_t);
					}
				}

				if(length > 0) {
					error_t err = call AMSend.send[iface](destination, msg, sizeof(device_description_v2_t));
					logger(err == SUCCESS ? LOG_DEBUG1: LOG_WARN1, "snd=%u", err);
					if(err == SUCCESS) {
						m_busy = TRUE;
						return SUCCESS;
					}
				}
				call MessagePool.put(msg);
			}
		}
		else warn1("bsy");
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
					uint8_t skip = 0;

					anc->header = DEVA_FEATURES;
					anc->version = DEVICE_ANNOUNCEMENT_VERSION;
					memcpy(anc->guid, guid.data, sizeof(anc->guid));
					anc->boot_number = call BootNumber.get();

					anc->total = featureCount();
					anc->offset = offset;

					for(;(ftrs<space)&&(offset+skip+ftrs<total_features);) {
						if(call DeviceFeatureUuid128.get[offset+ftrs](&uuid) == SUCCESS) {
							hton_uuid(&(anc->features[ftrs]), &uuid);
							ftrs++;
						}
						else { // Feature disabled ... or problematic?
							skip++;
						}
					}

					debugb1("ftrs %u total %u", anc, sizeof(device_features_t)+ftrs*sizeof(nx_uuid_t), ftrs, anc->total);

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
		else warn1("bsy");
		return FAIL;
	}

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

	event void AMSend.sendDone[uint8_t iface](message_t* msg, error_t err) {
		logger(err == SUCCESS ? LOG_DEBUG1: LOG_WARN1, "snt(%u)", err);
		call MessagePool.put(msg);
		m_busy = FALSE;
	}

	event message_t* Receive.receive[uint8_t iface](message_t* msg, void* payload, uint8_t len) {
		debugb1("rcv[%u]", payload, len, iface);
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
							signal DeviceAnnouncement.received(call AMPacket.source[iface](msg), da); // TODO a proper event?
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
							signal DeviceAnnouncement.received(call AMPacket.source[iface](msg), &da); // TODO a proper event?
						}
					}
					else {
						warn1("%04X ver %u", call AMPacket.source[iface](msg),
							((uint8_t*)payload)[1]); // Unknown version ... what to do?
					}
				break;

				case DEVA_QUERY: {
					info1("qry[%u] v%d %04X", iface, version, call AMPacket.source[iface](msg));
					announce(iface, version, call AMPacket.source[iface](msg));
				}
				break;

				case DEVA_DESCRIBE:
					info1("dsc[%u] %04X", iface, call AMPacket.source[iface](msg));
					describe(iface, version, call AMPacket.source[iface](msg));
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

	event void RadioChannel.setChannelDone() { }

	default command error_t DeviceFeatureUuid128.get[uint8_t fidx](uuid_t* uuid) { return ELAST; }

	default event void DeviceAnnouncement.received(am_addr_t addr, device_announcement_t* announcement) { }

	default command error_t AMSend.send[uint8_t iface](am_addr_t addr, message_t* msg, uint8_t len) { return EINVAL; }
	default command void* AMSend.getPayload[uint8_t iface](message_t* msg, uint8_t len) { return NULL; }
	default command uint8_t AMSend.maxPayloadLength[uint8_t iface]() { return 0; }

	default command am_addr_t AMPacket.address[uint8_t iface]() { return 0; }
	default command am_addr_t AMPacket.destination[uint8_t iface](message_t* amsg) { return 0; }
	default command am_addr_t AMPacket.source[uint8_t iface](message_t* amsg) { return 0; }

}
