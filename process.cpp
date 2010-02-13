/*=============================================================================
    Copyright (c) 2002 2004 2006 Joel de Guzman
    Copyright (c) 2004 Eric Niebler
    Copyright (c) 2010 Daniel James
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include "fwd.hpp"
#include "phrase_actions.hpp"
#include "block_actions.hpp"
#include "actions.hpp"
#include "state.hpp"
#include "parse_types.hpp"
#include "code.hpp"
#include "syntax_highlight.hpp"
#include "template.hpp"
#include "doc_info_actions.hpp"
#include "encoder.hpp"

namespace quickbook
{
    template <typename T>
    void process_action::operator()(T const& x) const
    {
        (*actions.state_.encoder)(actions.state_, process(actions.state_, x));
    }

    template <typename T>
    T const& process(quickbook::state&, T const& x)
    {
        return x;
    }

    template void process_action::operator()<formatted>(formatted const&) const;
    template void process_action::operator()<source_mode>(source_mode const&) const;
    template void process_action::operator()<macro>(macro const&) const;
    template void process_action::operator()<call_template>(call_template const&) const;
    template void process_action::operator()<anchor>(anchor const&) const;
    template void process_action::operator()<link>(link const&) const;
    template void process_action::operator()<simple_markup>(simple_markup const&) const;
    template void process_action::operator()<cond_phrase>(cond_phrase const&) const;
    template void process_action::operator()<break_>(break_ const&) const;
    template void process_action::operator()<image>(image const&) const;
    template void process_action::operator()<hr>(hr const&) const;
    template void process_action::operator()<paragraph>(paragraph const&) const;
    template void process_action::operator()<list>(list const&) const;
    template void process_action::operator()<begin_section>(begin_section const&) const;
    template void process_action::operator()<end_section>(end_section const&) const;
    template void process_action::operator()<heading>(heading const&) const;
    template void process_action::operator()<def_macro>(def_macro const&) const;
    template void process_action::operator()<variablelist>(variablelist const&) const;
    template void process_action::operator()<table>(table const&) const;
    template void process_action::operator()<xinclude>(xinclude const&) const;
    template void process_action::operator()<import>(import const&) const;
    template void process_action::operator()<include>(include const&) const;
    template void process_action::operator()<code>(code const&) const;
    template void process_action::operator()<define_template>(define_template const&) const;
    template void process_action::operator()<code_token>(code_token const&) const;
    template void process_action::operator()<char>(char const&) const;
    template void process_action::operator()<doc_info>(doc_info const&) const;
    template void process_action::operator()<doc_info_post>(doc_info_post const&) const;
    template void process_action::operator()<callout_link>(callout_link const&) const;
    template void process_action::operator()<callout_list>(callout_list const&) const;
}