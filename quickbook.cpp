/*=============================================================================
    Copyright (c) 2002 2004 2006 Joel de Guzman
    Copyright (c) 2004 Eric Niebler
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include <stdexcept>
#include <fstream>
#include <iostream>
#include <vector>
#include <boost/program_options.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/ref.hpp>
#include "fwd.hpp"
#include "quickbook.hpp"
#include "state.hpp"
#include "actions.hpp"
#include "grammars.hpp"
#include "post_process.hpp"
#include "utils.hpp"
#include "input_path.hpp"
#include "doc_info.hpp"

#if (defined(BOOST_MSVC) && (BOOST_MSVC <= 1310))
#pragma warning(disable:4355)
#endif

#define QUICKBOOK_VERSION "Quickbook Version 1.5.1"

namespace quickbook
{
    namespace qi = boost::spirit::qi;
    namespace fs = boost::filesystem;
    tm* current_time; // the current time
    tm* current_gm_time; // the current UTC time
    bool debug_mode; // for quickbook developers only
    bool ms_errors = false; // output errors/warnings as if for VS
    std::vector<std::string> include_path;

    ///////////////////////////////////////////////////////////////////////////
    //
    //  Parse a file
    //
    ///////////////////////////////////////////////////////////////////////////
    int
    parse(char const* filein_, state& state_, bool ignore_docinfo)
    {
        using std::cerr;
        using std::vector;
        using std::string;

        std::string storage;
        int err = detail::load(filein_, storage);
        if (err != 0) {
            ++state_.error_count;
            return err;
        }

        iterator first(storage.begin(), storage.end(), filein_);
        iterator last(storage.end(), storage.end());
        iterator start = first;

        doc_info info;
        actions actor(state_);
        doc_info_grammar l(actor);
        bool success = parse(first, last, l, info);

        if (success || ignore_docinfo)
        {
            if(!success) first = start;

            info.ignore = ignore_docinfo;

            actor.process(info);

            block_grammar g(actor);
            success = parse(first, last, g);
            if (success && first == last)
            {
                actor.process(doc_info_post(info));
            }
        }
        else {
            file_position const pos = first.get_position();
            detail::outerr(pos.file,pos.line)
                << "Doc Info error near column " << pos.column << ".\n";
        }

        if (!success || first != last)
        {
            file_position const pos = first.get_position();
            detail::outerr(pos.file,pos.line)
                << "Syntax Error near column " << pos.column << ".\n";
            ++state_.error_count;
        }
        
        if(state_.error_count)
        {
            detail::outerr(filein_)
                << "Error count: " << actor.state_.error_count << ".\n";
        }

        return state_.error_count ? 1 : 0;
    }

    static int
    parse(char const* filein_, fs::path const& outdir, string_stream& out, bool ignore_docinfo = false)
    {
        quickbook::state state(filein_, outdir, out, create_encoder("boostbook"));
        bool r = parse(filein_, state);
        if (state.section_level != 0)
            detail::outwarn(filein_)
                << "Warning missing [endsect] detected at end of file."
                << std::endl;
        return r;
    }

    static int
    parse(
        char const* filein_
      , char const* fileout_
      , int indent
      , int linewidth
      , bool pretty_print)
    {
        int result = 0;
        std::ofstream fileout(fileout_);
        fs::path outdir = fs::path(fileout_, fs::native).branch_path();
        if (outdir.empty())
            outdir = ".";
        if (pretty_print)
        {
            string_stream buffer;
            result = parse(filein_, outdir, buffer);
            if (result == 0)
            {
                result = post_process(buffer.str(), fileout, indent, linewidth);
            }
        }
        else
        {
            string_stream buffer;
            result = parse(filein_, outdir, buffer);
            fileout << buffer.str();
        }
        return result;
    }
}

///////////////////////////////////////////////////////////////////////////
//
//  Main program
//
///////////////////////////////////////////////////////////////////////////
int
main(int argc, char* argv[])
{
    try
    {
        using boost::program_options::options_description;
        using boost::program_options::variables_map;
        using boost::program_options::store;
        using boost::program_options::parse_command_line;
        using boost::program_options::command_line_parser;
        using boost::program_options::notify;
        using boost::program_options::value;
        using boost::program_options::positional_options_description;

        // First thing, the filesystem should record the current working directory.
        boost::filesystem::initial_path();

        options_description desc("Allowed options");
        desc.add_options()
            ("help", "produce help message")
            ("version", "print version string")
            ("no-pretty-print", "disable XML pretty printing")
            ("indent", value<int>(), "indent spaces")
            ("linewidth", value<int>(), "line width")
            ("input-file", value<quickbook::detail::input_path>(), "input file")
            ("output-file", value<quickbook::detail::input_path>(), "output file")
            ("debug", "debug mode (for developers)")
            ("ms-errors", "use Microsoft Visual Studio style error & warn message format")
            ("include-path,I", value< std::vector<quickbook::detail::input_path> >(), "include path")
        ;

        positional_options_description p;
        p.add("input-file", -1);

        variables_map vm;
        int indent = -1;
        int linewidth = -1;
        bool pretty_print = true;
        store(command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
        notify(vm);

        if (vm.count("help"))
        {
            std::cout << desc << "\n";
            return 0;
        }

        if (vm.count("version"))
        {
            std::cout << QUICKBOOK_VERSION << std::endl;
            return 0;
        }

        if (vm.count("ms-errors"))
            quickbook::ms_errors = true;

        if (vm.count("no-pretty-print"))
            pretty_print = false;

        if (vm.count("indent"))
            indent = vm["indent"].as<int>();

        if (vm.count("linewidth"))
            linewidth = vm["linewidth"].as<int>();

        if (vm.count("debug"))
        {
            static tm timeinfo;
            timeinfo.tm_year = 2000 - 1900;
            timeinfo.tm_mon = 12 - 1;
            timeinfo.tm_mday = 20;
            timeinfo.tm_hour = 12;
            timeinfo.tm_min = 0;
            timeinfo.tm_sec = 0;
            timeinfo.tm_isdst = -1;
            mktime(&timeinfo);
            quickbook::current_time = &timeinfo;
            quickbook::current_gm_time = &timeinfo;
            quickbook::debug_mode = true;
        }
        else
        {
            time_t t = std::time(0);
            static tm lt = *localtime(&t);
            static tm gmt = *gmtime(&t);
            quickbook::current_time = &lt;
            quickbook::current_gm_time = &gmt;
            quickbook::debug_mode = false;
        }
        
        if (vm.count("include-path"))
        {
            std::vector<quickbook::detail::input_path> paths
                = vm["include-path"].as<
                    std::vector<quickbook::detail::input_path> >();
            quickbook::include_path
                = std::vector<std::string>(paths.begin(), paths.end());
        }

        if (vm.count("input-file"))
        {
            std::string filein
                = vm["input-file"].as<quickbook::detail::input_path>();
            std::string fileout;

            if (vm.count("output-file"))
            {
                fileout = vm["output-file"].as<quickbook::detail::input_path>();
            }
            else
            {
                fileout = quickbook::detail::remove_extension(filein.c_str());
                fileout += ".xml";
            }

            std::cout << "Generating Output File: "
                << fileout
                << std::endl;

            return quickbook::parse(filein.c_str(), fileout.c_str(), indent, linewidth, pretty_print);
        }
        else
        {
            quickbook::detail::outerr("") << "Error: No filename given\n\n"
                << desc << std::endl;
            return 1;
        }
    }

    catch(std::exception& e)
    {
        quickbook::detail::outerr("") << "Error: " << e.what() << "\n";
        return 1;
    }

    catch(...)
    {
        quickbook::detail::outerr("") << "Error: Exception of unknown type caught\n";
        return 1;
    }

    return 0;
}
