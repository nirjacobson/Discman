#include "cddb.h"

CDDB::CDDB() {
    _conn = cddb_new();
    if (!_conn) {
        throw InitializationException();
    }

    cddb_set_email_address(_conn, "me@home");
    cddb_set_server_name(_conn, "freedb.freedb.org");
    cddb_set_server_port(_conn, 8880);
    cddb_http_disable(_conn);
}

CDDB::~CDDB() {
    cddb_destroy(_conn);
}

Disc CDDB::disc(const CDDrive& drive) {
    cddb_disc_t* cddb_disc = cddb_disc_new();

    for (unsigned int i = 0; i < drive.tracks(); i++) {
        cddb_track_t* cddb_track = cddb_track_new();

        cddb_track_set_frame_offset(cddb_track, drive.lba(1 + i));
        cddb_disc_add_track(cddb_disc, cddb_track);
    }

    cddb_disc_set_length(cddb_disc, drive.seconds());

    if (!cddb_disc_calc_discid(cddb_disc)) {
        throw DiscIDException();
    }

    int matches = cddb_query(_conn, cddb_disc);

    if (matches == -1) {
        throw QueryErrorException();
    }

    if (matches == 0) {
        throw NoResultsFoundException();
    }

    cddb_read(_conn, cddb_disc);

    Disc::Builder builder;
    builder
        .artist(cddb_disc_get_artist(cddb_disc) ? cddb_disc_get_artist(cddb_disc) : "")
        .title(cddb_disc_get_title(cddb_disc) ? cddb_disc_get_title(cddb_disc) : "")
        .genre(cddb_disc_get_genre(cddb_disc) ? cddb_disc_get_genre(cddb_disc) : "")
        .length(cddb_disc_get_length(cddb_disc))
        .year(cddb_disc_get_year(cddb_disc));

    for (unsigned int i = 0; i < drive.tracks(); i++) {
        cddb_track_t* cddb_track = cddb_disc_get_track(cddb_disc, i);

        builder.track(Track::Builder()
            .artist(cddb_track_get_artist(cddb_track) ? cddb_track_get_artist(cddb_track) : "")
            .title(cddb_track_get_title(cddb_track) ? cddb_track_get_title(cddb_track) : "")
            .length(cddb_track_get_length(cddb_track))
            .build());
    }

    cddb_disc_destroy(cddb_disc);

    return builder.build();
}