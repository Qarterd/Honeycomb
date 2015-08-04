// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma once

#include "Honey/String/String.h"
#include "Honey/Graph/Dep.h"

namespace honey
{
/// Logger methods
namespace log
{
    /// Severity level
    typedef DepNode<void*, NameId> Level;
    
    /// Severity levels (default levels)
    namespace level
    {
        extern Level critical;  ///< Information describing a critical problem that has occurred
        extern Level error;     ///< Information describing a major problem that has occurred
        extern Level warning;   ///< Information describing a minor problem that has occurred
        extern Level info;      ///< General information
        extern Level debug;     ///< Low-level information for debugging purposes
    }
    
    /// Format record with date and level id
    String format(const Level& level, const String& record);
    
    struct Sink
    {
        virtual void operator()(const Level& level, const String& record) = 0;
    };
    
    /// Standard streams sink (default sink)
    struct StdSink : public Sink
    {
        virtual void operator()(const Level& level, const String& record);
    };
    
    /// File sink
    struct FileSink : public Sink
    {
        virtual void operator()(const Level& level, const String& record);
    };
}

/// Logger
class Log
{
public:
    typedef DepGraph<const log::Level> LevelGraph;
    typedef SharedPtr<log::Sink> SinkPtr;
    
    /// Builds a record
    struct RecordStream : ostringstream
    {
        RecordStream(Log& log)                  : log(&log) {}
        RecordStream(RecordStream&& rhs)        : ostringstream(std::move(rhs)), log(rhs.log) { rhs.log = nullptr; }
        ~RecordStream()                         { if (!log) return; log->push(str()); }
        Log* log;
    };
    
    Log();
    
    /// Add a severity level to categorize records
    void addLevel(const log::Level& level);
    void removeLevel(const log::Level& level);
    
    /// Add a sink to receive records
    void addSink(const Id& name, const SinkPtr& sink);
    void removeSink(const Id& name);
    
    /// Add a record filter to a sink
    /**
      * \param sink         sink to filter
      * \param includes     levels to push to sink
      * \param includeDeps  also push to sink any levels that the includes depend on
      * \param excludes     levels to not push to sink
      */  
    void filter(const Id& sink, const vector<log::Level*>& includes, bool includeDeps = true, const vector<log::Level*>& excludes = {});
    void clearFilter(const Id& sink);
    
    /// Push a record with severity level to all sinks
    RecordStream operator<<(const log::Level& level)    { _level = &level; return RecordStream(*this); }
    
private:
    void push(const String& record);
    
    typedef std::map<Id, SinkPtr> SinkMap;
    typedef std::map<Id, std::set<Id>> FilterMap;
    
    LevelGraph _levelGraph;
    const log::Level* _level;
    SinkMap _sinks;
    FilterMap _filters;
};

}
