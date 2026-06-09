#ifndef __PROTOCOL_PROBE_APP_H
#define __PROTOCOL_PROBE_APP_H

#include <stdint.h>

#ifndef PROTOCOL_PROBE_PERIOD_MS
#define PROTOCOL_PROBE_PERIOD_MS        (1000U)
#endif

#ifndef PROTOCOL_PROBE_I2C_ADDR
#define PROTOCOL_PROBE_I2C_ADDR         (0x50U)
#endif

void ProtocolProbe_AppInit(void);
void ProtocolProbe_AppTask(void);

#endif
