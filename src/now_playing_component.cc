#include "now_playing_component.h"

NowPlayingComponent::NowPlayingComponent(Glib::RefPtr<Gtk::Builder> builder)
    : _state(State::Stopped) {
    _album_art_button = builder->get_widget<Gtk::Button>("albumArt_button");
    _album_art_image = builder->get_widget<Gtk::Image>("albumArtImage");
    _track_title_label = builder->get_widget<Gtk::Label>("trackTitleLabel");
    _track_artist_label = builder->get_widget<Gtk::Label>("trackArtistLabel");
    _seek_scale = builder->get_widget<Gtk::Scale>("seekScale");
    _seek_scale = builder->get_widget<Gtk::Scale>("seekScale");
    _prev_button = builder->get_widget<Gtk::Button>("prev_button");
    _play_pause_button = builder->get_widget<Gtk::Button>("play_pause_button");
    _stop_button = builder->get_widget<Gtk::Button>("stop_button");
    _next_button = builder->get_widget<Gtk::Button>("next_button");
    _play_pause_image = builder->get_widget<Gtk::Image>("play_pauseImage");

    _album_art_button->set_has_frame(false);
    _album_art_image->clear();

    _prev_button->signal_clicked().connect(sigc::mem_fun(*this, &NowPlayingComponent::on_prev_button_clicked));
    _play_pause_button->signal_clicked().connect(sigc::mem_fun(*this, &NowPlayingComponent::on_playpause_button_clicked));
    _stop_button->signal_clicked().connect(sigc::mem_fun(*this, &NowPlayingComponent::on_stop_button_clicked));
    _next_button->signal_clicked().connect(sigc::mem_fun(*this, &NowPlayingComponent::on_next_button_clicked));
    _album_art_button->signal_clicked().connect(sigc::mem_fun(*this, &NowPlayingComponent::on_album_art_button_clicked));
}

NowPlayingComponent::~NowPlayingComponent() {
    delete _next_button;
    delete _stop_button;
    delete _play_pause_button;
    delete _prev_button;
    delete _seek_scale;
    delete _track_artist_label;
    delete _track_title_label;
    delete _album_art_image;
    delete _album_art_button;
}

void NowPlayingComponent::set_track(const DiscDB::Disc& disc, unsigned int track, const bool first, const bool last) {
    _track_title_label->set_text(disc.tracks()[track - 1].title());
    _track_artist_label->set_text(disc.artist());
    _seek_scale->get_adjustment()->set_lower(0);
    _seek_scale->get_adjustment()->set_upper(disc.track_length(track));
    _seek_scale->get_adjustment()->set_value(0);
    _prev_button->set_sensitive(!first);
    _next_button->set_sensitive(!last);
}

void NowPlayingComponent::set_state(const State state) {
    _state = state;

    if (state == State::Disabled || state == State::Stopped) {
        if (state == State::Disabled)
            _album_art_image->clear();
        _track_title_label->set_text("");
        _track_artist_label->set_text("");
        _seek_scale->get_adjustment()->set_value(0);
        _prev_button->set_sensitive(false);
        _next_button->set_sensitive(false);
    }

    _play_pause_button->set_sensitive(state != State::Disabled);
    _play_pause_image->set_from_resource((state != State::Playing) ? "/ui/icons/play.png" : "/ui/icons/pause.png");
    _stop_button->set_sensitive(state != State::Stopped && state != State::Disabled);
}

NowPlayingComponent::State NowPlayingComponent::get_state() const {
    return _state;
}

void NowPlayingComponent::set_seconds(const float seconds) {
    _seek_scale->get_adjustment()->set_value(seconds);
}

void NowPlayingComponent::set_album(const std::string& url) {
    try {
        AlbumArtProvider::AlbumArt albumArt = AlbumArtProvider::instance()->album_art(url, _album_art_image->get_width(), _album_art_image->get_height());
        _album_art_image->set(albumArt.art);
    } catch (const AlbumArtProvider::NotFoundException& e) {
        _album_art_image->clear();
    }
}

NowPlayingComponent::sig_button NowPlayingComponent::signal_button() {
    return _signal_button;
}

NowPlayingComponent::sig_album_art NowPlayingComponent::signal_album_art() {
    return _signal_album_art;
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

void NowPlayingComponent::on_album_art_button_clicked() {
    _signal_album_art.emit();
}