#include "mouse.h"

Mouse::Mouse()
    : _mouseDevice(std::getenv("DISCMAN_INPUT_DEVICE"))
    , _poller(new Poller(*this)) {
    _dispatcher.connect(sigc::mem_fun(*this, &Mouse::on_notification_from_poller));

}

Mouse::~Mouse() {
    delete _poller;
}

void Mouse::on_notification_from_poller() {
    _sig_click.emit();
}

Mouse::sig_click Mouse::signal_clicked() {
    return _sig_click;
}

Mouse::Poller::Poller(Mouse& mouse)
    : _mouse(mouse)
    , _exit(false)
    , _thread(&Mouse::Poller::poll, this) {

    _fd = open(_mouse._mouseDevice, O_RDONLY);
    if (_fd == -1) {
        throw new Mouse::CouldNotOpenMouseException();
    }

}
Mouse::Poller::~Poller() {
    _exit_lock.lock();
    _exit = true;
    _exit_lock.unlock();
    _thread.join();
}

void Mouse::Poller::poll() {
    struct input_event ie;

    while (true) {
        _exit_lock.lock();
        bool exit = _exit;
        _exit_lock.unlock();

        if (exit) return;

        size_t bytes = read(_fd, &ie, sizeof(ie));

        if (bytes > 0) {
            if (ie.type == EV_KEY) {
                if (ie.code == BTN_LEFT || ie.code == BTN_TOUCH) {
                    if (ie.value == 0) {
                        _mouse._dispatcher.emit();
                    }
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

