#include "disc_component.h"

DiscComponent::DiscComponent(DriveManager& driveManager, Glib::RefPtr<Gtk::Builder> builder)
    : _driveManager(driveManager) {
    _ejectButtonStack = builder->get_widget<Gtk::Stack>("ejectButtonStack");
    _ejectButton = builder->get_widget<Gtk::Button>("ejectButton");
    _doubleEjectButton = builder->get_widget<Gtk::MenuButton>("doubleEjectButton");
    _doubleEjectButtonMenu = Gio::Menu::create();
    _doubleEjectButtonMenu->append("CD", "eject.cd");
    _doubleEjectButtonMenu->append("iPod", "eject.ipod");
    _doubleEjectButton->set_menu_model(_doubleEjectButtonMenu);

    _ejectActionGroup = Gio::SimpleActionGroup::create();
    _ejectActionGroup->add_action("cd", sigc::bind(sigc::mem_fun(*this, &DiscComponent::on_eject), DriveManager::Drive::DISC));
    _ejectActionGroup->add_action("ipod", sigc::bind(sigc::mem_fun(*this, &DiscComponent::on_eject), DriveManager::Drive::REMOVABLE));

    Gtk::Root* root = _doubleEjectButton->get_root();
    Gtk::Window* window = dynamic_cast<Gtk::Window*>(root);
    window->insert_action_group("eject", _ejectActionGroup);

    _ipodButton = builder->get_widget<Gtk::Button>("ipodButton");
    _albumLabel = builder->get_widget<Gtk::Label>("albumLabel");
    _albumArtistLabel = builder->get_widget<Gtk::Label>("albumArtistLabel");
    _tracksTreeView = builder->get_widget<Gtk::TreeView>("tracksTreeView");
    _tracksListStore = std::dynamic_pointer_cast<Gtk::ListStore>(builder->get_object("tracksListStore"));

    TracksListColumnRecord cols;
    _tracksTreeView->append_column_numeric("#", cols.numberColumn, "%d");
    _tracksTreeView->append_column("Title", cols.titleColumn);

    auto renderer = Gtk::make_managed<Gtk::CellRendererProgress>();
    int idx = _tracksTreeView->append_column("", *renderer);
    _tracksTreeView->get_column(idx - 1)->set_sizing(Gtk::TreeViewColumn::Sizing::FIXED);
    _tracksTreeView->get_column(idx - 1)->set_fixed_width(96);
    _tracksTreeView->get_column(idx - 1)->add_attribute(renderer->property_value(), cols.progressColumn);
    _tracksTreeView->get_column(idx - 1)->set_visible(false);

    _tracksTreeView->append_column("Length", cols.lengthColumn);

    _tracksTreeView->get_column_cell_renderer(0)->set_alignment(1.0, 0.5);
    _tracksTreeView->get_column_cell_renderer(3)->set_alignment(1.0, 0.5);

    _tracksTreeView->get_column_cell_renderer(1)->set_property("ellipsize", Pango::EllipsizeMode::END);
    _tracksTreeView->get_column(1)->set_property("expand", true);

    _ejectButton->signal_clicked().connect(sigc::mem_fun(*this, &DiscComponent::on_eject_button_clicked));
    _ipodButton->signal_clicked().connect(sigc::mem_fun(*this, &DiscComponent::on_ipod_button_clicked));
    _tracksTreeView->signal_row_activated().connect(sigc::mem_fun(*this, &DiscComponent::on_row_activated));
}

DiscComponent::~DiscComponent() {
    delete _tracksTreeView;
    delete _albumArtistLabel;
    delete _albumLabel;
    delete _ejectButton;
}

void DiscComponent::show_progress(const bool show) {
    _tracksTreeView->get_column(_tracksTreeView->get_n_columns() - 2)->set_visible(show);
}

