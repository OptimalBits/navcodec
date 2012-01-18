#ifndef _AVSTREAM_H
#define _AVSTREAM_H

#include <v8.h>
#include <node.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

using namespace v8;

class _AVStream  {

public:
  AVStream *pContext;
  
  _AVStream(AVStream *pStream);
  ~_AVStream();
  
  static Persistent<ObjectTemplate> templ;
  
  static void Init();
  
  static Handle<Value> New(const Arguments& args);
  static Handle<Value> New(AVStream *pStream);
};

#endif _AVSTREAM_H
