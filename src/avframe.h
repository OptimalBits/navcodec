
#ifndef _AVFRAME_H
#define _AVFRAME_H

#include <v8.h>
#include <node.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

using namespace v8;

class _AVFrame : node::ObjectWrap {
private:
  AVFrame *pFrame;
  
public:
  _AVFrame();
  
  ~_AVFrame();
  
  static Persistent<FunctionTemplate> templ;
  
  static void Init(Handle<Object> target);
  
  static Handle<Value> New(const Arguments& args);
  
};

#endif //_AVFRAME_H
