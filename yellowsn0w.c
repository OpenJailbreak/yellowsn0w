/* yellowsn0w -- iPhone 3G unlock
 * by MuscleNerd and the iPhone Dev Team
 * http://blog.iphone-dev.org 
 * Copyright iPhone DevTeam 2008 
 * Leet Hax not for commercial use.
 * DO NOT SELL - STRICTLY NO COMMERCIAL USE.
 * CUSTOM PACKETS MUST NOT BE USED IN ANY OTHER THIRD PARTY PRODUCTS.
 * YOU HAVE BEEN WARNED.
 * Punishment: Monkeys coming out of your ass Bruce Almighty style.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <IOKit/IOKitLib.h>
#include <CoreFoundation/CFRunLoop.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "unlock_strings.h"

extern int atprog(char *cmds[], int num);

#define DETECTED_BB_RESET     0xE3FF8000
#define DETECTED_BB_RECOVERED 0xE3FF8001

char *whoami;
int isDaemon = 1;
int justCheck = 0;
int justReset = 0;
int useQuickMethod = 0;
int useAlternateMethod = 0;

void usage()
{
  fprintf(stderr, 
	  "yellowsn0w -- iPhone3G unlock (v%s)\n"\
	  "by MuscleNerd and the iPhone Dev Team\n"\
	  "http://blog.iphone-dev.org\n\n"\
	  "Usage: %s [options]\n\n"	\
	  "  Options:\n"\
	  "     -s  Single-shot mode\n"\
	  "     -c  Just check for compatibility\n"\
	  "     -r  Just reset the baseband\n"\
	  "     -q  Use quicker method (only for some SIM cards)\n"\
	  "     -a  Use alternate method (only for some SIM cards)\n"\
	  "\n", VERSION, whoami);
  exit(0);
}

void ResetBaseband(io_connect_t conn)
{
  int result = IOConnectCallScalarMethod(conn, 0, 0, 0, 0, 0);
  if (result) {
    fprintf(stderr, "Couldn't reset baseband (%d)\n", result);
    exit(1);
  }
}

extern char *BasebandVersion(void);
int CheckCompatibility(void) {
  char *actual   = BasebandVersion();
  char *required = "02.28.00";
  if (strcmp(actual, required)) {
    fprintf(stderr, "ERROR!  %s works only for baseband \"%s\", but this baseband is \"%s\"\n", whoami, required, actual);
    return 0;  // bad
  } else {
    return 1;  // good
  }
}

void DeviceNotification(void *refCon, io_service_t service, unsigned int messageType, void *messageArgument)
{
  if (messageType==DETECTED_BB_RECOVERED) {
    if (!CheckCompatibility())
      exit(0); // bad (but don't exit(1) else we'll be respawned by launchdaemon)

    sleep(5);
    atprog(&unlock_strings[0], 1);

    atprog(&unlock_strings[1], NUM_CMDS-1-3);
    if (!useQuickMethod) {
      sleep(60);
    }

    if (useAlternateMethod)
      atprog(&unlock_strings[NUM_CMDS-3], 1);
    else
      atprog(&unlock_strings[NUM_CMDS-3], 3);

    if (!isDaemon) {
      exit(0);
    }
  } else {
    fprintf(stderr, "Unknown message (0x%08x)\n", messageType);
  }
}


int main(int argc, char *argv[])
{
  whoami = rindex(argv[0], '/');
  whoami = (whoami == NULL) ? argv[0] : whoami+1;

  int opt;
  while ((opt = getopt(argc, argv, "scrqah"))!=-1) {
    switch (opt) {
    case 's':
      isDaemon = 0;
      break;
    case 'c':
      justCheck = 1;
      break;
    case 'r':
      justReset = 1;
      break;
    case 'q':
      useQuickMethod = 1;
      break;
    case 'a':
      useAlternateMethod = 1;
      break;
    case 'h':
      usage();
      break;
    }
  }
  argc -= optind;
  argv += optind;

  kern_return_t result;

  char *serviceName = "AppleBaseband";
  CFMutableDictionaryRef matchingDict = IOServiceMatching(serviceName);
  io_service_t service = IOServiceGetMatchingService(kIOMasterPortDefault, matchingDict);
  if (!service) {
    fprintf(stderr, "Couldn't find %s\n", serviceName);
    exit(1);
  }

  io_connect_t conn;
  result = IOServiceOpen(service, mach_task_self(), 0, &conn);
  if (result) {
    fprintf(stderr, "Couldn't open service (%d)\n", result);
    exit(1);
  }

  if (justCheck) {
    ResetBaseband(conn);
    if (CheckCompatibility())
      exit(0); // good
    else
      exit(1); // bad
  }

  if (justReset) {
    ResetBaseband(conn);
    exit(0);
  }
    
  IONotificationPortRef notificationObject = IONotificationPortCreate(kIOMasterPortDefault);
  if (!notificationObject) {
    fprintf(stderr, "Couldn't create notification port\n");
    exit(1);
  }

  io_object_t notification;
  int refCon = 0;
  result = IOServiceAddInterestNotification(notificationObject, service, kIOGeneralInterest,
					    DeviceNotification, &refCon, &notification);
  if (result) {
    fprintf(stderr, "Couldn't add interest notification (%d)\n", result);
    exit(1);
  }

  CFRunLoopSourceRef notificationRunLoopSource = IONotificationPortGetRunLoopSource(notificationObject);
  CFRunLoopAddSource(CFRunLoopGetCurrent(), notificationRunLoopSource, kCFRunLoopCommonModes);

  ResetBaseband(conn);
  CFRunLoopRun();

  exit(1); // shouldn't be reached
}

