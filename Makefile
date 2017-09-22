
SDKDIR=/Developer/Platforms/iPhoneOS.platform
SDKBIN=$(SDKDIR)/Developer/usr/bin
SDKFRAMEWORKS=$(SDKDIR)/Developer/SDKs/iPhoneOS2.0.sdk/System/Library/Frameworks

CC=$(SDKBIN)/arm-apple-darwin9-gcc-4.0.1
CXX=$(SDKBIN)/arm-apple-darwin9-g++-4.0.1
STRIP=$(SDKBIN)/strip

CFLAGS+=-I$(SDKDIR)/Developer/SDKs/iPhoneOS2.0.sdk/usr/include
CFLAGS+=-I$(SDKDIR)/Developer/usr/lib/gcc/arm-apple-darwin9/4.0.1/include
CFLAGS+=-F/System/Library/Frameworks
CFLAGS+=-F$(SDKDIR)/Developer/SDKs/iPhoneOS2.0.sdk/System/Library/Frameworks
CFLAGS+=-F$(SDKDIR)/Developer/SDKs/iPhoneOS2.0.sdk/System/Library/PrivateFrameworks

CFLAGS+=-funroll-loops
CFLAGS+=-DMAC_OS_X_VERSION_MAX_ALLOWED=1050

LDFLAGS+=-L$(SDKDIR)/Developer/SDKs/iPhoneOS2.0.sdk/usr/lib
LDFLAGS+=-Z -Wl,-syslibroot,$(SDKFRAMEWORKS)
LDFLAGS+=$(SDKFRAMEWORKS)/IOKit.framework/Versions/A/IOKit
LDFLAGS+=$(SDKFRAMEWORKS)/CoreFoundation.framework/CoreFoundation

LDFLAGS+=-lobjc
CFLAGS+=-fpack-struct -fsigned-char -Wall -O3

CFLAGS += -DVERSION=\"0.9.1\"

PHONE = i

all:   yellowsn0w

yellowsn0w: yellowsn0w.o atprog.o
	$(CC) -o $@ $(LDFLAGS) yellowsn0w.o atprog.o
	$(STRIP) $@
	ldid -S $@

%.o:    %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

install: all
	scp yellowsn0w $(PHONE):/usr/bin/
	scp org.iphone-dev.yellowsn0w.plist $(PHONE):/System/Library/LaunchDaemons/

clean:
	rm -f *.o yellowsn0w *~


