
#include "avstream.h"
#include "avcodeccontext.h"

using namespace v8;


_AVStream::_AVStream(){
}
_AVStream::~_AVStream(){
}

Handle<Object> _AVStream::New(AVStream *s){
  HandleScope scope;
  
  Handle<Object> stream = Object::New();
  
  Handle<Value> codec = _AVCodecContext::New(s->codec);
  
  stream->Set(String::New("codec"), codec);
  
  return scope.Close(stream);
}
