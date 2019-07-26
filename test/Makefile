
CFLAGS += -std=c99
CFLAGS += -I.
CFLAGS += -I../include
CFLAGS += -I../tos/deviceannouncement

CFLAGS += -Izoo/thinnect.mist-comm/include
CFLAGS += -Izoo/thinnect.mist-comm/include/compat
CFLAGS += -Izoo/thinnect.mist-comm/test

CFLAGS += -Izoo/jtbr.endianness

CFLAGS += -Izoo/thinnect.device-signature/signature
CFLAGS += -Izoo/thinnect.device-signature/area
CFLAGS += -Izoo/thinnect.device-signature/area/file
CFLAGS += -Izoo/thinnect.device-signature/test

CFLAGS += -Izoo/thinnect.lll/logging
CFLAGS += -DBASE_LOG_LEVEL=0xFFFF

CFLAGS += -DSW_MAJOR_VERSION=1
CFLAGS += -DSW_MINOR_VERSION=2
CFLAGS += -DSW_PATCH_VERSION=3
CFLAGS += -DIDENT_TIMESTAMP=0x0102030405060708

SRCS = test.c device_announcement.c device_features.c
SRCS += mist_comm_am.c mist_comm_api.c mist_comm_rcv.c
SRCS += DeviceSignature.c SignatureArea.c
SRCS += loggers_std.c

OBJS := $(SRCS:.c=.o)

VPATH += ../src
VPATH += zoo/thinnect.mist-comm/am
VPATH += zoo/thinnect.mist-comm/api
VPATH += zoo/thinnect.device-signature/area/file
VPATH += zoo/thinnect.device-signature/signature
VPATH += zoo/thinnect.lll/logging

all: test-app

test-app: $(OBJS)
	gcc $^ -o $@

%.o: %.c
	gcc -c -o $@ $< $(CFLAGS)

clean:
	rm *.o
	rm test-app
