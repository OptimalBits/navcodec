
#include "avframe.h"

using namespace v8;

Persistent<ObjectTemplate> _AVFrame::templ;

_AVFrame::_AVFrame(AVFrame *pFrame){
  pContext = pFrame;
}
  
_AVFrame::~_AVFrame(){
  printf("Frame Free'd\n");
  av_free(pContext);
}
  
void _AVFrame::Init(){
  HandleScope scope;
  
  Local<ObjectTemplate> templ = ObjectTemplate::New();
  templ = ObjectTemplate::New();
  templ->SetInternalFieldCount(1);
  
  _AVFrame::templ = Persistent<ObjectTemplate>::New(templ);
}
  
Handle<Object> _AVFrame::New(AVFrame *pFrame) {
  HandleScope scope;
  
  _AVFrame *instance = new _AVFrame(pFrame);

  Local<Object> obj = _AVFrame::templ->NewInstance();
  obj->SetInternalField(0, External::New(instance));

  return scope.Close(obj);
}
