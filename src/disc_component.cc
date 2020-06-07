#include "disc_component.h"

DiscComponent::DiscComponent(Glib::RefPtr<Gtk::Builder> builder) {
  builder->get_widget("ejectButton", _ejectButton);
  builder->get_widget("albumLabel", _albumLabel);
  builder->get_widget("albumArtistLabel", _albumArtistLabel);
  builder->get_widget("tracksTreeView", _tracksTreeView);
  _tracksListStore = Glib::RefPtr<Gtk::ListStore>::cast_dynamic(
    builder->get_object("tracksListStore")
  );

  TracksListColumnRecord cols;
  _tracksTreeView->append_column_numeric("#", cols.numberColumn, "%d");
  _tracksTreeView->append_column("Title", cols.titleColumn);
  _tracksTreeView->append_column("Length", cols.lengthColumn);
  _tracksTreeView->get_column_cell_renderer(0)->set_alignment(1.0, 0.5);
  _tracksTreeView->get_column_cell_renderer(2)->set_alignment(1.0, 0.5);

  _tracksTreeView->get_column_cell_renderer(1)->set_property("ellipsize", Pango::ELLIPSIZE_END);
  _tracksTreeView->get_column(1)->set_property("expand", true);

  _ejectButton->signal_clicked().connect(sigc::mem_fun(this, &DiscComponent::on_eject_button_clicked));
  _tracksTreeView->signal_row_activated().connect(sigc::mem_fun(this, &DiscComponent::on_row_activated));
}

DiscComponent::~DiscComponent() {
  delete _tracksTreeView;
  delete _albumArtistLabel;
  delete _albumLabel;
  delete _ejectButton;
}

void DiscComponent::set_disc(const DiscDB::Disc* const disc) {
  _albumLabel->set_text(disc ? disc->title() : "No Disc");
  _albumArtistLabel->set_text(disc ? disc->artist() : "Please insert disc.");

  if (!!disc) {
    TracksListColumnRecord cols;

    for (unsigned int i = 0; i < disc->tracks().size(); i++) {
      auto row = *(_tracksListStore->append());
      row[cols.numberColumn] = i + 1;
      row[cols.titleColumn] = disc->tracks().at(i).title();

      std::stringstream lengthStream;
      const unsigned int seconds = disc->trackLength(i + 1);
      lengthStream << seconds / 60
                   << ":"
                   << std::setw(2) << std::setfill('0') << seconds % 60;

      row[cols.lengthColumn] = lengthStream.str();
    }
  } else {
    _tracksListStore->clear();
  }

  _ejectButton->set_sensitive(!!disc);
}

void DiscComponent::clear_selection() {
  _tracksTreeView->get_selection()->unselect_all();
}

void DiscComponent::set_selection(unsigned int track) {
  Gtk::TreeNodeChildren::iterator it = _tracksListStore->children().begin();
  for (unsigned int i = 0; i < track - 1; i++) it++;
  _tracksTreeView->get_selection()->select(*it);
}

DiscComponent::sig_eject_requested DiscComponent::signal_eject_requested() {
  return _signal_eject_requested;
}

DiscComponent::sig_track_selected DiscComponent::signal_track_selected() {
  return _signal_track_selected;
}

void DiscComponent::on_eject_button_clicked() {
  set_disc(nullptr);
  _signal_eject_requested.emit();
}

void DiscComponent::on_row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeView::Column*) {
  unsigned int track = _tracksListStore->get_iter(path)->get_value(TracksListColumnRecord().numberColumn);
  _signal_track_selected.emit(track);
}