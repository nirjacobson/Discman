#include "cdplayer.h"

CDPlayer::CDPlayer(int argc, char **argv)
  : _audioOutput(AudioOutput<int16_t>::instance())
  , _track(0)
  , _poller(new Poller(*this)) {
  _audioOutput->producer(&_drive);
  _audioOutput->init();

  _app = Gtk::Application::create(argc, argv, "org.nirjacobson.cdplayer");

  _builder = Gtk::Builder::create();
  _builder->add_from_file("ui/cdplayer.glade");

  _discComponent = new DiscComponent(_builder);
  _discComponent->signal_eject_requested().connect(sigc::mem_fun(this, &CDPlayer::eject));
  _discComponent->signal_track_selected().connect(sigc::mem_fun(this, &CDPlayer::on_track_selected));

  _nowPlayingComponent = new NowPlayingComponent(_builder);
  _nowPlayingComponent->signal_prev().connect(sigc::mem_fun(this, &CDPlayer::on_prev));
  _nowPlayingComponent->signal_playpause().connect(sigc::mem_fun(this, &CDPlayer::on_playpause));
  _nowPlayingComponent->signal_stop().connect(sigc::mem_fun(this, &CDPlayer::on_stop));
  _nowPlayingComponent->signal_next().connect(sigc::mem_fun(this, &CDPlayer::on_next));

  _builder->get_widget("window", _window);
  _window->fullscreen();

  _dispatcher.connect(sigc::mem_fun(*this, &CDPlayer::on_notification_from_poller));
}

CDPlayer::~CDPlayer() {
  delete _window;
  delete _nowPlayingComponent;
  delete _discComponent;
  if (_poller) delete _poller;

  _audioOutput->destroy();
}

void CDPlayer::run() {
  _app->run(*_window);
}

void CDPlayer::notify() {
  _dispatcher.emit();
}

void CDPlayer::on_notification_from_poller() {
  _disc = CDDB().disc(_drive);
  _discComponent->set_disc(&_disc);
  _nowPlayingComponent->set_album(_disc.artist(), _disc.title());
  _nowPlayingComponent->set_state(NowPlayingComponent::State::Stopped);

  delete _poller;
  _poller = nullptr;
}

void CDPlayer::on_track_selected(unsigned int track) {
  play(track);
}

void CDPlayer::on_prev() {
  play(_track - 1);
}

void CDPlayer::on_playpause() {
  if (_nowPlayingComponent->get_state() == NowPlayingComponent::State::Playing) {
    pause();
  } else {
    play();
  }
}

void CDPlayer::on_stop() {
  stop();
}

void CDPlayer::on_next() {
  play(_track + 1);
}

bool CDPlayer::on_timeout() {
  if (_drive.done()) {
    eject();
  } else {
    if (_track != _drive.track()) {
      _track = _drive.track();
      _discComponent->set_selection(_track);
      _nowPlayingComponent->set_track(_disc.tracks().at(_track - 1), _track == 1, _track == _disc.tracks().size());
    }
    _nowPlayingComponent->set_seconds(_drive.elapsed());
  }

  return true;
}

void CDPlayer::play(unsigned int track) {
  if (_nowPlayingComponent->get_state() == NowPlayingComponent::State::Playing)
    stop();

  _track = track;

  _drive.track(track);
  _discComponent->set_selection(track);
  _nowPlayingComponent->set_track(_disc.tracks().at(track - 1), track == 1, track == _disc.tracks().size());
  play();
}

void CDPlayer::play() {
  if (_track == 0) {
    play(1);
  } else {
    _timerConnection = Glib::signal_timeout().connect(sigc::mem_fun(this, &CDPlayer::on_timeout), 250);
    _audioOutput->start();
    _nowPlayingComponent->set_state(NowPlayingComponent::State::Playing);
  }
}

void CDPlayer::pause() {
  _timerConnection.disconnect();
  _audioOutput->stop();
  _nowPlayingComponent->set_state(NowPlayingComponent::State::Paused);
}

void CDPlayer::stop() {
  _track = 0;

  _timerConnection.disconnect();
  _audioOutput->stop();
  _discComponent->clear_selection();
  _nowPlayingComponent->set_state(NowPlayingComponent::State::Stopped);
}

void CDPlayer::eject() {
  if (_nowPlayingComponent->get_state() == NowPlayingComponent::State::Playing)
    stop();

  _discComponent->set_disc(nullptr);
  _nowPlayingComponent->set_state(NowPlayingComponent::State::Disabled);
  _drive.eject();

  if (!_poller)
    _poller = new Poller(*this);
}

CDPlayer::Poller::Poller(CDPlayer& cdplayer)
  : _cdplayer(cdplayer)
  , _present(false)
  , _exit(false)
  , _thread(&CDPlayer::Poller::loop, this) {

}

CDPlayer::Poller::~Poller() {
  _exit_lock.lock();
  _exit = true;
  _exit_lock.unlock();

  _thread.join();
}

void CDPlayer::Poller::loop() {
  while (true) {
    _exit_lock.lock();
    bool exit = _exit;
    _exit_lock.unlock();

    if (exit) return;

    if (_cdplayer._drive.present()) {
      _cdplayer.notify();
      return;
    }
  }
}