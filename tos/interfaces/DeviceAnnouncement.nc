/**
 * Device announcement receive interface.
 *
 * @author Raido Pahtma
 * @license MIT
 **/
interface DeviceAnnouncement {

	event void received(am_addr_t addr, device_announcement_t* announcement);

}
