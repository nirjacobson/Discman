#ifndef CDDB_H_
#define CDDB_H_

#include <exception>
#include <cddb/cddb.h>

#include "cd_drive.h"
#include "disc.h"

class CDDB {
    public:
        CDDB();
        ~CDDB();

        struct InitializationException : public std::exception {
            const char* what() const throw() {
                return "Could not initialize libcddb.";
            }
        };

        struct DiscIDException : public std::exception {
            const char* what() const throw() {
                return "Could not generate CDDB disc ID.";
            }
        };

        struct QueryErrorException : public std::exception {
            const char* what() const throw() {
                return "Error performing CDDB query.";
            }
        };

        struct NoResultsFoundException : public std::exception {
            const char* what() const throw() {
                return "No disc results found.";
            }
        };

        Disc disc(const CDDrive& drive);

    private:
        cddb_conn_t* _conn;
};

#endif // CDDB_H_