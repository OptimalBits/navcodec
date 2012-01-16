
#include "navcodec.h"

#include <v8.h>
#include <node.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

using namespace v8;

extern "C" { // Cause of name mangling in C++, we use extern C here
  static void init(Handle<Object> target) {
    AVFormat::Init(target);
    _AVFrame::Init(target);
    
    _AVCodecContext::Init();
    
  }
  NODE_MODULE(navcodec, init);
}
