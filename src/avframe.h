
#ifndef _AVFRAME_H
#define _AVFRAME_H

#include <v8.h>
#include <node.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

using namespace v8;

class _AVFrame {

public:

  AVFrame *pContext;
  _AVFrame(AVFrame *pContext);
  ~_AVFrame();
  
  static Persistent<ObjectTemplate> templ;
  
  static void Init();
  
  static Handle<Object> New(AVFrame *pFrame);
};

#endif //_AVFRAME_H
