#include "BuiltinProtocolRegistry.h"

#include "ProtocolDispatcher.h"
#include "src/protocols/NmeaProtocolPlugin.h"

namespace hdgnss {

void BuiltinProtocolRegistry::registerProtocols(ProtocolDispatcher &dispatcher,
                                                NmeaProtocolPlugin &nmea) {
    dispatcher.registerPlugin(nmea);
}

}  // namespace hdgnss
