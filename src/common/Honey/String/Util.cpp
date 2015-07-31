// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.

#include "Honey/String/Util.h"

namespace honey { namespace string
{
 
void parseArgv(const String& str, int& argc, char**& argv)
{
    std::stringstream is(str);
    is >> std::noskipws;            //disable whitespace skipping
    std::vector<std::string> args;
    std::stringstream curArg("");
    curArg >> std::noskipws;

    enum State
    {
        InArg,      //scanning an argument
        InArgQuote, //scanning an argument (which started with quotes)
        OutOfArg    //not scanning an argument
    };
    State state = OutOfArg;
    char quoteChar = '\0'; //to distinguish between ' and ", this allows to use "foo'bar"

    argc = 0;
    argv = nullptr;
    
    char c;
    while (!is.eof() && (is >> c))
    {
        switch (c)
        {
        case '\"':
        case '\'':
            switch (state)
            {
            case OutOfArg:
                curArg.str(std::string());
            case InArg:
                state = InArgQuote;
                quoteChar = c;
                break;
            case InArgQuote:
                if (c == quoteChar) state = InArg;
                else curArg << c;
                break;
            }
            break;
            
        case ' ':
        case '\t':
            switch (state)
            {
            case InArg:
                args.push_back(curArg.str());
                state = OutOfArg;
                break;
            case InArgQuote:
                curArg << c;
                break;
            case OutOfArg:
                break;
            }
            break;
            
        case '\\':
            switch (state)
            {
            case OutOfArg:
                curArg.str(std::string());
                state = InArg;
            case InArg:
            case InArgQuote:
                if(is.eof()) throw_ Exception() << "found escape character at end of file";
                else
                {
                    is >> c;
                    curArg << c;
                }
                break;
            }
            break;
            
        default:
            switch (state)
            {
            case InArg:
            case InArgQuote:
                curArg << c;
                break;
            case OutOfArg:
                curArg.str(std::string());
                curArg << c;
                state = InArg;
                break;
            }
            break;
        }
    }

    if (state == InArg) args.push_back(curArg.str());
    else if (state == InArgQuote) throw_ Exception() << "starting quote has no ending quote";

    argc = size(args);
    argv = new char*[args.size()];
    for (auto i: range(args.size()))
    {
        argv[i] = new char[args[i].length()+1];
        std::strcpy(argv[i], args[i].c_str());
    }
}

void deleteArgv(int argc, char**& argv)
{
    for (auto i: range(argc)) delete_(argv[i]);
    delete_(argv);
}

} }