
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
private:
  AVStream *pContext;
  
public:
  _AVStream();
  ~_AVStream();
  
  static Handle<Object> New(AVStream *pStream);
};

#endif _AVSTREAM_H
