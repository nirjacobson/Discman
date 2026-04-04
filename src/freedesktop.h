#ifndef FREEDESKTOP_H
#define FREEDESKTOP_H

#include <string>

namespace Freedesktop {

    class ObjectManager {
        public:
            class Signals {
                public:
                    inline static const std::string InterfacesAdded = "InterfacesAdded";
                    inline static const std::string InterfacesRemoved = "InterfacesRemoved";
            };
            class Methods {
                public:
                    inline static const std::string GetManagedObjects = "GetManagedObjects";
            };
            inline static const std::string Interface = "org.freedesktop.DBus.ObjectManager";
    };

}

#endif // FREEDESKTOP_H