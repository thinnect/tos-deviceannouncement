/**
 * Device announcement protocol wiring.
 *
 * @author Raido Pahtma
 * @license MIT
 **/
#include "DeviceAnnouncement.h"
configuration DeviceAnnouncementC {
	provides interface DeviceAnnouncement;
	uses {
		interface AMSend[uint8_t iface];
		interface AMPacket[uint8_t iface];
		interface Receive[uint8_t iface];
	}
}
implementation {

	components new DeviceAnnouncementP(UQ_DEVICE_ANNOUNCEMENT_INTERFACE_COUNT, UQ_DEVICE_FEATURE_COUNT);
	DeviceAnnouncement = DeviceAnnouncementP;

	components DeviceFeaturesC;
	DeviceAnnouncementP.DeviceFeatureUuid128 -> DeviceFeaturesC;

	components LocalIeeeEui64C;
	DeviceAnnouncementP.LocalIeeeEui64 -> LocalIeeeEui64C;

	components PlatformUUIDC;
	DeviceAnnouncementP.PlatformUuid128 -> PlatformUUIDC;

	components ApplicationUUIDC;
	DeviceAnnouncementP.ApplicationUuid128 -> ApplicationUUIDC;

	components RealTimeClockC;
	DeviceAnnouncementP.RealTimeClock -> RealTimeClockC;

	components CoordinatesC;
	DeviceAnnouncementP.GetGeo -> CoordinatesC.GetGeo;

	components BootsLifetimeC;
	DeviceAnnouncementP.BootNumber -> BootsLifetimeC.BootNumber;
	DeviceAnnouncementP.Lifetime -> BootsLifetimeC.Lifetime;
	DeviceAnnouncementP.Uptime -> BootsLifetimeC.Uptime;
	DeviceAnnouncementP.BootStable -> BootsLifetimeC;

	components UserSignatureAreaC;
	DeviceAnnouncementP.ProductionTime -> UserSignatureAreaC.ProductionTime;
	DeviceAnnouncementP.ManufacturerUuid128 -> UserSignatureAreaC.ManufacturerUuid128;

	components new TimerMilliC();
	DeviceAnnouncementP.Timer -> TimerMilliC;

	components RandomC;
	DeviceAnnouncementP.Random -> RandomC;

	components CrcC;
	DeviceAnnouncementP.Crc -> CrcC;

	components GlobalPoolC;
	DeviceAnnouncementP.MessagePool -> GlobalPoolC;

	DeviceAnnouncementP.AMSend = AMSend;
	DeviceAnnouncementP.AMPacket = AMPacket;
	DeviceAnnouncementP.Receive = Receive;

	components new AMSenderC(AMID_DEVICE_ANNOUNCEMENT);
	DeviceAnnouncementP.AMSend[DEVA_ANNOUNCEMENT_INTERFACE_ID] -> AMSenderC;
	DeviceAnnouncementP.AMPacket[DEVA_ANNOUNCEMENT_INTERFACE_ID] -> AMSenderC;

	components new AMReceiverC(AMID_DEVICE_ANNOUNCEMENT);
	DeviceAnnouncementP.Receive[DEVA_ANNOUNCEMENT_INTERFACE_ID] -> AMReceiverC;

}
