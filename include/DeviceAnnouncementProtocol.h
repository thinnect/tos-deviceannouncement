/**
 * Device announcement protocol packet structures.
 *
 * @author Raido Pahtma
 * @license MIT
 **/
#ifndef DEVICEANNOUNCEMENTPROTOCOL_H_
#define DEVICEANNOUNCEMENTPROTOCOL_H_

#define AMID_DEVICE_ANNOUNCEMENT 0xDA

#define DEVICE_ANNOUNCEMENT_VERSION_V1 0x01
#define DEVICE_ANNOUNCEMENT_VERSION 0x02

#ifndef NESC
// This is a nesC header. If you wish to include it in a plain C project, it's
// your responsibility to provide the compatibility header below which provides
// the nx_* types (and other nesC specific stuff) so C will recognize them.
// NB! All nx_* types use big endian. You need to manually convert those in C.
# include "nesc_to_c_compat.h"
#endif

#include <UniversallyUniqueIdentifier.h>

enum DeviceAnnouncementHeaderEnum {
	DEVA_ANNOUNCEMENT    = 0x00,
	DEVA_DESCRIPTION     = 0x01,
	DEVA_FEATURES        = 0x02,

	DEVA_QUERY           = 0x10, // Ask for a device announcement packet
	DEVA_DESCRIBE        = 0x11, // Query device properties
	DEVA_LIST_FEATURES   = 0x12, // Query device features

	// Devices agree to specifically notify each other when either restarts
	// Not implemented for now ... possible future extension
	// DEVA_REGISTER            = 0x81, // Register for a unicast announcement that needs an ack
	// DEVA_DEREGISTER          = 0x82, // Stop expecting the announcement
	// DEVA_REGISTRATION_STATUS = 0x83  // Registration status (register/deregister ack)

	DEVA_ACKNOWLEDGEMENT = 0xAA, // Ack an announcement (when appropriate)
};

#pragma pack(push, 1)
typedef nx_struct device_announcement_v1 {
	nx_uint8_t header;             // 00
	nx_uint8_t version;            // Protocol version
	nx_uint8_t guid[8];            // Device EUI64
	nx_uint32_t boot_number;       // Current boot number

	nx_time64_t boot_time;         // Unix timestamp, seconds
	nx_uint32_t uptime;            // Uptime since boot, seconds
	nx_uint32_t lifetime;          // Total uptime since production, potentially lossy, seconds
	nx_uint32_t announcement;      // Announcement number since boot

	nx_uuid_t uuid;                // Application UUID (general feature set)

	nx_int32_t latitude;           // 1E6
	nx_int32_t longitude;          // 1E6
	nx_int32_t elevation;          // centimeters

	nx_time64_t ident_timestamp;   // Compilation time, unix timestamp, seconds

	nx_uint32_t feature_list_hash; // hash of feature UUIDs
} device_announcement_v1_t;
// 2+8+4+20+16+12+8+4=74
#pragma pack(pop)

#pragma pack(push, 1)
typedef nx_struct device_announcement_v2 {
	nx_uint8_t header;             // 00
	nx_uint8_t version;            // 02 Protocol version
	nx_uint8_t guid[8];            // Device EUI64
	nx_uint32_t boot_number;       // Current boot number

	nx_time64_t boot_time;         // Unix timestamp, seconds
	nx_uint32_t uptime;            // Uptime since boot, seconds
	nx_uint32_t lifetime;          // Total uptime since production, potentially lossy, seconds
	nx_uint32_t announcement;      // Announcement number since boot

	nx_uuid_t uuid;                // Application UUID (general feature set)

	nx_uint8_t position_type;      // F:fix, C:central(special F), G:gps, L:local, A:area, U:unknown.
	nx_int32_t latitude;           // 1E6
	nx_int32_t longitude;          // 1E6
	nx_int32_t elevation;          // centimeters

	nx_uint8_t radio_tech;         // 0:unknown(channel info invalid), 1:802.15.4, 2:BLE, 3:BLE+802.15.4(15.4 channel info), 4:802.11
	nx_uint8_t radio_channel;      // Current primary radio channel of the device - 0 unknown / 255 hopping

	nx_time64_t ident_timestamp;   // Compilation time, unix timestamp, seconds

	nx_uint32_t feature_list_hash; // hash of feature UUIDs
} device_announcement_v2_t;
// 2+8+4+20+16+RDO+13+8+4=75
#pragma pack(pop)

