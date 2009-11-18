/*=============================================================================
    Copyright (c) 2002 2004 2006 Joel de Guzman
    Copyright (c) 2004 Eric Niebler
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_QUICKBOOK_GRAMMARS_HPP)
#define BOOST_SPIRIT_QUICKBOOK_GRAMMARS_HPP

#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_symbols.hpp>
#include <boost/spirit/include/classic_numerics.hpp>
#include <boost/spirit/include/classic_iterator.hpp>
#include <boost/spirit/include/qi_core.hpp>

namespace quickbook
{
    using namespace boost::spirit;

    template <typename Actions>
    struct phrase_grammar : classic::grammar<phrase_grammar<Actions> >
    {
        phrase_grammar(Actions& actions, bool& no_eols)
            : no_eols(no_eols), actions(actions) {}

        template <typename Scanner>
        struct definition
        {
            definition(phrase_grammar const& self);

            classic::rule<Scanner>
                            space, blank, comment, phrase, phrase_markup, image,
                            phrase_end, bold, italic, underline, teletype,
                            strikethrough, escape, url, common, funcref, classref,
                            memberref, enumref, macroref, headerref, conceptref, globalref,
                            anchor, link, hard_space, eol, inline_code, simple_format,
                            simple_bold, simple_italic, simple_underline,
                            simple_teletype, source_mode, template_,
                            quote, code_block, footnote, replaceable, macro,
                            dummy_block, cond_phrase, macro_identifier, template_args,
                            template_args_1_4, template_arg_1_4, brackets_1_4,
                            template_args_1_5, template_arg_1_5,
                            template_inner_arg_1_5, brackets_1_5
                            ;

            classic::rule<Scanner> const&
            start() const { return common; }
        };

        bool& no_eols;
        Actions& actions;
    };

    template <typename Actions>
    struct simple_phrase_grammar
    : public classic::grammar<simple_phrase_grammar<Actions> >
    {
        simple_phrase_grammar(Actions& actions)
            : actions(actions) {}

        template <typename Scanner>
        struct definition
        {
            definition(simple_phrase_grammar const& self);

            bool unused;
            classic::rule<Scanner> phrase, comment, dummy_block;
            phrase_grammar<Actions> common;

            classic::rule<Scanner> const&
            start() const { return phrase; }
        };

        Actions& actions;
    };

    template <typename Actions, bool skip_initial_spaces = false>
    struct block_grammar : classic::grammar<block_grammar<Actions> >
    {
        block_grammar(Actions& actions_)
            : actions(actions_) {}

        template <typename Scanner>
        struct definition
        {
            definition(block_grammar const& self);

            bool no_eols;

            classic::rule<Scanner>
                            start_, blocks, block_markup, code, code_line,
                            paragraph, space, blank, comment, headings, h, h1, h2,
                            h3, h4, h5, h6, hr, blurb, blockquote, admonition,
                            phrase, list, phrase_end, ordered_list, def_macro,
                            macro_identifier, table, table_row, variablelist,
                            varlistentry, varlistterm, varlistitem, table_cell,
                            preformatted, list_item, begin_section, end_section,
                            xinclude, include, hard_space, eol, paragraph_end,
                            template_, template_id, template_formal_arg,
                            template_body, identifier, dummy_block, import,
                            inside_paragraph, element_id, element_id_1_5;

            classic::symbols<>
                            paragraph_end_markups;

            phrase_grammar<Actions> common;

            classic::rule<Scanner> const&
            start() const { return start_; }
        };

        Actions&   actions;
    };

    template <typename Actions>
    struct doc_info_grammar
    : public classic::grammar<doc_info_grammar<Actions> >
    {
        doc_info_grammar(Actions& actions)
            : actions(actions) {}

        template <typename Scanner>
        struct definition
        {
            typedef classic::uint_parser<int, 10, 1, 2>  uint2_t;

            definition(doc_info_grammar const& self);
            bool unused;
            std::pair<std::string, std::string> name;
            std::pair<std::vector<std::string>, std::string> copyright;
            classic::rule<Scanner>
                            doc_info, doc_title, doc_version, doc_id, doc_dirname,
                            doc_copyright, doc_purpose,doc_category, doc_authors,
                            doc_author, comment, space, hard_space, doc_license,
                            doc_last_revision, doc_source_mode, phrase, quickbook_version;
            phrase_grammar<Actions> common;
            classic::symbols<> doc_types;

            classic::rule<Scanner> const&
            start() const { return doc_info; }
        };

        Actions& actions;
    };

    // TODO: Duplicate definition:
    
    struct code_snippet_actions;

    template <typename Iterator>
    struct python_code_snippet_grammar
        : qi::grammar<Iterator>
    {
        typedef code_snippet_actions actions_type;
  
        python_code_snippet_grammar(actions_type & actions);

        qi::rule<Iterator>
            start_, snippet, code_elements, escaped_comment,
            inline_callout, line_callout, ignore;
        qi::rule<Iterator, std::string()>
            identifier;

        actions_type& actions;
    };  

    template <typename Iterator>
    struct cpp_code_snippet_grammar
        : qi::grammar<Iterator>
    {
        typedef code_snippet_actions actions_type;
  
        cpp_code_snippet_grammar(actions_type & actions);

        qi::rule<Iterator>
            start_, snippet, code_elements, escaped_comment,
            inline_callout, line_callout, ignore;
        qi::rule<Iterator, std::string()>
            identifier;

        actions_type& actions;
    };
}

#endif // BOOST_SPIRIT_QUICKBOOK_GRAMMARS_HPP


