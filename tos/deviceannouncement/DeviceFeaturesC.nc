/**
 * Device feature wiring hub.
 *
 * @author Raido Pahtma
 * @license MIT
 **/
configuration DeviceFeaturesC {
	provides interface GetStruct<uuid_t> as DeviceFeatureUuid128[uint8_t fidx];
	uses interface GetStruct<uuid_t> as SubDeviceFeatureUuid128[uint8_t fidx];
}
implementation {

	DeviceFeatureUuid128 = SubDeviceFeatureUuid128;

}
