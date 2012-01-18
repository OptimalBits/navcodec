
#ifndef _NAV_OUTPUT_FORMAT_H
#define _NAV_OUTPUT_FORMAT_H

#include "avstream.h"

#include <v8.h>
#include <node.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

using namespace v8;

class NAVOutputFormat : node::ObjectWrap {
private:
  AVFormatContext *pFormatCtx;
  AVOutputFormat *pOutputFormat;

  char *filename;
  
  uint8_t *pVideoBuffer;
  int videoBufferSize;
  
public:
  NAVOutputFormat();
  ~NAVOutputFormat();
  
  static Persistent<FunctionTemplate> templ;
  
  static void Init(Handle<Object> target);
  
  static Handle<Value> New(const Arguments& args);
  
  static Handle<Value> New(AVFormatContext *pContext);
  
  // (stream_type::String, [codecId::String])
  static Handle<Value> AddStream(const Arguments& args);
  
  // ()
  static Handle<Value> Begin(const Arguments& args);
  
  // (stream::AVStream, frame::AVFrame)
  static Handle<Value> Encode(const Arguments& args);
  
  // ()
  static Handle<Value> End(const Arguments& args);
};

#endif // _NAV_OUTPUT_FORMAT_H
