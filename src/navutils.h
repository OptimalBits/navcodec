/* Copyright(c) 2012 Optimal Bits Sweden AB. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef _NAVUTILS_H
#define _NAVUTILS_H

#define UNWRAP_OBJECT(ClassName, args) node::ObjectWrap::Unwrap<ClassName>(args.This())

#define GET_VALUE(obj, key) \
(obj->Get(String::New(key))->conv())

#define GET_OPTION(obj, key, conv, default_val) \
(obj->Get(String::New(#key))->conv())

#define GET_OPTION_UINT32(obj, key, val) \
(obj->Has(String::New(#key))?obj->Get(String::New(#key))->Uint32Value():val)

#define SET_KEY_VALUE(obj, key, val) \
(obj->Set(String::NewSymbol(key), val))

#define OBJECT_SET_ENUM(obj, value) \
(obj->Set(String::NewSymbol(#value), Integer::New(value)))

#endif // _NAVUTILS_H
