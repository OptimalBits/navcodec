
#ifndef _NAVSWS_H
#define _NAVSWS_H

#include "navcodec.h"

#include <v8.h>
#include <node.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

using namespace v8;

class NAVSws : node::ObjectWrap {
private:
  struct SwsContext *pContext;
  AVFrame *pFrame;

public:
  NAVSws();
  ~NAVSws();
  
  static Persistent<FunctionTemplate> templ;
  
  static void Init(Handle<Object> target);
  
  // (srcStream, dstStream)
  static Handle<Value> New(const Arguments& args);
    
  // (srcFrame) -> dstFrame
  static Handle<Value> Convert(const Arguments& args);
};

#endif // _NAVSWS_H
