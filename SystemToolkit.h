#ifndef SYSTEMTOOLKIT_H
#define SYSTEMTOOLKIT_H

#ifdef WIN32
#define PATH_SEP '\\'
#elif defined(LINUX) or defined(APPLE)
#define PATH_SEP '/'
#endif

#include <string>
#include <list>

namespace SystemToolkit
{
    // get fixed length string (17 chars) YYYYMMDDHHmmssiii
    std::string date_time_string();

    // get the OS dependent path where to store settings
    std::string settings_path();

    // builds the OS dependent complete file name for a settings file
    std::string settings_prepend_path(const std::string& basefilename);

    // extract the base file name from a full URI (e.g. file:://home/me/toto.mpg -> toto)
    std::string base_filename(const std::string& uri);

    // true of file exists
    bool file_exists(const std::string& path);

    // true if directory could be created
    bool create_directory(const std::string& path);

}

#endif // SYSTEMTOOLKIT_H
