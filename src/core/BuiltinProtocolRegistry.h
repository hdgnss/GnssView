#pragma once

namespace hdgnss {

class ProtocolDispatcher;
class NmeaProtocolPlugin;

class BuiltinProtocolRegistry {
public:
    static void registerProtocols(ProtocolDispatcher &dispatcher,
                                  NmeaProtocolPlugin &nmea);
};

}  // namespace hdgnss
