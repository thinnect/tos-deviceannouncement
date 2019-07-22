/**
 * Manipulation functions for UUID's.
 * @author Raido Pahtma
 * @license MIT
*/

#include "UniversallyUniqueIdentifier.h"

nx_uuid_t* hton_uuid(nx_uuid_t* net_uuid, uuid_t* host_uuid) {
	net_uuid->time_low = host_uuid->time_low;
	net_uuid->time_mid = host_uuid->time_mid;
	net_uuid->time_hi_and_version = host_uuid->time_hi_and_version;
	net_uuid->clock_seq_hi_and_reserved = host_uuid->clock_seq_hi_and_reserved;
	net_uuid->clock_seq_low = host_uuid->clock_seq_low;
	memcpy(net_uuid->node, host_uuid->node, 6);
	return net_uuid;
}

uuid_t* ntoh_uuid(uuid_t* host_uuid, nx_uuid_t* net_uuid) {
	host_uuid->time_low = net_uuid->time_low;
	host_uuid->time_mid = net_uuid->time_mid;
	host_uuid->time_hi_and_version = net_uuid->time_hi_and_version;
	host_uuid->clock_seq_hi_and_reserved = net_uuid->clock_seq_hi_and_reserved;
	host_uuid->clock_seq_low = net_uuid->clock_seq_low;
	memcpy(host_uuid->node, net_uuid->node, 6);
	return host_uuid;
}
