# tos-deviceannouncement
Implementation of the DeviceAnnouncement protocol for the mist-comm API and
TinyOS ActiveMessage APIs.

# Device announcement protocol

The protocol is described in [PROTOCOL.md]().

# Protocol implementation

The main implementation is built on top of the
[mist-comm API](https://github.com/thinnect/mist-comm), it makes use of the
[LowLevelLogging API](https://github.com/thinnect/lll.git) and
[DeviceSignature API](https://github.com/thinnect/device-signature).

The DeviceAnnouncement module currently also contains the device feature
management API and implementation.

**This implementation currently does not support local listeners.**

The module needs intialization at boot, initialization should be performed
after DeviceSignature has been initialized and a message pool is available.
```
devf_init();
deva_init(&comms_pool);
```
It is then possible to register features and add announcers. Multiple announcers
can be added for cases where the device has several communication interfaces.


## TinyOS implementation

The TinyOS implementation should be considered deprecated, though it may be
gradually reworked to include the core functionality from the new main
implementation. It is in the tos subdirectory.

# Python implementation

There is a python library/tool for receiving and actively requesting
announcements over at [https://github.com/thinnect/python-moteannouncement]().

# Examples and tests
There is a unit-test like solution under [test/](). It runs through some basic
scenarios and checks that responses match manually crafted packets.
Travis has been configured to run this test-application.
