
#include "avstream.h"
#include "avcodeccontext.h"

using namespace v8;

Persistent<ObjectTemplate> _AVStream::templ;

_AVStream::_AVStream(AVStream *pStream){
  this->pContext = pStream;
}
_AVStream::~_AVStream(){
}

void _AVStream::Init(){
  HandleScope scope;
  
  Local<ObjectTemplate> templ = ObjectTemplate::New();
  templ = ObjectTemplate::New();
  templ->SetInternalFieldCount(1);
  
  _AVStream::templ = Persistent<ObjectTemplate>::New(templ);
}

Handle<Value> _AVStream::New(AVStream *pStream){
  HandleScope scope;
  
  _AVStream *instance = new _AVStream(pStream);
  
  Local<Object> obj = _AVStream::templ->NewInstance();
  obj->SetInternalField(0, External::New(instance));
    
  Handle<Value> codec = _AVCodecContext::New(pStream->codec);
  
  obj->Set(String::NewSymbol("codec"), codec);
  
  return scope.Close(obj);
}
HandleScope scope;
