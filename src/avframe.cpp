
#include "avframe.h"

using namespace v8;

Persistent<FunctionTemplate> _AVFrame::templ;

_AVFrame::_AVFrame(){
}
  
_AVFrame::~_AVFrame(){
  printf("Frame Free'd\n");
  av_free(pFrame);
}
  
void _AVFrame::Init(Handle<Object> target){
  HandleScope scope;
    
  // Our constructor
  Local<FunctionTemplate> templ = FunctionTemplate::New(New);
    
  _AVFrame::templ = Persistent<FunctionTemplate>::New(templ);
    
  _AVFrame::templ->InstanceTemplate()->SetInternalFieldCount(1); // 1 since this is a constructor function
  _AVFrame::templ->SetClassName(String::NewSymbol("AVFrame"));
    
  // NODE_SET_PROTOTYPE_METHOD(AVFormat::persistent_function_template, "dump", Dump);
    
  target->Set(String::NewSymbol("AVFrame"), _AVFrame::templ->GetFunction());
}
  
Handle<Value> _AVFrame::New(const Arguments& args) {
  HandleScope scope;
    
  _AVFrame* instance = new _AVFrame();
    
  instance->pFrame = avcodec_alloc_frame();
  if(instance->pFrame == NULL){
    // throw expection!
  }
    
  // Wrap our C++ object as a Javascript object
  instance->Wrap(args.This());
    
  return args.This();
}
