/**
 * @author Raido Pahtma
 * @license MIT
 **/
#include "DeviceParameters.h"
#include "tiny_haversine.h"
#include "sec_tmilli.h"
generic module DevACoordinateSyncP() {
	uses {
		interface DeviceAnnouncement;

		interface Timer<TMilli>;

		interface Get<int32_t> as GetLatitude;
		interface Set<int32_t> as SetLatitude;
		interface Set<int32_t> as SaveLatitude;

		interface Get<int32_t> as GetLongitude;
		interface Set<int32_t> as SetLongitude;
		interface Set<int32_t> as SaveLongitude;

		interface Get<char> as GetFixType;
		interface Set<char> as SetFixType;
		interface Set<char> as SaveFixType;

		interface Boot @exactlyonce();
	}
}
implementation {

	#define __MODUUL__ "devacs"
	#define __LOG_LEVEL__ (LOG_LEVEL_DevACoordinateSyncP & BASE_LOG_LEVEL)
	#include "log.h"

	int32_t m_avg_latitude = 0;
	int32_t m_avg_longitude = 0;
	uint16_t m_avg_count = 0; // Reset every 12 hours

	event void Boot.booted() {
		call Timer.startOneShot(SEC_TMILLI(3600UL)); // Inspect 1 hour after boot
	}

	event void Timer.fired() {
		call Timer.startOneShot(SEC_TMILLI(12*3600UL)); // Look again in 12 hours

		if(m_avg_count == 0) {
			return; // No data available
		}

		m_avg_count = 0; // Reset averaging

		if(call GetFixType.get() == 'F') {
			uint32_t distance = tiny_haversine(call GetLatitude.get(), call GetLongitude.get(), m_avg_latitude, m_avg_longitude);
			if(distance < 10000) { // 10000 - 10 km
				return; // Coords line up, keep using fixed coords.
			}
			else warn1("fob %"PRIu32, distance);
		}

		// Update and save location area estimate
		info1("%"PRIi32"; %"PRIi32, m_avg_latitude, m_avg_longitude);
		call SaveLatitude.set(m_avg_latitude);
		call SaveLongitude.set(m_avg_longitude);
		call SaveFixType.set('A');
	}

	event void DeviceAnnouncement.received(am_addr_t addr, device_announcement_t* announcement) {
		if(call GetFixType.get() == 'G') { // We have our own GPS
			return;
		}

		if(call GetFixType.get() == 'L') { // We have local positioning capability
			return;
		}

		// F type coordinates are not used for reference as we suspect they may
		// sometimes be configured incorrectly (reinstall) and this could under
		// the right circumstances end up poisoning other coordinates instead
		// G is GPS and relatively accurate
		// C is a special type of Fixed coordinates, simply assumed to be correct
		if((announcement->position_type == 'G')||(announcement->position_type == 'C')) {
			int64_t sum_latitude = m_avg_latitude*m_avg_count + announcement->latitude;
			int64_t sum_longitude = m_avg_longitude*m_avg_count + announcement->longitude;

			m_avg_count++;
			m_avg_latitude = sum_latitude / m_avg_count;
			m_avg_longitude = sum_longitude / m_avg_count;
		}
	}

}