void DiscComponent::set_disc(const DiscDB::Disc* const disc) {
    _albumLabel->set_text(disc ? disc->title() : "No Disc");
    _albumArtistLabel->set_text(disc ? disc->artist() : "Please insert disc.");

    if (!!disc) {
        const TracksListColumnRecord cols;

        for (unsigned int i = 0; i < disc->tracks().size(); i++) {
            auto row = *(_tracksListStore->append());
            row[cols.numberColumn] = i + 1;
            row[cols.titleColumn] = disc->tracks().at(i).title();
            row[cols.progressColumn] = 0;

            std::stringstream lengthStream;
            const unsigned int seconds = disc->trackLength(i + 1);
            lengthStream << seconds / 60
                         << ":"
                         << std::setw(2) << std::setfill('0')
                         << seconds % 60;

            row[cols.lengthColumn] = lengthStream.str();
        }
    } else {
        _tracksListStore->clear();
    }

    if (!disc) {
        show_progress(false);
    }
}

void DiscComponent::clear_selection() {
    _tracksTreeView->get_selection()->unselect_all();
}

void DiscComponent::set_selection(unsigned int track) {
    Glib::RefPtr<Gtk::TreeModel> model = _tracksTreeView->get_model();
    Gtk::TreeNodeChildren::iterator it = _tracksListStore->children().begin();
    for (unsigned int i = 0; i < track - 1; i++)
        it++;
    _tracksTreeView->get_selection()->select(model->get_path(it));
}

DiscComponent::sig_eject_requested DiscComponent::signal_eject_requested() {
    return _signal_eject_requested;
}

DiscComponent::sig_rip_requested DiscComponent::signal_rip_requested() {
    return _signal_rip_requested;
}


DiscComponent::sig_track_selected DiscComponent::signal_track_selected() {
    return _signal_track_selected;
}

void DiscComponent::update_track_progress(unsigned int track, unsigned int progress) {
    Glib::RefPtr<Gtk::TreeModel> model = _tracksTreeView->get_model();
    Gtk::TreeNodeChildren::iterator it = _tracksListStore->children().begin();
    for (unsigned int i = 0; i < track - 1; i++)
        it++;

    auto row = *it;
    const TracksListColumnRecord cols;
    row[cols.progressColumn] = progress;
}

void DiscComponent::show_ipod_button(const bool show) {
    _ipodButton->set_visible(show);
}

void DiscComponent::enable_ipod_button(const bool enable) {
    _ipodButton->set_sensitive(enable);
}

void DiscComponent::enable_eject_button(const bool enable) {
    _ejectButton->set_sensitive(enable);
}

void DiscComponent::show_double_eject_button(const bool show) {
    _ejectButtonStack->set_visible_child(*_ejectButtonStack->get_children()[show]);
}

void DiscComponent::on_eject_button_clicked() {

    DriveManager::Drive drive = _driveManager.isRemovablePresent()
        ? DriveManager::Drive::REMOVABLE
        : DriveManager::Drive::DISC;

    on_eject(drive);
}

void DiscComponent::on_eject(const DriveManager::Drive drive) {
    if (drive == DriveManager::Drive::DISC) {
        set_disc(nullptr);
    }
    _signal_eject_requested.emit(drive);
}

void DiscComponent::on_ipod_button_clicked() {
    Glib::RefPtr<Gtk::TreeSelection> selection = _tracksTreeView->get_selection();
    Gtk::TreeModel::iterator it = selection->get_selected();

    if (it) {
        Gtk::TreeModel::Path path = _tracksTreeView->get_model()->get_path(it);
        unsigned int track = _tracksListStore->get_iter(path)->get_value(TracksListColumnRecord().numberColumn);

        _signal_rip_requested.emit(track);
    } else {
        _signal_rip_requested.emit(0);
    }
}

void DiscComponent::on_row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeView::Column*) {
    unsigned int track = _tracksListStore->get_iter(path)->get_value(TracksListColumnRecord().numberColumn);
    _signal_track_selected.emit(track);
}