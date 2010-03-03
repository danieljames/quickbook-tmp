/*=============================================================================
    Copyright (c) 2002 2004 2006 Joel de Guzman
    Copyright (c) 2004 Eric Niebler
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include <map>
#include <boost/spirit/include/qi_core.hpp>
#include <boost/spirit/include/qi_symbols.hpp>
#include <boost/spirit/include/qi_attr.hpp>
#include <boost/spirit/include/qi_eoi.hpp>
#include <boost/spirit/include/qi_eol.hpp>
#include <boost/spirit/include/qi_eps.hpp>
#include <boost/spirit/include/qi_matches.hpp>
#include <boost/spirit/include/qi_uint.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/fusion/include/std_pair.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include "code.hpp"
#include "phrase.hpp"
#include "grammars.hpp"
#include "actions.hpp"
#include "template.hpp"
#include "parse_utils.hpp"
#include "misc_rules.hpp"
#include "rule_store.hpp"

BOOST_FUSION_ADAPT_STRUCT(
    quickbook::anchor,
    (std::string, id)
)

BOOST_FUSION_ADAPT_STRUCT(
    quickbook::link,
    (quickbook::formatted_type, type)
    (std::string, destination)
    (std::string, content)
)

BOOST_FUSION_ADAPT_STRUCT(
    quickbook::simple_markup,
    (char, symbol)
    (std::string, raw_content)
)

BOOST_FUSION_ADAPT_STRUCT(
    quickbook::break_,
    (quickbook::file_position, position)
)

BOOST_FUSION_ADAPT_STRUCT(
    quickbook::image,
    (quickbook::file_position, position)
    (std::string, image_filename)
    (quickbook::image::attribute_map, attributes)
)

BOOST_FUSION_ADAPT_STRUCT(
    quickbook::cond_phrase,
    (std::string, macro_id)
    (std::string, content)
)

BOOST_FUSION_ADAPT_STRUCT(
    quickbook::call_template,
    (quickbook::file_position, position)
    (bool, escape)
    (quickbook::template_symbol const*, symbol)
    (std::vector<quickbook::template_value>, args)
)

BOOST_FUSION_ADAPT_STRUCT(
    quickbook::template_value,
    (quickbook::file_position, position)
    (std::string, content)
)

BOOST_FUSION_ADAPT_STRUCT(
    quickbook::callout_link,
    (std::string, role)
    (std::string, identifier)
)

BOOST_FUSION_ADAPT_STRUCT(
    quickbook::unicode_char,
    (std::string, value)
)

namespace quickbook
{
    namespace qi = boost::spirit::qi;
    namespace ph = boost::phoenix;
    
    struct phrase_grammar::rules
    {
        rules(quickbook::actions& actions, bool& no_eols);
    
        quickbook::actions& actions;
        bool& no_eols;

        rule_store store_;
        qi::rule<iterator> common;        
    };

    phrase_grammar::phrase_grammar(quickbook::actions& actions, bool& no_eols)
        : phrase_grammar::base_type(start, "phrase")
        , rules_pimpl(new rules(actions, no_eols))
    {
        start = rules_pimpl->common;
    }

    phrase_grammar::~phrase_grammar() {}

    phrase_grammar::rules::rules(quickbook::actions& actions, bool& no_eols)
        : actions(actions), no_eols(no_eols)
    {
        qi::rule<iterator, std::string()>& phrase = store_.create();
        qi::rule<iterator>& macro = store_.create();
        qi::rule<iterator>& phrase_markup = store_.create();
        qi::rule<iterator, quickbook::code()>& code_block = store_.create();
        qi::rule<iterator, quickbook::code()>& inline_code = store_.create();
        qi::rule<iterator, quickbook::simple_markup(), qi::locals<char> >& simple_format = store_.create();
        qi::rule<iterator>& escape = store_.create();
        qi::rule<iterator, quickbook::callout_link()>& callout_link = store_.create();
        qi::rule<iterator, quickbook::cond_phrase()>& cond_phrase = store_.create();
        qi::rule<iterator, quickbook::image()>& image = store_.create();
        qi::rule<iterator, quickbook::link()>& url = store_.create();
        qi::rule<iterator, quickbook::link()>& link = store_.create();
        qi::rule<iterator, quickbook::anchor()>& anchor = store_.create();
        qi::symbols<char, quickbook::source_mode>& source_mode = store_.create();
        qi::rule<iterator, quickbook::formatted()>& formatted = store_.create();
        qi::rule<iterator, quickbook::formatted()>& footnote = store_.create();
        qi::rule<iterator, quickbook::call_template()>& call_template = store_.create();
        qi::rule<iterator, quickbook::break_()>& break_ = store_.create();
        qi::rule<iterator>& phrase_end = store_.create();

        phrase =
                qi::eps                         [actions.phrase_push]        
            >> *(   common
                |   comment
                |   (qi::char_ - phrase_end)    [actions.process]
                )
            >>  qi::eps                         [actions.phrase_pop]
            ;

        common =
                macro
            |   phrase_markup
            |   code_block                          [actions.process]
            |   inline_code                         [actions.process]
            |   simple_format                       [actions.process]
            |   escape
            |   comment
            ;

         macro =
            (   actions.macro                       // must not be followed by
            >>  !(qi::alpha | '_')                  // alpha or underscore
            )                                       [actions.process]
            ;

        phrase_markup =
            (   '['
            >>  (   callout_link
                |   cond_phrase
                |   image
                |   url
                |   link
                |   anchor
                |   source_mode
                |   formatted
                |   footnote
                |   call_template
                |   break_
                )
            >>  ']'
            )                                       [actions.process]
            ;

        code_block =
                (
                    "```"
                >>  position
                >>  qi::raw[*(qi::char_ - "```")]
                >>  "```"
                >>  qi::attr(true)
                )
            |   (
                    "``"
                >>  position
                >>  qi::raw[*(qi::char_ - "``")]
                >>  "``"
                >>  qi::attr(true)
                )
            ;

        inline_code =
                '`'
            >>  position
            >>  qi::raw
                [   *(  qi::char_ -
                        (   '`'
                        |   (eol >> eol)            // Make sure that we don't go
                        )                           // past a single block
                    )
                    >>  &qi::lit('`')
                ]
            >>  '`'
            >>  qi::attr(false)
            ;

        qi::rule<iterator>& simple_phrase_end = store_.create();

        simple_format %=
                qi::char_("*/_=")               [qi::_a = qi::_1]
            >>  qi::raw
                [   (   (   qi::graph               // A single char. e.g. *c*
                        >>  &(  qi::char_(qi::_a)
                            >>  (qi::space | qi::punct | qi::eoi)
                            )
                        )
                    |
                        (   qi::graph               // qi::graph must follow qi::lit(qi::_r1)
                        >>  *(  qi::char_ -
                                (   (qi::graph >> qi::lit(qi::_a))
                                |   simple_phrase_end // Make sure that we don't go
                                )                     // past a single block
                            )
                        >>  qi::graph               // qi::graph must precede qi::lit(qi::_r1)
                        >>  &(  qi::char_(qi::_a)
                            >>  (qi::space | qi::punct | qi::eoi)
                            )
                        )
                    )
                ]
            >> qi::omit[qi::char_(qi::_a)]
            ;

        simple_phrase_end = '[' | phrase_end;

        qi::rule<iterator, quickbook::break_()>& escape_break = store_.create();
        qi::rule<iterator, quickbook::formatted()>& escape_punct = store_.create();
        qi::rule<iterator, quickbook::formatted()>& escape_markup = store_.create();
        qi::rule<iterator, quickbook::unicode_char()>& escape_unicode = store_.create();

        escape =
            (   escape_break
            |   "\\ "                               // ignore an escaped char            
            |   escape_punct
            |   escape_unicode
            |   escape_markup                       
            )                                       [actions.process]
            ;
        
        escape_break =
                position
            >>  "\\n"
            >>  qi::attr(nothing())
            ;

        escape_punct =
                qi::attr(formatted_type(""))
            >>  '\\'
            >>  qi::repeat(1)[qi::punct]
            ;

        escape_markup =
                ("'''" >> -eol)
            >>  qi::attr("escape")
            >>  *(qi::char_ - "'''")
            >>  "'''"
            ;

        escape_unicode =
                "\\u"
            >>  qi::raw[qi::repeat(1,4)[qi::hex]]
            >>  qi::attr(nothing())
            ;

        // Don't use this, it's meant to be private.
        callout_link =
                "[callout]"
            >>  *~qi::char_(' ')
            >>  ' '
            >>  *~qi::char_(']')
            >>  qi::attr(nothing())
            ;

        cond_phrase =
                '?'
            >>  blank
            >>  macro_identifier
            >>  -phrase
            ;

        qi::rule<iterator, quickbook::image()>& image_1_4 = store_.create();
        qi::rule<iterator, quickbook::image()>& image_1_5 = store_.create();
        qi::rule<iterator, std::string()>& image_filename = store_.create();
        qi::rule<iterator, quickbook::image::attribute_map()>& image_attributes = store_.create();
        qi::rule<iterator, std::pair<std::string, std::string>()>& image_attribute = store_.create();
        qi::rule<iterator, std::string()>& image_attribute_key = store_.create();
        qi::rule<iterator, std::string()>& image_attribute_value = store_.create();

        image =
            (qi::eps(qbk_since(105u)) >> image_1_5) |
            (qi::eps(qbk_before(105u)) >> image_1_4);
        
        image_1_4 =
                position
            >>  '$'
            >>  blank
            >>  *(qi::char_ - phrase_end)
            >>  &qi::lit(']')
            ;
        
        image_1_5 =
                position
            >>  '$'
            >>  blank
            >>  image_filename
            >>  hard_space
            >>  image_attributes
            >>  &qi::lit(']')
            ;

        image_filename = qi::raw[
                +(qi::char_ - (qi::space | phrase_end | '['))
            >>  *(
                    +qi::space
                >>  +(qi::char_ - (qi::space | phrase_end | '['))
             )];

        image_attributes = *(image_attribute >> space);
        
        image_attribute =
                '['
            >>  image_attribute_key
            >>  space
            >>  image_attribute_value
            >>  ']'
            ;
            
        image_attribute_key = *(qi::alnum | '_');
        image_attribute_value = *(qi::char_ - (phrase_end | '['));

        url =
                '@'
            >>  qi::attr("url")
            >>  *(qi::char_ - (']' | qi::space))
            >>  (   &qi::lit(']')
                |   (hard_space >> phrase)
                )
            ;

        qi::symbols<char, formatted_type>& link_symbol = store_.create();

        link =
                link_symbol
            >>  hard_space
            >>  *(qi::char_ - (']' | qi::space))
            >>  (   &qi::lit(']')
                |   (hard_space >> phrase)
                )
            ;

        link_symbol.add
            ("link", formatted_type("link"))
            ("funcref", formatted_type("funcref"))
            ("classref", formatted_type("classref"))
            ("memberref", formatted_type("memberref"))
            ("enumref", formatted_type("enumref")) 
            ("macroref", formatted_type("macroref")) 
            ("headerref", formatted_type("headerref")) 
            ("conceptref", formatted_type("conceptref"))
            ("globalref", formatted_type("globalref"))
            ;

        anchor =
                '#'
            >>  blank
            >>  *(qi::char_ - phrase_end)
            >>  qi::attr(nothing())
            ;

        source_mode.add
            ("c++", quickbook::source_mode("c++"))
            ("python", quickbook::source_mode("python"))
            ("teletype", quickbook::source_mode("teletype"))
            ;

        qi::symbols<char, formatted_type>& format_symbol = store_.create();

        formatted = format_symbol >> blank >> phrase;

        format_symbol.add
            ("*", "bold")
            ("'", "italic")
            ("_", "underline")
            ("^", "teletype")
            ("-", "strikethrough")
            ("\"", "quote")
            ("~", "replaceable")
            ;

        footnote =
                "footnote"
            >>  qi::attr("footnote")
            >>  blank
            >>  phrase
            ;

        // Template call

        qi::rule<iterator, std::vector<quickbook::template_value>()>& template_args = store_.create();
        qi::rule<iterator, quickbook::template_value()>& template_arg_1_4 = store_.create();
        qi::rule<iterator>& brackets_1_4 = store_.create();
        qi::rule<iterator, quickbook::template_value()>& template_arg_1_5 = store_.create();
        qi::rule<iterator>& brackets_1_5 = store_.create();

        call_template =
                position
            >>  qi::matches['`']
            >>  (                                   // Lookup the template name
                    (&qi::punct >> actions.templates.scope)
                |   (actions.templates.scope >> hard_space)
                )
            >>  template_args
            >>  &qi::lit(']')
            ;

        template_args =
            qi::eps(qbk_before(105u)) >> -(template_arg_1_4 % "..") |
            qi::eps(qbk_since(105u)) >> -(template_arg_1_5 % "..");

        template_arg_1_4 =
            position >>
            qi::raw[+(brackets_1_4 | ~qi::char_(']') - "..")]
            ;

        brackets_1_4 =
            '[' >> +(brackets_1_4 | ~qi::char_(']') - "..") >> ']'
            ;

        template_arg_1_5 =
            position >>
            qi::raw[+(brackets_1_5 | '\\' >> qi::char_ | ~qi::char_("[]") - "..")]
            ;

        brackets_1_5 =
            '[' >> +(brackets_1_5 | '\\' >> qi::char_ | ~qi::char_("[]")) >> ']'
            ;

        break_ =
                position
            >>  "br"
            >>  qi::attr(nothing())
            ;

        phrase_end =
            ']' |
            qi::eps(ph::ref(no_eols)) >>
                eol >> eol                      // Make sure that we don't go
            ;                                   // past a single block, except
                                                // when preformatted.
    }

    struct simple_phrase_grammar::rules
    {
        rules(quickbook::actions& actions);

        quickbook::actions& actions;
        bool unused;
        phrase_grammar common;
        qi::rule<iterator> phrase;
    };

    simple_phrase_grammar::simple_phrase_grammar(quickbook::actions& actions)
        : simple_phrase_grammar::base_type(start, "simple_phrase")
        , rules_pimpl(new rules(actions))
        , start(rules_pimpl->phrase) {}

    simple_phrase_grammar::~simple_phrase_grammar() {}

    simple_phrase_grammar::rules::rules(quickbook::actions& actions)
        : actions(actions), unused(false), common(actions, unused)
    {
        phrase =
           *(   common
            |   comment
            |   (qi::char_ - ']')               [actions.process]
            )
            ;
    }
}
