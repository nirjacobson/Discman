#include "now_playing_component.h"

NowPlayingComponent::NowPlayingComponent(Glib::RefPtr<Gtk::Builder> builder)
    : _state(State::Stopped) {
    builder->get_widget("albumArtImage", _albumArtImage);
    builder->get_widget("trackTitleLabel", _trackTitleLabel);
    builder->get_widget("trackArtistLabel", _trackArtistLabel);
    builder->get_widget("seekScale", _seekScale);
    builder->get_widget("seekScale", _seekScale);
    builder->get_widget("prevButton", _prevButton);
    builder->get_widget("playPauseButton", _playPauseButton);
    builder->get_widget("stopButton", _stopButton);
    builder->get_widget("nextButton", _nextButton);
    builder->get_widget("playPauseImage", _playPauseImage);

    _albumArtImage->clear();

    _prevButton->signal_clicked().connect(sigc::mem_fun(this, &NowPlayingComponent::on_prev_button_clicked));
    _playPauseButton->signal_clicked().connect(sigc::mem_fun(this, &NowPlayingComponent::on_playpause_button_clicked));
    _stopButton->signal_clicked().connect(sigc::mem_fun(this, &NowPlayingComponent::on_stop_button_clicked));
    _nextButton->signal_clicked().connect(sigc::mem_fun(this, &NowPlayingComponent::on_next_button_clicked));
}

NowPlayingComponent::~NowPlayingComponent() {
    delete _nextButton;
    delete _stopButton;
    delete _playPauseButton;
    delete _prevButton;
    delete _seekScale;
    delete _trackArtistLabel;
    delete _trackTitleLabel;
    delete _albumArtImage;
}

void NowPlayingComponent::set_track(const DiscDB::Disc& disc, unsigned int track, const bool first, const bool last) {
    _trackTitleLabel->set_text(disc.tracks()[track - 1].title());
    _trackArtistLabel->set_text(disc.artist());
    _seekScale->get_adjustment()->set_lower(0);
    _seekScale->get_adjustment()->set_upper(disc.trackLength(track));
    _seekScale->get_adjustment()->set_value(0);
    _prevButton->set_sensitive(!first);
    _nextButton->set_sensitive(!last);
}

void NowPlayingComponent::set_state(const State state) {
    _state = state;

    if (state == State::Disabled || state == State::Stopped) {
        if (state == State::Disabled)
            _albumArtImage->clear();
        _trackTitleLabel->set_text("");
        _trackArtistLabel->set_text("");
        _seekScale->get_adjustment()->set_value(0);
        _prevButton->set_sensitive(false);
        _nextButton->set_sensitive(false);
    }

    _playPauseButton->set_sensitive(state != State::Disabled);
    _playPauseImage->set_from_resource((state != State::Playing) ? "/ui/icons/play.png" : "/ui/icons/pause.png");
    _stopButton->set_sensitive(state != State::Stopped && state != State::Disabled);
}

NowPlayingComponent::State NowPlayingComponent::get_state() const {
    return _state;
}

void NowPlayingComponent::set_seconds(const float seconds) {
    _seekScale->get_adjustment()->set_value(seconds);
}

void NowPlayingComponent::set_album(const std::string& artist, const std::string& title) {
    try {
        _albumArtImage->set(LastFM::album_art(artist, title, _albumArtImage->get_width(), _albumArtImage->get_height()));
    } catch (const LastFM::NotFoundException& e) {
        _albumArtImage->clear();
    }
}

NowPlayingComponent::sig_button NowPlayingComponent::signal_button() {
    return _signal_button;
}

void NowPlayingComponent::on_prev_button_clicked() {
    _signal_button.emit(Button::Previous);
}

void NowPlayingComponent::on_playpause_button_clicked() {
    _signal_button.emit(Button::PlayPause);
}

void NowPlayingComponent::on_stop_button_clicked() {
    _signal_button.emit(Button::Stop);
}

void NowPlayingComponent::on_next_button_clicked() {
    _signal_button.emit(Button::Next);
}