
#ifndef _AVFORMAT_H
#define _AVFORMAT_H

#include "avstream.h"

#include <v8.h>
#include <node.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

using namespace v8;

class AVFormat : node::ObjectWrap {
private:
  AVFormatContext *pFormatCtx;
  char *filename;
  Handle<Array> streams;
  
public:
  AVFormat();
  ~AVFormat();
  
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

#endif // _AVFORMAT_H
