/**
 * @file component.h
 * @author Nir Jacobson
 * @date 2026-04-10
 */

 #ifndef COMPONENT_H
 #define COMPONENT_H

/// @brief Base class for all Application components.
class Component {
    protected:
        Component();
        virtual ~Component() = 0;
};

#endif // COMPONENT_H