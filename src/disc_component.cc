/**
 * @file disc_component.cc
 * @author Nir Jacobson
 * @date 2026-04-08
 */

 #include "disc_component.h"

DiscComponent::DiscComponent(DriveManager& drive_manager, Glib::RefPtr<Gtk::Builder> builder)
    : _drive_manager(drive_manager) {
    _eject_button_stack = builder->get_widget<Gtk::Stack>("ejectButtonStack");
    _eject_button = builder->get_widget<Gtk::Button>("ejectButton");
    _double_eject_button = builder->get_widget<Gtk::MenuButton>("doubleEjectButton");
    _double_eject_button_menu = Gio::Menu::create();
    _double_eject_button_menu->append("CD", "eject.cd");
    _double_eject_button_menu->append("iPod", "eject.ipod");
    _double_eject_button->set_menu_model(_double_eject_button_menu);

    _eject_action_group = Gio::SimpleActionGroup::create();
    _eject_action_group->add_action("cd", sigc::bind(sigc::mem_fun(*this, &DiscComponent::on_eject), DriveManager::Drive::Disc));
    _eject_action_group->add_action("ipod", sigc::bind(sigc::mem_fun(*this, &DiscComponent::on_eject), DriveManager::Drive::Removable));

    Gtk::Root* root = _double_eject_button->get_root();
    Gtk::Window* window = dynamic_cast<Gtk::Window*>(root);
    window->insert_action_group("eject", _eject_action_group);

    _ipod_button = builder->get_widget<Gtk::Button>("ipodButton");
    _album_label = builder->get_widget<Gtk::Label>("albumLabel");
    _album_artist_label = builder->get_widget<Gtk::Label>("albumArtistLabel");
    _tracks_tree_view = builder->get_widget<Gtk::TreeView>("tracksTreeView");
    _tracks_list_store = std::dynamic_pointer_cast<Gtk::ListStore>(builder->get_object("tracksListStore"));

    TracksListColumnRecord cols;
    _tracks_tree_view->append_column_numeric("#", cols.number_column, "%d");
    _tracks_tree_view->append_column("Title", cols.title_column);

    auto renderer = Gtk::make_managed<Gtk::CellRendererProgress>();
    int idx = _tracks_tree_view->append_column("", *renderer);
    _tracks_tree_view->get_column(idx - 1)->set_sizing(Gtk::TreeViewColumn::Sizing::FIXED);
    _tracks_tree_view->get_column(idx - 1)->set_fixed_width(96);
    _tracks_tree_view->get_column(idx - 1)->add_attribute(renderer->property_value(), cols.progress_column);
    _tracks_tree_view->get_column(idx - 1)->set_visible(false);

    _tracks_tree_view->append_column("Length", cols.length_column);

    _tracks_tree_view->get_column_cell_renderer(0)->set_alignment(1.0, 0.5);
    _tracks_tree_view->get_column_cell_renderer(3)->set_alignment(1.0, 0.5);

    _tracks_tree_view->get_column_cell_renderer(1)->set_property("ellipsize", Pango::EllipsizeMode::END);
    _tracks_tree_view->get_column(1)->set_property("expand", true);

    _eject_button->signal_clicked().connect(sigc::mem_fun(*this, &DiscComponent::on_eject_button_clicked));
    _ipod_button->signal_clicked().connect(sigc::mem_fun(*this, &DiscComponent::on_ipod_button_clicked));
    _tracks_tree_view->signal_row_activated().connect(sigc::mem_fun(*this, &DiscComponent::on_row_activated));
}

DiscComponent::~DiscComponent() {
    delete _tracks_tree_view;
    delete _album_artist_label;
    delete _album_label;
    delete _eject_button;
}

void DiscComponent::show_progress(const bool show) {
    _tracks_tree_view->get_column(_tracks_tree_view->get_n_columns() - 2)->set_visible(show);
}

void DiscComponent::set_disc(const DiscDB::Disc* const disc) {
    _album_label->set_text(disc ? disc->title() : "No Disc");
    _album_artist_label->set_text(disc ? disc->artist() : "Please insert disc.");

    if (!!disc) {
        const TracksListColumnRecord cols;

        for (unsigned int i = 0; i < disc->tracks().size(); i++) {
            auto row = *(_tracks_list_store->append());
            row[cols.number_column] = i + 1;
            row[cols.title_column] = disc->tracks().at(i).title();
            row[cols.progress_column] = 0;

            std::stringstream lengthStream;
            const unsigned int seconds = disc->track_length(i + 1);
            lengthStream << seconds / 60
                         << ":"
                         << std::setw(2) << std::setfill('0')
                         << seconds % 60;

            row[cols.length_column] = lengthStream.str();
        }
    } else {
        _tracks_list_store->clear();
    }

    if (!disc) {
        show_progress(false);
    }
}

void DiscComponent::clear_selection() {
    _tracks_tree_view->get_selection()->unselect_all();
}

void DiscComponent::set_selection(unsigned int track) {
    Glib::RefPtr<Gtk::TreeModel> model = _tracks_tree_view->get_model();
    Gtk::TreeNodeChildren::iterator it = _tracks_list_store->children().begin();
    for (unsigned int i = 0; i < track - 1; i++)
        it++;
    _tracks_tree_view->get_selection()->select(model->get_path(it));
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
    Glib::RefPtr<Gtk::TreeModel> model = _tracks_tree_view->get_model();
    Gtk::TreeNodeChildren::iterator it = _tracks_list_store->children().begin();
    for (unsigned int i = 0; i < track - 1; i++)
        it++;

    auto row = *it;
    const TracksListColumnRecord cols;
    row[cols.progress_column] = progress;
}

void DiscComponent::show_ipod_button(const bool show) {
    _ipod_button->set_visible(show);
}

void DiscComponent::enable_ipod_button(const bool enable) {
    _ipod_button->set_sensitive(enable);
}

void DiscComponent::enable_eject_button(const bool enable) {
    _eject_button->set_sensitive(enable);
}

void DiscComponent::show_double_eject_button(const bool show) {
    _eject_button_stack->set_visible_child(*_eject_button_stack->get_children()[show]);
}

void DiscComponent::on_eject_button_clicked() {

    DriveManager::Drive drive = _drive_manager.is_removable_present()
                                ? DriveManager::Drive::Removable
                                : DriveManager::Drive::Disc;

    on_eject(drive);
}

void DiscComponent::on_eject(const DriveManager::Drive drive) {
    if (drive == DriveManager::Drive::Disc) {
        set_disc(nullptr);
    }
    _signal_eject_requested.emit(drive);
}

void DiscComponent::on_ipod_button_clicked() {
    Glib::RefPtr<Gtk::TreeSelection> selection = _tracks_tree_view->get_selection();
    Gtk::TreeModel::iterator it = selection->get_selected();

    if (it) {
        Gtk::TreeModel::Path path = _tracks_tree_view->get_model()->get_path(it);
        unsigned int track = _tracks_list_store->get_iter(path)->get_value(TracksListColumnRecord().number_column);

        _signal_rip_requested.emit(track);
    } else {
        _signal_rip_requested.emit(0);
    }
}

void DiscComponent::on_row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeView::Column*) {
    unsigned int track = _tracks_list_store->get_iter(path)->get_value(TracksListColumnRecord().number_column);
    _signal_track_selected.emit(track);
}