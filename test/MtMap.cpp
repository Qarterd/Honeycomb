// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.
#pragma hdrstop

#include "MtMap.h"
#include "Honey/Misc/MtMap.h"
#include "Honey/String/Stream.h"

namespace honey
{

mtkey(key_int);         //Construct keys
mtkey(key_id);
mtkey(key_char);
mtkey(key_string);

mtkeygen(key_index);    //Construct a templated key generator that can be used to turn the map into an array,
                        //indexable by static ints and compile-time integer arithmetic.

template<class T> void iterTest(T&);
void keywordTest();

void mtmap_test()
{
                                                        //Declare a map. Note the "Value, key" order, similar to a variable declaration
    typedef MtMap<int, key_int, Id, key_id> FooMap;
                                                            //Construct using mtmap factory to initialize in any order
                                                            //All keys are required on construction, for optional keys see keyword args example
    FooMap foo = mtmap(key_id() = "foo", key_int() = 1);    //Key matching is resolved at compile-time, move semantics ensure fast init

    int x = foo[key_int()];                             //Get at key
    Id myid = foo[key_id()];

    foo[key_int()] = 2;                                 //Set
    foo[key_id()] = "foo2"_id;
                                                                        
    assert(foo.hasKey(key_id()) && FooMap::hasKey_<key_id>::value); //Check if has key at run/compile-time
    assert((std::is_same<FooMap::getResult<key_id>, Id>::value));   //The result type of get(key_id) is Id
    assert(foo.size() == 2 && FooMap::size_::value == 2);           //Get number of keys at run/compile-time

    x = foo.get(key_int());                             //Get at key
    foo.set(key_id() = "foo3"_id);                      //Set accepts any type, converts using operator=.  Returns true.
    foo.set(key_char() = 'a');                          //Key doesn't exist, returns false

                                                        //Create a map that holds references 
                                                        //char doesn't exist in fooRef, so it will be ignored in the initialization
    MtMap<int&, key_int, Id&, key_id> fooRef =
        mtmap(key_id() = myid, key_int() = x, key_char() = 'b');                
                        
    fooRef[key_int()] = 3;                              //Set x to 3
    fooRef[key_id()] = "foo4"_id;                       //Set myid to "foo4"

    foo = fooRef;                                       //Flexible map assignment. Matching keys are copied from fooRef using each value type's operator=
                                                        
                                                        //Insert keys/values into map 
                                                        //Insert asserts that the keys don't already exist
    FooMap::insertResult<String, key_string, int8, key_char> fooInsert =
        foo.insert(key_string() = "foo5", key_char() = 'c');

                                                        //Erase keys from the map, it's ok if the keys don't exist
                                                        //Returns decltype(fooInsert)::eraseResult<...>::type
    auto fooErase = fooInsert.erase(key_id(), key_int());

    auto empty = foo.clear();                           //Clear map of keys, returns MtMap<>
    assert(empty.empty());                              //Test if empty at run-time

    mt::Void empty_val = empty[key_string()];           //Key doesn't exist, returns mt::Void
    mt_unused(empty_val);
    empty.set(key_string() = "empty");                  //Key doesn't exist, returns false

    enum { IDX0, IDX1 };                                //Use the key generator to make map indexable by static ints and arithmetic             
    MtMap<Id, key_index<IDX0>, String, key_index<IDX1>> indexmap =
        mtmap(key_index<IDX0 + 1>() = "idx1", key_index<IDX1 - 1>() = "idx0");

    iterTest(fooInsert);
    keywordTest();
}

                                                        //Define a functor for iterating over the map
struct Functor
{                                                       //Generic catch-all
    template<class Key, class Val>
    void operator()(Key key, const Val& val)
    {
        mt_unused(key);
        debug_print(sout() << "Key: " << key.id() << " ; Value: " << val << endl);
    }
                                                        //Overload for specific key/value pair
    void operator()(key_int key, int& val)
    {
        mt_unused(key);
        debug_print(sout() << "key: " << key.id() << " ; value: " << val << endl);

        val = -1;                                       //Modify value in map
    }
};

template<class T>
void iterTest(T& fooInsert)
{
                                                        //Use Begin/End and functor to print contents of map
    debug_print(sout() << "--fooInsert--" << endl);
    for_each_mtmap(fooInsert.begin(), fooInsert.end(), Functor());

                                                        //Use iter() to get an iterator by key
    debug_print(sout() << "--fooInsert[key_int, end]--" << endl);
    for_each_mtmap(fooInsert.iter(key_int()), fooInsert.end(), Functor());
}

                                                        //Define a function that takes keyword arguments.
                                                        //Note that key_id is optional and has a default, all others are required.
                                                        //The default value is wrapped in a lambda so the Id ctor can be omitted.
void keywordFunc(MtMap<int8, key_char, int, key_int, optional<Id>, key_id> _)
{
    _.setDefaults(mtmap(key_id() = []{ return "default"_id; }));
    debug_print(sout() << "Keyword Args: " << _ << endl);
}

void keywordTest()
{
    keywordFunc(mtmap(key_int() = 1, key_char() = 'c'));
    keywordFunc(mtmap(key_int() = 1, key_char() = 'c', key_id() = "user"));
}

}
