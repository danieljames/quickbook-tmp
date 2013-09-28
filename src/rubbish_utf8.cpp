/*=============================================================================
    Copyright (c) 2013 Daniel James

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include "rubbish_utf8.hpp"

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
    namespace {

        // Note: will return true for '\0'. This is usefull for null-terminated
        // strings, because the first character in a code point is also the
        // end of the previous character.
        bool is_first_char_in_codepoint(unsigned char c) {
            return (c & 0xc0) != 0x80;
        }

    }

    // This only checks the encoding, not that the string is well-formed.
    // The point is to check that the filename is using UTF-8, not to
    // protect from mallicious strings.
    bool check_utf8_encoding(string_ref_iterator begin, string_ref_iterator end) {
        while (begin != end) {
            unsigned char c = *begin;
            ++begin;

            // A lookup table might be better.
            unsigned char length =
                (c & 0x80) == 0x00 ? 1 :    // 0xxxxxxx
                (c & 0xc0) == 0x80 ? 100 :  // 10xxxxxx (invalid)
                (c & 0xe0) == 0xc0 ? 2 :    // 110xxxxx
                (c & 0xf0) == 0xe0 ? 3 :    // 1110xxxx
                (c & 0xf8) == 0xf0 ? 4 :    // 11110xxx
                (c & 0xfc) == 0xf8 ? 5 :    // 111110xx
                (c & 0xfe) == 0xfc ? 6 :    // 1111110x
                100;                        // 1111111x (invalid)
            if (length == 100) return false;

            while (--length > 0) {
                if (begin == end) return false;
                if (is_first_char_in_codepoint(*begin)) return false;
                ++begin;
            }
        }

        return true;
    }

    // Move to the next code point.
    //
    // This assumes that correct UTF-8 is used. If it isn't it'll
    // get a weird result, but won't overrun the buffer.
    string_ref_iterator find_end_of_codepoint(string_ref_iterator begin, string_ref_iterator end) {
        if (begin == end) return begin;

        // Skip over first character.
        ++begin;

        // Skip over trailing characters.
        while (begin != end && !is_first_char_in_codepoint(*begin)) ++begin;

        return begin;
    }

    namespace {
        unsigned int combining_ranges[] = {
            // [0300, 0370)
            0xcc80, 0xcdb0,

            // [1DC0, 1E00)
            0xe1b780, 0xe1b7c0,

            // [20D0, 2100)
            0xe28390, 0xe283c0,

            // [FE20, FE30)
            0xefb8a0, 0xefb8b0
        };

        // pre: [begin, end) is a code point.
        // TODO: might be better to do this on a character by character basis.
        bool is_combining_character(string_ref_iterator begin, string_ref_iterator end) {
            if (end - begin < 2 || end - begin > 3) return false;

            unsigned int value = 0;
            for (;begin != end; ++begin) {
                value = (value << 8) + ((unsigned char) *begin);
            }

            int i = 0;
            const int length = sizeof(combining_ranges) / sizeof(unsigned int);

            for (;i < length; ++i) {
                if (value < combining_ranges[i]) break;
            }

            return i & 1;
        }
    }

    // Does the range starts with a combining character?
    bool prefix_combining_character(string_ref_iterator begin, string_ref_iterator end) {
        return begin != end &&
            is_combining_character(begin, find_end_of_codepoint(begin, end));
    }

    // Move to the next character.
    string_ref_iterator find_end_of_char(string_ref_iterator begin, string_ref_iterator end) {
        begin = find_end_of_codepoint(begin, end);

        while (begin != end) {
            string_ref_iterator next = find_end_of_codepoint(begin, end);
            if (!is_combining_character(begin, next)) break;
            begin = next;
        }

        return begin;
    }
}
