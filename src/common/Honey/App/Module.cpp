// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.

#include "Honey/App/App.h"

namespace honey { namespace app
{

static auto _ = ModuleRegistry::inst().reg(new Module("root"_id, []{}));

Module::Module( const Id& id, const function<void ()>& f,
                const vector<Id>& outDeps, const vector<Id>& inDeps) :
    task(new DepTask_<void>(f, id))
{
    task->deps().add("root"_id, DepTask::DepNode::DepType::in);
    for (auto& e: outDeps) task->deps().add(e);
    for (auto& e: inDeps) task->deps().add(e, DepTask::DepNode::DepType::in);
}

mt::Void ModuleRegistry::reg(const Module::Ptr& module)
{
    modules[module->task->getId()] = module;
    return mt::Void();
}

} }
