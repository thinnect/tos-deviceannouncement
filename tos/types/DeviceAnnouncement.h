/**
 * TinyOS DeviceAnnouncement header.
 *
 * @author Raido Pahtma
 * @license MIT
 */
#ifndef DEVICEANNOUNCEMENT_H_
#define DEVICEANNOUNCEMENT_H_

#define UQ_DEVICE_ANNOUNCEMENT_INTERFACE_ID unique("DeviceAnnouncementCommunicationsInterface")
#define UQ_DEVICE_ANNOUNCEMENT_INTERFACE_COUNT uniqueCount("DeviceAnnouncementCommunicationsInterface")

#define UQ_DEVICE_FEATURE_INDEX unique("DeviceFeatureUuidIndex")
#define UQ_DEVICE_FEATURE_COUNT uniqueCount("DeviceFeatureUuidIndex")

enum DeviceAnnouncementInterfaceEnum {
	DEVA_ANNOUNCEMENT_INTERFACE_ID = UQ_DEVICE_ANNOUNCEMENT_INTERFACE_ID
};

#endif//DEVICEANNOUNCEMENT_H_
