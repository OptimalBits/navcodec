
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
  
    // Global initiallization of libavcodec.
    av_register_all();
    avformat_network_init();
    
    Handle<Array> version = Array::New(3);
    version->Set(0, Integer::New(LIBAVFORMAT_VERSION_MAJOR));
    version->Set(1, Integer::New(LIBAVFORMAT_VERSION_MINOR));
    version->Set(2, Integer::New(LIBAVFORMAT_VERSION_MICRO));
    
    target->Set(String::NewSymbol("AVFormatVersion"), version); 
    
    target->Set(String::NewSymbol("PixelFormat"), CreatePixelFormatsEnum()); 
    
    AVFormat::Init(target);
    NAVOutputFormat::Init(target);
    NAVSws::Init(target);
    
    // Objects only instantiable from C++
    _AVFrame::Init();
    _AVStream::Init();    
    _AVCodecContext::Init();
  }
  NODE_MODULE(navcodec, init);
}

