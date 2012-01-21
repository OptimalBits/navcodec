
#ifndef _NAVUTILS_H
#define _NAVUTILS_H

#define UNWRAP_OBJECT(ClassName, args) node::ObjectWrap::Unwrap<ClassName>(args.This())

#define GET_VALUE(obj, key) \
obj->Get(String::New(key))->conv()

#define GET_OPTION(obj, key, conv, default_val) \
obj->Get(String::New(#key))->conv()

#define GET_OPTION_UINT32(obj, key, val) \
obj->Has(String::New(#key))?obj->Get(String::New(#key))->Uint32Value():val

#define SET_KEY_VALUE(obj, key, val) \
obj->Set(String::NewSymbol(key), val);

#endif // _NAVUTILS_H
