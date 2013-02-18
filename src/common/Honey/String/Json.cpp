// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.

#include "Honey/String/Json.h"
#include "yajl_gen.h"
#include "yajl_parse.h"

namespace honey
{

template class Manip<json::priv::WriterManip>;

namespace json
{

template<class Config>
struct Parser
{
    typedef Value_<Config> Value;
    typedef typename Value::Array Array;
    typedef typename Value::Object Object;
    
    bool error_(const String& msg)
    {
        //It's not safe to throw exceptions from callback, save error to throw later and debug break here
        error = sout() << "Parse error: " << msg;
        error(error);
        return false;
    }

    pair<Value*, bool> add(Value&& val)
    {
        if (stack.empty()) return make_pair(nullptr, error_("stack empty"));
        Value& parent = stack.back();
        switch (parent.type())
        {
        case ValueType::Array:
            parent.push_back(move(val));
            return make_pair(&parent.back(), true);
        case ValueType::Object:
            {
                if (key.empty()) return make_pair(nullptr, error_("invalid key, can't insert value into object"));
                auto pair = parent.insert(key, move(val));
                if (!pair.second) return make_pair(nullptr, error_(sout() << "duplicate key in json object: " << key));
                return make_pair(pair.first->second.ptr(), true);
            }
        default:
            return make_pair(nullptr, error_("invalid parent type"));
        }
    }

    static int null_(void* ctx)
    {
        Parser& parser = *static_cast<Parser*>(ctx);
        return parser.add(Value()).second;
    }

    static int bool_(void* ctx, int boolVal)
    {
        Parser& parser = *static_cast<Parser*>(ctx);
        return parser.add(Value(bool(boolVal))).second;
    }

    static int int_(void* ctx, long long intVal)
    {
        Parser& parser = *static_cast<Parser*>(ctx);
        return parser.add(Value(int64(intVal))).second;
    }

    static int double_(void* ctx, double doubleVal)
    {
        Parser& parser = *static_cast<Parser*>(ctx);
        return parser.add(Value(doubleVal)).second;
    }

    static int String_(void* ctx, const uint8* str, size_t len)
    {
        Parser& parser = *static_cast<Parser*>(ctx);
        return parser.add(Value(String(reinterpret_cast<const char*>(str), (int)len))).second;
    }

    static int ObjectKey(void* ctx, const uint8* str, size_t len)
    {
        Parser& parser = *static_cast<Parser*>(ctx);
        if (parser.stack.empty()) return parser.error_("stack empty");
        parser.key = String(reinterpret_cast<const char*>(str), (int)len);
        return true;
    }

    static int ObjectStart(void* ctx)
    {
        Parser& parser = *static_cast<Parser*>(ctx);
        if (parser.stack.empty()) return parser.error_("stack empty");
        Value& parent = parser.stack.back();
        if (parent.type() == ValueType::Null)
        {
            if (parser.stack.size() > 1) return parser.error_("invalid parent type");
            parent = Object();
        }
        else
        {
            auto pair = parser.add(Value(Object()));
            parser.stack.push_back(pair.first);
            return pair.second;
        }
        return true;
    }

    static int ObjectEnd(void* ctx)
    {
        Parser& parser = *static_cast<Parser*>(ctx);
        if (parser.stack.empty()) return parser.error_("stack empty");
        Value& val = parser.stack.back();
        if (val.type() != ValueType::Object) return parser.error_("unexpected end of object");
        parser.stack.pop_back();
        return true;
    }

    static int ArrayStart(void* ctx)
    {
        Parser& parser = *static_cast<Parser*>(ctx);
        if (parser.stack.empty()) return parser.error_("stack empty");
        Value& parent = parser.stack.back();
        if (parent.type() == ValueType::Null)
        {
            if (parser.stack.size() > 1) return parser.error_("invalid parent type");
            parent = Array();
        }
        else
        {
            auto pair = parser.add(Value(Array()));
            parser.stack.push_back(pair.first);
            return pair.second;
        }
        return true;
    }

    static int ArrayEnd(void* ctx)
    {
        Parser& parser = *static_cast<Parser*>(ctx);
        if (parser.stack.empty()) return parser.error_("stack empty");
        Value& val = parser.stack.back();
        if (val.type() != ValueType::Array) return parser.error_("unexpected end of array");
        parser.stack.pop_back();
        return true;
    }

    vector<deref_wrap<Value>> stack;
    String key;
    String error;
    
