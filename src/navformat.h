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

#ifndef _NAVFORMAT_H
#define _NAVFORMAT_H

#include "navstream.h"

#include <v8.h>
#include <node.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

using namespace v8;

class NAVFormat : node::ObjectWrap {
private:
  AVFormatContext *pFormatCtx;
  char *filename;
  Handle<Array> streams;
  
public:
  NAVFormat();
  ~NAVFormat();
  
  static Persistent<FunctionTemplate> templ;
  
  static void Init(Handle<Object> target);
  
  static Handle<Value> New(const Arguments& args);
  
  static Handle<Value> New(AVFormatContext *pContext);
  
  /*
   // this.title=
   static void SetTitle(Local<String> property, Local<Value> value, const AccessorInfo& info) {
   Gtknotify* gtknotify_instance = node::ObjectWrap::Unwrap<Gtknotify>(info.Holder());
   v8::String::Utf8Value v8str(value);
   gtknotify_instance->title = *v8str;
   }
   */
  
  // ([streams], cb(stream, frame))
  static Handle<Value> Version(const Arguments& args);
  static Handle<Value> Decode(const Arguments& args);
  static Handle<Value> Dump(const Arguments& args);
};

#endif // _NAVFORMAT_H
