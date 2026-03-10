#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <RadioLib.h>

#define SLEEP_DURATION_SECONDS 30

#ifndef RADIOLIB_LORAWAN_BAND
#define RADIOLIB_LORAWAN_BAND    "EU868"
#endif

#ifndef RADIOLIB_LORAWAN_SUBBAND
#define RADIOLIB_LORAWAN_SUBBAND   0
#endif

#ifndef RADIOLIB_LORAWAN_JOIN_EUI  // Put your Join EUI here
#define RADIOLIB_LORAWAN_JOIN_EUI  0x0000000000000000
#endif


#ifndef RADIOLIB_LORAWAN_DEV_EUI   // Put your Dev EUI here
#define RADIOLIB_LORAWAN_DEV_EUI   0x70B...
#endif

#ifndef RADIOLIB_LORAWAN_APP_KEY   // Put your App Key here
#define RADIOLIB_LORAWAN_APP_KEY   0x54, 0xD3, 0x00, ...
#endif


#ifndef RADIOLIB_LORAWAN_NWK_KEY   // Put your Nwk Key here
#define RADIOLIB_LORAWAN_NWK_KEY   0xC4, 0xE2, 0x00, ...
#endif

#define LORAWAN_UPLINK_USER_PORT  2
#define LORAWAN_UPLINK_DATA_RATE  0

#endif
