/*=============================================================================
    Copyright (c) 2002 2004 2006 Joel de Guzman
    Copyright (c) 2004 Eric Niebler
    Copyright (c) 2005 Thomas Guest
    Copyright (c) 2013 Daniel James

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#if !defined(BOOST_QUICKBOOK_INCLUDE_PATHS_HPP)
#define BOOST_QUICKBOOK_INCLUDE_PATHS_HPP

// Classes and functions for dealing with the values from include, import and
// xinclude elements.

#include "fwd.hpp"
#include "values.hpp"
#include <set>
#include <string>
#include <boost/filesystem/path.hpp>

namespace quickbook
{
    struct path_parameter {
        // Will possibly add 'url' to this list later:
        enum path_type { path, glob };

        std::string value;
        path_type type;

        path_parameter(std::string const& value, path_type type) :
            value(value), type(type) {}
    };

    path_parameter check_path(value const& path, quickbook::state& state);

    struct quickbook_path
    {
        quickbook_path(fs::path const& x, fs::path const& y)
            : file_path(x), abstract_file_path(y) {}

        // The actual location of the file.
        fs::path file_path;

        // A machine independent representation of the file's
        // path - not unique per-file
        fs::path abstract_file_path;

        bool operator<(quickbook_path const& other) const;
    };

    std::set<quickbook_path> include_search(path_parameter const&,
            quickbook::state& state, string_iterator pos);
}

#endif
