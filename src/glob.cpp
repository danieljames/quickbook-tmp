/*=============================================================================
    Copyright (c) 2013 Daniel James

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include "glob.hpp"
#include <cassert>

namespace quickbook
{
    typedef boost::string_ref::const_iterator glob_iterator;

    // Crude, and not very efficient implementation of some very basic UTF-8
    // handling. Globs can only be ascii, but directory paths might not be,
    // so in that case muddle through to something vaguely sensible.
    //
    // None of this it is appropriate for general use. Proper unicode support
    // requires a proper unicode library which handles all sorts of things
    // that this doesn't, but I don't feel that I can justify adding a large
    // dependency.

    namespace {

        // Note: will return true for '\0'. This is usefull for null-terminated
        // strings, because the first character in a code point is also the
        // end of the previous character.
        bool is_first_char_in_codepoint(unsigned char c) {
            return (c & 0xc0) != 0x80;
        }

        // This only checks the encoding, not that the string is well-formed.
        // The point is to check that the filename is using UTF-8, not to
        // protect from mallicious strings.
        bool check_utf8_encoding(glob_iterator begin, glob_iterator end) {
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
        glob_iterator find_end_of_codepoint(glob_iterator begin, glob_iterator end) {
            if (begin == end) return begin;

            // Skip over first character.
            ++begin;

            // Skip over trailing characters.
            while (begin != end && !is_first_char_in_codepoint(*begin)) ++begin;

            return begin;
        }

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
        bool is_combining_character(glob_iterator begin, glob_iterator end) {
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

        // Does the range starts with a combining character?
        bool prefix_combining_character(glob_iterator begin, glob_iterator end) {
            return begin != end &&
                is_combining_character(begin, find_end_of_codepoint(begin, end));
        }

        // Move to the next character.
        glob_iterator find_end_of_char(glob_iterator begin, glob_iterator end) {
            begin = find_end_of_codepoint(begin, end);

            while (begin != end) {
                glob_iterator next = find_end_of_codepoint(begin, end);
                if (!is_combining_character(begin, next)) break;
                begin = next;
            }

            return begin;
        }
    }

    //
    // Glob implementation
    //

    void check_glob_range(glob_iterator&, glob_iterator);
    void check_glob_escape(glob_iterator&, glob_iterator);

    bool match_section(glob_iterator& pattern_begin, glob_iterator pattern_end,
            glob_iterator& filename_begin, glob_iterator& filename_end);
    bool match_range(glob_iterator& pattern_begin, glob_iterator pattern_end,
            glob_iterator& filename_begin, glob_iterator& filename_end);

    bool check_glob(boost::string_ref pattern)
    {
        bool is_glob = false;
        bool is_ascii = true;

        glob_iterator begin = pattern.begin();
        glob_iterator end = pattern.end();

        while (begin != end) {
            if (*begin < 32 || *begin > 127)
                is_ascii = false;

            switch(*begin) {
                case '\\':
                    check_glob_escape(begin, end);
                    break;

                case '[':
                    check_glob_range(begin, end);
                    is_glob = true;
                    break;

                case ']':
                    throw glob_error("uneven square brackets");

                case '?':
                    is_glob = true;
                    ++begin;
                    break;

                case '*':
                    is_glob = true;
                    ++begin;

                    if (begin != end && *begin == '*') {
                        throw glob_error("'**' not supported");
                    }
                    break;

                default:
                    ++begin;
            }
        }

        if (is_glob && !is_ascii)
            throw glob_error("invalid character, globs are ascii only");

        return is_glob;
    }

    void check_glob_range(glob_iterator& begin, glob_iterator end)
    {
        assert(begin != end && *begin == '[');
        ++begin;

        if (*begin == ']')
            throw glob_error("empty range");

        while (begin != end) {
            switch (*begin) {
                case '\\':
                    ++begin;

                    if (begin == end) {
                        throw glob_error("trailing escape");
                    }
                    else if (*begin == '\\' || *begin == '/') {
                        throw glob_error("contains escaped slash");
                    }

                    ++begin;
                    break;
                case '[':
                    // TODO: Allow?
                    throw glob_error("nested square brackets");
                case ']':
                    ++begin;
                    return;
                case '/':
                    throw glob_error("slash in square brackets");
                default:
                    ++begin;
            }
        }

        throw glob_error("uneven square brackets");
    }

    void check_glob_escape(glob_iterator& begin, glob_iterator end)
    {
        assert(begin != end && *begin == '\\');

        ++begin;

        if (begin == end) {
            throw glob_error("trailing escape");
        }
        else if (*begin == '\\' || *begin == '/') {
            throw glob_error("contains escaped slash");
        }

        ++begin;
    }

    bool glob(boost::string_ref const& pattern,
            boost::string_ref const& filename)
    {
        // If there wasn't this special case then '*' would match an
        // empty string.
        if (filename.empty()) return pattern.empty();

        glob_iterator filename_it = filename.begin();
        glob_iterator filename_end = filename.end();

        // TODO: Warn the user?
        if (!check_utf8_encoding(filename_it, filename_end))
            return false;

        glob_iterator pattern_it = pattern.begin();
        glob_iterator pattern_end = pattern.end();

        if (!match_section(pattern_it, pattern_end, filename_it, filename_end))
            return false;

        while (pattern_it != pattern_end) {
            assert(*pattern_it == '*');
            ++pattern_it;

            if (pattern_it == pattern_end) return true;

            // TODO: Error?
            if (*pattern_it == '*') return false;

            while (true) {
                if (filename_it == filename_end) return false;
                if (match_section(pattern_it, pattern_end, filename_it, filename_end))
                    break;
                filename_it = find_end_of_char(filename_it, filename_end);
            }
        }

        return filename_it == filename_end;
    }

    bool match_section(glob_iterator& pattern_begin, glob_iterator pattern_end,
            glob_iterator& filename_begin, glob_iterator& filename_end)
    {
        glob_iterator pattern_it = pattern_begin;
        glob_iterator filename_it = filename_begin;

        while (pattern_it != pattern_end && *pattern_it != '*') {
            if (filename_it == filename_end) return false;

            switch(*pattern_it) {
                case '*':
                    assert(false);
                    return false;
                case '[':
                    if (prefix_combining_character(filename_it, filename_end))
                        return false;
                    if (!match_range(pattern_it, pattern_end,
                                filename_it, filename_end))
                        return false;
                    break;
                case '?':
                    if (prefix_combining_character(filename_it, filename_end))
                        return false;
                    ++pattern_it;
                    filename_it = find_end_of_char(filename_it, filename_end);
                    break;
                case '\\':
                    ++pattern_it;
                    if (pattern_it == pattern_end) return false;
                    BOOST_FALLTHROUGH;
                default:
                    if (*pattern_it != *filename_it) return false;
                    ++pattern_it;
                    ++filename_it;
            }
        }

        if (pattern_it == pattern_end && filename_it != filename_end)
            return false;

        if (prefix_combining_character(filename_it, filename_end))
            return false;

        pattern_begin = pattern_it;
        filename_begin = filename_it;
        return true;
    }

    bool match_range(glob_iterator& pattern_begin, glob_iterator pattern_end,
            glob_iterator& filename_begin, glob_iterator& filename_end)
    {
        assert(pattern_begin != pattern_end && *pattern_begin == '[');
        ++pattern_begin;
        if (pattern_begin == pattern_end) return false;

        bool invert_match = false;
        bool matched = false;
        bool prevent_match = false;

        if (prefix_combining_character(filename_begin, filename_end)) {
            prevent_match = true;
        }

        if (*pattern_begin == '^') {
            invert_match = true;
            ++pattern_begin;
            if (pattern_begin == pattern_end) return false;
        }

        unsigned x = *filename_begin;

        // Search for a match
        while (true) {
            unsigned char first = *pattern_begin;
            ++pattern_begin;
            if (first == ']') break;
            if (pattern_begin == pattern_end) return false;

            if (first == '\\') {
                first = *pattern_begin;
                ++pattern_begin;
                if (pattern_begin == pattern_end) return false;
            }

            if (*pattern_begin != '-') {
                matched = matched || (first == x);
            }
            else {
                ++pattern_begin;
                if (pattern_begin == pattern_end) return false;

                unsigned char second = *pattern_begin;
                ++pattern_begin;
                if (second == ']') {
                    matched = matched || (first == x) || (x == '-');
                    break;
                }
                if (pattern_begin == pattern_end) return false;

                if (second == '\\') {
                    second = *pattern_begin;
                    ++pattern_begin;
                    if (pattern_begin == pattern_end) return false;
                }

                // TODO: What if second < first?
                matched = matched || (first <= x && x <= second);
            }
        }

        filename_begin = find_end_of_codepoint(filename_begin, filename_end);

        if (prefix_combining_character(filename_begin, filename_end))
        {
            filename_begin = find_end_of_char(filename_begin, filename_end);
            matched = false;
        }

        return !prevent_match && matched != invert_match;
    }

    std::size_t find_glob_char(boost::string_ref pattern,
            std::size_t pos)
    {
        // Weird style is because boost::string_ref's find_first_of
        // doesn't take a position argument.
        std::size_t removed = 0;

        while (true) {
            pos = pattern.find_first_of("[]?*\\");
            if (pos == boost::string_ref::npos) return pos;
            if (pattern[pos] != '\\') return pos + removed;
            pattern.remove_prefix(pos + 2);
            removed += pos + 2;
        }
    }

    std::string glob_unescape(boost::string_ref pattern)
    {
        std::string result;

        while (true) {
            std::size_t pos = pattern.find("\\");
            if (pos == boost::string_ref::npos) {
                result.append(pattern.data(), pattern.size());
                break;
            }

            result.append(pattern.data(), pos);
            ++pos;
            if (pos < pattern.size()) {
                result += pattern[pos];
                ++pos;
            }
            pattern.remove_prefix(pos);
        }

        return result;
    }
}
