#include "albumart_component.h"

AlbumArtComponent::AlbumArtComponent(Glib::RefPtr<Gtk::Builder> builder) {
    _doneButton = builder->get_widget<Gtk::Button>("albumArtDoneButton");
    _viewport = builder->get_widget<Gtk::Viewport>("albumArtViewport");
    _grid = builder->get_widget<Gtk::Grid>("albumArtGrid");

    _doneButton->signal_clicked().connect(sigc::mem_fun(*this, &AlbumArtComponent::on_done_button_clicked));
}

AlbumArtComponent::~AlbumArtComponent() {
    delete _grid;
    delete _viewport;
    delete _doneButton;
}

AlbumArtComponent::sig_done AlbumArtComponent::signal_done() {
    return _signal_done;
}

AlbumArtComponent::sig_art AlbumArtComponent::signal_art() {
    return _signal_art;
}

void AlbumArtComponent::set_albumarts(const std::vector<Spotify::AlbumArt>& arts, const int windowWidth) {
    Gtk::Grid* newGrid = new Gtk::Grid();
    newGrid->set_halign(Gtk::Align::CENTER);
    newGrid->set_valign(Gtk::Align::START);
    newGrid->set_column_spacing(SPACING);
    newGrid->set_row_spacing(SPACING);

    int cols = windowWidth / (ART_SIZE + SPACING);
    int rows = std::ceil(((float)arts.size())/((float)cols));
    
    for (int i = 0; i < cols; i++) {
        newGrid->insert_column(0);
    }

    for (int i = 0; i < rows; i++) {
        newGrid->insert_row(0);
    }

    bool done = false;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (((unsigned long)((i * cols) + j)) == arts.size()) {
                done = true;
                break;
            }
            Gtk::Button* button = Gtk::make_managed<Gtk::Button>();
            Gtk::Image* image = Gtk::make_managed<Gtk::Image>();


            image->set(arts[(i * cols) + j].art);
            button->set_has_frame(false);
            button->set_size_request(ART_SIZE, ART_SIZE);
            button->set_child(*image);

            const std::string url = arts[(i * cols) + j].url;
            button->signal_clicked().connect([this,url](){
                _signal_art.emit(url);
            });

            newGrid->attach(*button, j, i);
        }
        if (done) {
            break;
        }
    }

    _viewport->set_child(*newGrid);

    delete _grid;
    _grid = newGrid;
}

void AlbumArtComponent::on_done_button_clicked() {
    _signal_done.emit();
}