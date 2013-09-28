/*=============================================================================
    Copyright (c) 2013 Daniel James

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include <boost/utility/string_ref.hpp>

// Crude, and not very efficient implementation of some very basic UTF-8
// handling. Globs can only be ascii, but directory paths might not be,
// so in that case muddle through to something vaguely sensible.
//
// None of this it is appropriate for general use. Proper unicode support
// requires a proper unicode library which handles all sorts of things
// that this doesn't, but I don't feel that I can justify adding a large
// dependency.

namespace quickbook
{
    typedef boost::string_ref::const_iterator string_ref_iterator;

    // This only checks the encoding, not that the string is well-formed.
    // The point is to check that the filename is using UTF-8, not to
    // protect from mallicious strings.
    bool check_utf8_encoding(string_ref_iterator, string_ref_iterator);

    inline bool check_utf8_encoding(boost::string_ref x) {
        return check_utf8_encoding(x.begin(), x.end());
    }

    // Move to the next code point.
    //
    // This assumes that correct UTF-8 is used. If it isn't it'll
    // get a weird result, but won't overrun the buffer.
    string_ref_iterator find_end_of_codepoint(string_ref_iterator,
            string_ref_iterator);

    // Does the range starts with a combining character?
    bool prefix_combining_character(string_ref_iterator, string_ref_iterator);

    // Move to the next character.
    string_ref_iterator find_end_of_char(string_ref_iterator,
            string_ref_iterator);
}

