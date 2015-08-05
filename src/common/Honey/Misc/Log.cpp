// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.

#include "Honey/Misc/Log.h"

namespace honey
{
namespace log
{
    namespace level
    {
        /** \cond */
        namespace priv
        {
            static bool init()
            {
                error.add("critical");
                warning.add("error");
                info.add("warning");
                debug.add("info");
                return true;
            }
        }
        /** \endcond */
        Level critical(nullptr, "critical");
        Level error(nullptr, "error");
        Level warning(nullptr, "warning");
        Level info(nullptr, "info");
        Level debug(nullptr, "debug");
    }
    
    String format(const Level& level, const String& record)
    {
        char sz[64];
        auto t = std::time(NULL);
        std::strftime(sz, sizeof(sz), "%d/%b/%Y:%H:%M:%S", std::localtime(&t));
        return sout() <<    "[" << sz << "] " <<
                            level.getKey().name().toUpper() << ": " <<
                            record;
    }
    
    void StreamSink::operator()(const Level& level, const String& record)
    {
        os << format(level, record) << endl;
        os.flush();
    }
    
    FileSink::FileSink(String filepath) :
        StreamSink(os),
        filepath(filepath)
    {
        os.exceptions(std::ofstream::failbit | std::ofstream::badbit); //enable exceptions
        os.open(filepath, std::ofstream::out | std::ofstream::app); //open for append
    }
    
    FileSink::~FileSink()
    {
        os.exceptions(std::ofstream::goodbit); //disable exceptions
        os.close();
    }

    void FileSink::operator()(const Level& level, const String& record)
    {
        StreamSink::operator()(level, record);
    }
}

Log::Log()
{
    static bool _ = log::level::priv::init(); mt_unused(_);
    addLevel(log::level::critical);
    addLevel(log::level::error);
    addLevel(log::level::warning);
    addLevel(log::level::info);
    addLevel(log::level::debug);
    
    addSink("stdout"_id, new log::StreamSink(std::cout));
    filter("stdout"_id, {&log::level::debug}, true, {&log::level::error});
    addSink("stderr"_id, new log::StreamSink(std::cerr));
    filter("stderr"_id, {&log::level::error});
    
    _level = &log::level::info;
}

void Log::addLevel(const log::Level& level)
{
    _levelGraph.add(level);
}

void Log::removeLevel(const log::Level& level)
{
    _levelGraph.remove(level);
}

void Log::addSink(const Id& name, const SinkPtr& sink)
{
    _sinks[name] = sink;
}

void Log::removeSink(const Id& name)
{
    _sinks.erase(name);
    clearFilter(name);
}
    
void Log::filter(   const Id& sink, const vector<log::Level*>& includes, bool includeDeps,
                    const vector<log::Level*>& excludes, bool excludeDeps)
{
    auto& filter = _filters[sink];
    for (auto level: includes)
    {
        if (includeDeps)
            for (auto e: _levelGraph.range(level->getKey())) filter.insert(*e.keys().begin());
        else
            filter.insert(level->getKey());
    }
    for (auto level: excludes)
    {
        if (excludeDeps)
            for (auto e: _levelGraph.range(level->getKey())) filter.erase(*e.keys().begin());
        else
            filter.erase(level->getKey());
    }
}

void Log::clearFilter(const Id& sink)
{
    _filters.erase(sink);
}

void Log::push(const String& record)
{
    for (auto e: _sinks)
    {
        auto it = _filters.find(e.first);
        if (it != _filters.end() && !it->second.count(_level->getKey())) continue;
        (*e.second)(*_level, record);
    }
}

}