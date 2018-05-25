/**
 * @author Raido Pahtma
 * @license MIT
 **/
configuration DevACoordinateSyncC { }
implementation {

	components new DevACoordinateSyncP();

	components DeviceAnnouncementC;
	DevACoordinateSyncP.DeviceAnnouncement -> DeviceAnnouncementC.DeviceAnnouncement;

	components new TimerMilliC();
	DevACoordinateSyncP.Timer -> TimerMilliC;

	components MainC;
	DevACoordinateSyncP.Boot -> MainC;

	components DevicePositionParametersC;
	DevACoordinateSyncP.GetLatitude  -> DevicePositionParametersC.Latitude;
	DevACoordinateSyncP.GetLongitude -> DevicePositionParametersC.Longitude;
	DevACoordinateSyncP.GetFixType   -> DevicePositionParametersC.FixType;

	DevACoordinateSyncP.SetLatitude  -> DevicePositionParametersC.SetLatitude;
	DevACoordinateSyncP.SetLongitude -> DevicePositionParametersC.SetLongitude;
	DevACoordinateSyncP.SetFixType   -> DevicePositionParametersC.SetFixType;

	DevACoordinateSyncP.SaveLatitude  -> DevicePositionParametersC.SaveLatitude;
	DevACoordinateSyncP.SaveLongitude -> DevicePositionParametersC.SaveLongitude;
	DevACoordinateSyncP.SaveFixType   -> DevicePositionParametersC.SaveFixType;

}
