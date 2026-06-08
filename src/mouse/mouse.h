/**
 * @file mouse.h
 * @author Nir Jacobson
 * @date 2026-06-08
 */

#ifndef MOUSE_H
#define MOUSE_H

#include <fcntl.h>
#include <linux/input.h>
#include <thread>

#include <glibmm/dispatcher.h>

/// @brief Detects mouse/touchscreen presses.
class Mouse {

    public:
        /// @brief Thrown when the input device could not be opened.
        struct CouldNotOpenMouseException : public std::exception {
            const char* what() const throw() {
                return "Could not open mouse";
            }
        };

        /// @brief "Click" signal type.
        typedef sigc::signal<void(void)> sig_click;

        sig_click signal_clicked(); ///< Getter for ::_sig_click;

        /// @brief Mouse constructor.
        Mouse();

        /// @brief Mouse destructor.
        ~Mouse();

    private:
        class Poller;
        
        const char* _mouseDevice;           ///< The path to the mouse/touchscreen event interface.

        Glib::Dispatcher _dispatcher;       ///< Used to safely emit _sig_click.

        Poller* _poller;                    ///< When running, checks for mouse/touchscreen presses.
        sig_click _sig_click;               ///< Emitted when the mouse/touchscreen is pressed.

        void on_notification_from_poller(); ///< Called when _dispatcher is notified.

};

/// @brief The Poller uses a separate thread to poll an event interface for mouse/touchscreen presses.
class Mouse::Poller {
    public:
        /// @brief Poller constructor.
        /// @param [in] manager The parent Mouse.
        Poller(Mouse& mouse);

        /// @brief Poller destructor.
        ~Poller();

        void poll(); ///< The execution loop performed by the thread.

    private:
        Mouse& _mouse;              ///< The parent Mouse.
        bool _exit;                 ///< Stores a request to exist poll().

        int _fd;                    ///< Mouse/touchscreen event interface file handle.

        /// @brief Provides mutually exclusive access to the _exit member
        /// between the the application thread and the Poller thread.
        std::mutex _exit_lock;

        /// @brief The thread used to run poll().
        std::thread _thread;
};

#endif // MOUSE_H