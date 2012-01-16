
#include "avcodeccontext.h"

using namespace v8;

void NoopSet(Local<String> property, Local<Value> value, 
             const AccessorInfo& info) {};

Persistent<ObjectTemplate> _AVCodecContext::templ;

_AVCodecContext::_AVCodecContext(){}
  
_AVCodecContext::~_AVCodecContext(){
    avcodec_close(pContext);
}
  
void _AVCodecContext::Init(){
    HandleScope scope;
    
    Local<ObjectTemplate> templ = ObjectTemplate::New();
    templ = ObjectTemplate::New();
    templ->SetInternalFieldCount(1);
    
    templ->SetAccessor(String::New("codec_type"), GetType, NoopSet);
    templ->SetAccessor(String::New("width"), GetWidth, NoopSet);
    templ->SetAccessor(String::New("height"), GetHeight, NoopSet);
    
    _AVCodecContext::templ = Persistent<ObjectTemplate>::New(templ);
  }
  
Handle<Object> _AVCodecContext::New(AVCodecContext *pContext) {
    HandleScope scope;
    
    Local<Object> obj = _AVCodecContext::templ->NewInstance();
    obj->SetInternalField(0, External::New(pContext));
    
    NODE_SET_METHOD(obj, "open", Open);
    
    return scope.Close(obj);
  }
  
Handle<Value> _AVCodecContext::Open(const Arguments& args) {
    HandleScope scope;
    
    Local<Object> self = args.This();
    
    Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
    AVCodecContext *pCodecCtx = (AVCodecContext*) wrap->Value();
    
    AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if(pCodec==NULL) {
      // Throw error!
    }
    
    if(avcodec_open2(pCodecCtx, pCodec, NULL) < 0){
      // Throw error: Could not open codec
    }
    
    return Undefined();
  }
  
Handle<Value> _AVCodecContext::GetType(Local<String> property, 
                               const AccessorInfo& info) {
    HandleScope scope;
    
    Local<Object> self = info.Holder();
    
    Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
    AVCodecContext *pCodecCtx = (AVCodecContext*) wrap->Value();
    
    Handle<String> codecType;
    
    switch(pCodecCtx->codec_type){
      case AVMEDIA_TYPE_UNKNOWN: codecType = String::New("Unknown");break;
      case AVMEDIA_TYPE_VIDEO: codecType = String::New("Video");break;
      case AVMEDIA_TYPE_AUDIO: codecType = String::New("Audio");break;
      case AVMEDIA_TYPE_DATA: codecType = String::New("Data");break;
      case AVMEDIA_TYPE_SUBTITLE: codecType = String::New("Subtitle");break;
      case AVMEDIA_TYPE_ATTACHMENT: codecType = String::New("Attachment");break;
      case AVMEDIA_TYPE_NB: codecType = String::New("NB");break;
    }
    
    return scope.Close(codecType);
  }
  
Handle<Value> _AVCodecContext::GetWidth(Local<String> property, 
                                const AccessorInfo& info) {
    HandleScope scope;
    
    Local<Object> self = info.Holder();
    
    Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
    AVCodecContext *pCodecCtx = (AVCodecContext*) wrap->Value();
    
    Handle<Integer> width = Integer::New(pCodecCtx->width);
    
    return scope.Close(width);
  }
  
Handle<Value> _AVCodecContext::GetHeight(Local<String> property, 
                                 const AccessorInfo& info) {
    HandleScope scope;
    
    Local<Object> self = info.Holder();
    Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
    AVCodecContext *pCodecCtx = (AVCodecContext*) wrap->Value();
    
    Handle<Integer> height = Integer::New(pCodecCtx->height);
    
    return scope.Close(height);
  }