    static yajl_callbacks callbacks;
};

template<class Config>
yajl_callbacks Parser<Config>::callbacks =
{
    null_,
    bool_,
    int_,
    double_,
    nullptr,
    String_,
    ObjectStart,
    ObjectKey,
    ObjectEnd,
    ArrayStart,
    ArrayEnd
};

template<class Config>
istream& operator>>(istream& is, Value_<Config>& val)
{
    val = null;
    Parser<Config> parser;
    parser.stack.push_back(&val);
    
    yajl_handle hand = yajl_alloc(&Parser<Config>::callbacks, nullptr, &parser);
    auto _ = ScopeGuard([&]{ yajl_free(hand); });
    yajl_config(hand, yajl_allow_comments, true);
    yajl_config(hand, yajl_allow_trailing_garbage, true);
    
    char buf[1024];
    yajl_status stat = yajl_status_ok;
    int readBytes = 0;
    while (stat == yajl_status_ok && !parser.stack.empty() && is.read(buf, sizeof(buf)).gcount())
    {
        readBytes = (int)is.gcount();
        stat = yajl_parse(hand, reinterpret_cast<uint8*>(buf), readBytes);
    }
    int extra = int(readBytes - yajl_get_bytes_consumed(hand));
    stat = yajl_complete_parse(hand);
    
    //handle errors
    switch (stat)
    {
    case yajl_status_client_canceled:
        throw_ ValueError() << parser.error;
    case yajl_status_error:
        {
            uint8* error_ = yajl_get_error(hand, true, reinterpret_cast<uint8*>(buf), is.gcount());
            String error = reinterpret_cast<char*>(error_);
            yajl_free_error(hand, error_);
            throw_ ValueError() << error;
        }
    default:
        break;
    }
    
    //rewind stream to end of json array/object
    if (extra)
    {
        if (is.eof()) { is.clear(); is.seekg(-extra, ios_base::end); }
        else is.seekg(-extra, ios_base::cur);
    }
    
    if (val.type() != ValueType::Null && !parser.stack.empty()) throw_ ValueError() << "Parse error: root array/object not closed";
    
    return is;
}

template istream& operator>>(istream& is, Value_<Config<true>>& val);
template istream& operator>>(istream& is, Value_<Config<false>>& val);



template<class Config>
struct Writer
{
    typedef Value_<Config> Value;
    typedef typename Value::Array Array;
    typedef typename Value::ObjectUnordered ObjectUnordered;
    typedef typename Value::ObjectOrdered ObjectOrdered;
    
    Writer(yajl_gen hand)               : hand(hand) {}
    
    void error_(const String& msg)
    {
        String error = sout() << "Write error: " << msg;
        error(error);
        throw_ ValueError() << error;
    }
    
    void verifyStatus()
    {
        if (stat != yajl_gen_status_ok) error_(sout() << "yajl_gen_status " << stat);
    }
    
    void write(const Value& val)
    {
        val.visit(overload(
            [&](null_t)
            {
                stat = yajl_gen_null(hand);
                verifyStatus();
            },
            [&](double doubleVal)
            {
                stat = yajl_gen_double(hand, doubleVal);
                verifyStatus();
            },
            [&](int64 intVal)
            {
                stat = yajl_gen_integer(hand, intVal);
                verifyStatus();
            },
            [&](bool boolVal)
            {
                stat = yajl_gen_bool(hand, boolVal);
                verifyStatus();
            },
            [&](const String& str)
            {
                write(str);
            },
            [&](const Array& arr)
            {
                stat = yajl_gen_array_open(hand);
                verifyStatus();
                
                for (auto& e: arr) write(e);
                
                stat = yajl_gen_array_close(hand);
                verifyStatus();
            },
            [&](const ObjectUnordered& obj)
            {
                stat = yajl_gen_map_open(hand);
                verifyStatus();
                
                for (auto& e: obj)
                {
                    write(e.first.name());
                    write(e.second);
                }
                
                stat = yajl_gen_map_close(hand);
                verifyStatus();
            },
            [&](const ObjectOrdered& obj)
            {
                stat = yajl_gen_map_open(hand);
                verifyStatus();
                
                for (auto& e: obj.orderedNames)
                {
                    auto it = obj.find(NameId(e));
                    assert(it != obj.end(), "Ordered name not found in object");
                    write(it->first.name());
                    write(it->second);
                }
                
                stat = yajl_gen_map_close(hand);
                verifyStatus();
            }
        ));
    }
    
    void write(const String& str)
    {
        auto u8 = str.u8();
        stat = yajl_gen_string(hand, reinterpret_cast<const uint8*>(u8.c_str()), u8.length());
        verifyStatus();
    }
    
    yajl_gen hand;
    yajl_gen_status stat;
};

template<class Config>
ostream& operator<<(ostream& os, const Value_<Config>& val)
{
    yajl_gen hand = yajl_gen_alloc(nullptr);
    auto _ = ScopeGuard([&]{ yajl_gen_free(hand); });
    if (priv::WriterManip::hasInst(os))
    {
        auto& manip = priv::WriterManip::inst(os);
        yajl_gen_config(hand, yajl_gen_beautify, manip.beautify);
        yajl_gen_config(hand, yajl_gen_escape_solidus, manip.escapeSlash);
    }
    
    Writer<Config> writer(hand);
    writer.write(val);
    
    const uint8* buf; size_t len;
    yajl_gen_get_buf(hand, &buf, &len);
    assert(buf);
    return os << String(reinterpret_cast<const char*>(buf), (int)len);
}

template ostream& operator<<(ostream& os, const Value_<Config<true>>& val);
template ostream& operator<<(ostream& os, const Value_<Config<false>>& val);

} }
