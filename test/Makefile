# Makefile for building device-announcement test-scenarios

CFLAGS += -std=c99 -g
CFLAGS += -I.
CFLAGS += -I../include
CFLAGS += -I../tos/deviceannouncement

CFLAGS += -DUUID_APPLICATION_BYTES='"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"'

CFLAGS += -Izoo/thinnect.node-platform/mocks
CFLAGS += -Izoo/thinnect.node-platform/include

CFLAGS += -Izoo/thinnect.cmsis-ext
CFLAGS += -Izoo/thinnect.cmsis-freertos/CMSIS_5/CMSIS/RTOS2/Include

CFLAGS += -Izoo/thinnect.mist-comm/include
CFLAGS += -Izoo/thinnect.mist-comm/include/compat
CFLAGS += -Izoo/thinnect.mist-comm/addrcache
CFLAGS += -Izoo/thinnect.mist-comm/am
CFLAGS += -Izoo/thinnect.mist-comm/cmsis
CFLAGS += -Izoo/thinnect.mist-comm/mock
CFLAGS += -Izoo/thinnect.mist-comm/test

CFLAGS += -Izoo/jtbr.endianness

CFLAGS += -Izoo/thinnect.device-signature/signature
CFLAGS += -Izoo/thinnect.device-signature/area
CFLAGS += -Izoo/thinnect.device-signature/area/file
CFLAGS += -Izoo/thinnect.device-signature/test

CFLAGS += -Izoo/lammertb.libcrc/include

CFLAGS += -Izoo/thinnect.lll/logging
CFLAGS += -DBASE_LOG_LEVEL=0xFFFF

CFLAGS += -DSW_MAJOR_VERSION=1
CFLAGS += -DSW_MINOR_VERSION=2
CFLAGS += -DSW_PATCH_VERSION=3
CFLAGS += -DIDENT_TIMESTAMP=0x0102030405060708

CFLAGS += -DUNITTEST=1

SRCS = test.c device_announcement.c device_features.c
SRCS += eui64.c
SRCS += mist_comm_am.c mist_comm_api.c mist_comm_rcv.c mist_comm_defer.c
SRCS += mist_comm_controller.c mist_comm_addrcache.c mist_comm_am_addrdisco.c
# mist-comm mocks
SRCS += mist_comm_mutex.c mist_comm_defer.c mist_comm_pool.c
SRCS += DeviceSignature.c SignatureArea.c

SRCS += loggers_std.c

SRCS += crcccitt.c

SRCS += cmsis_os2_mock.c

OBJS := $(SRCS:.c=.o)

VPATH += ../src
VPATH += zoo/thinnect.mist-comm/mock
VPATH += zoo/thinnect.mist-comm/am
VPATH += zoo/thinnect.mist-comm/api
VPATH += zoo/thinnect.mist-comm/control
VPATH += zoo/thinnect.mist-comm/cmsis
VPATH += zoo/thinnect.mist-comm/addrcache
VPATH += zoo/thinnect.mist-comm/addrdisco
VPATH += zoo/thinnect.device-signature/area/file
VPATH += zoo/thinnect.device-signature/signature
VPATH += zoo/thinnect.lll/logging
VPATH += zoo/lammertb.libcrc/src

# eui64.c
VPATH += zoo/thinnect.node-platform/common

all: test-app

test-app: $(OBJS)
	gcc $^ -o $@

%.o: %.c
	gcc -c -o $@ $< $(CFLAGS)

clean:
	rm -f *.o
	rm -f test-app
