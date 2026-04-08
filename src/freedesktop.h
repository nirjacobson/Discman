/**
 * @file freedesktop.h
 * @author Nir Jacobson
 * @date 2026-04-07
 */

#ifndef FREEDESKTOP_H
#define FREEDESKTOP_H

#include <string>

/// @brief Freedesktop D-Bus information
namespace Freedesktop {

    /// @brief The D-Bus object manager interface
    class ObjectManager {
        public:
            /// @brief Object manager signal names
            class Signals {
                public:
                    inline static const std::string InterfacesAdded = "InterfacesAdded";
                    inline static const std::string InterfacesRemoved = "InterfacesRemoved";
            };

            /// @brief Object manager method names
            class Methods {
                public:
                    inline static const std::string GetManagedObjects = "GetManagedObjects";
            };
            
            inline static const std::string Interface = "org.freedesktop.DBus.ObjectManager";
    };

}

#endif // FREEDESKTOP_H