#pragma pack(push, 1)
typedef nx_struct device_request {
	nx_uint8_t header;             // 0x10 or 0x11
	nx_uint8_t version;            // Protocol version
} device_request_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef nx_struct device_feature_request {
	nx_uint8_t header;             // 0x12
	nx_uint8_t version;            // Protocol version
	nx_uint8_t offset;             // What feature to start from
} device_description_request_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef nx_struct device_description_v1 {
	nx_uint8_t header;             // 00
	nx_uint8_t version;            // Protocol version
	nx_uint8_t guid[8];            // Device EUI64
	nx_uint32_t boot_number;       // Current boot number

	nx_uuid_t platform;            // Platform UUID - platform is a combination of a BOARD and peripherals
	nx_uuid_t manufacturer;        // Manufacturer UUID
	nx_time64_t production;        // When the device was produced, unix timestamp, seconds

	nx_time64_t ident_timestamp;   // Compilation time, unix timestamp, seconds
	nx_uint8_t  sw_major_version;  // Firmware version, major.
	nx_uint8_t  sw_minor_version;  // Firmware version, minor.
	nx_uint8_t  sw_patch_version;  // Firmware version, patch.
} device_description_v1_t;
// 2+8+4+16+16+8+8+3=65
#pragma pack(pop)

#pragma pack(push, 1)
typedef nx_struct device_description_v2 {
	nx_uint8_t header;             // 00
	nx_uint8_t version;            // Protocol version
	nx_uint8_t guid[8];            // Device EUI64
	nx_uint32_t boot_number;       // Current boot number

	nx_uuid_t  platform;           // Platform UUID - platform is a combination of a BOARD and peripherals
	nx_uint8_t hw_major_version;   // Platform HW version, major.
	nx_uint8_t hw_minor_version;   // Platform HW version, minor.
	nx_uint8_t hw_assem_version;   // Platform HW version, assembly.

	nx_uuid_t manufacturer;        // Manufacturer UUID
	nx_time64_t production;        // When the device was produced, unix timestamp, seconds

	nx_time64_t ident_timestamp;   // Compilation time, unix timestamp, seconds
	nx_uint8_t  sw_major_version;  // Firmware version, major.
	nx_uint8_t  sw_minor_version;  // Firmware version, minor.
	nx_uint8_t  sw_patch_version;  // Firmware version, patch.
} device_description_v2_t;
// 2+8+4+16+3+16+8+8+3=68
#pragma pack(pop)

typedef device_announcement_v2_t device_announcement_t;

#pragma pack(push, 1)
typedef nx_struct device_features {
	nx_uint8_t header;             // 00
	nx_uint8_t version;            // Protocol version
	nx_uint8_t guid[8];            // Device EUI64
	nx_uint32_t boot_number;       // Current boot number

	nx_uint8_t total;              // Number of features/services on the device
	nx_uint8_t offset;             // Feature index offset
	nx_uuid_t features[];
} device_features_t;
// 2+8+4+2+0*16=12 -> 2+8+4+2+1*16=28 -> 2+8+4+2+6*16=108
#pragma pack(pop)

#ifdef NESC // TinyOS specific announcement module handling - TODO should not be here
	#define UQ_DEVICE_ANNOUNCEMENT_INTERFACE_ID unique("DeviceAnnouncementCommunicationsInterface")
	#define UQ_DEVICE_ANNOUNCEMENT_INTERFACE_COUNT uniqueCount("DeviceAnnouncementCommunicationsInterface")

	#define UQ_DEVICE_FEATURE_INDEX unique("DeviceFeatureUuidIndex")
	#define UQ_DEVICE_FEATURE_COUNT uniqueCount("DeviceFeatureUuidIndex")

	enum DeviceAnnouncementInterfaceEnum {
		DEVA_ANNOUNCEMENT_INTERFACE_ID = UQ_DEVICE_ANNOUNCEMENT_INTERFACE_ID
	};
#endif//NESC

#endif // DEVICEANNOUNCEMENTPROTOCOL_H_
