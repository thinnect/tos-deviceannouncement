/**
 * Device announcement protocol wiring.
 *
 * @author Raido Pahtma
 * @license MIT
 **/
#include "DeviceAnnouncement.h"
configuration DeviceAnnouncementC {
	provides interface DeviceAnnouncement;
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

	components GlobalPoolC;
	DeviceAnnouncementP.MessagePool -> GlobalPoolC;

	components new AMSenderC(AMID_DEVICE_ANNOUNCEMENT);
	DeviceAnnouncementP.AMSend[DA_ANNOUNCEMENT_INTERFACE_ID] -> AMSenderC;
	DeviceAnnouncementP.AMPacket[DA_ANNOUNCEMENT_INTERFACE_ID] -> AMSenderC;

	components new AMReceiverC(AMID_DEVICE_ANNOUNCEMENT);
	DeviceAnnouncementP.Receive[DA_ANNOUNCEMENT_INTERFACE_ID] -> AMReceiverC;

}
