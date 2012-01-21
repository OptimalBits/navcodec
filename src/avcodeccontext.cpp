
#include "avcodeccontext.h"
#include "navutils.h"

using namespace v8;

void NoopSet(Local<String> property, Local<Value> value, 
             const AccessorInfo& info) {};

AVCodecContext *GetCodecContext(const AccessorInfo& info){
  Local<Object> self = info.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  return (AVCodecContext*) wrap->Value();
}

#define GET_CODEC_CONTEXT(self)\
    (AVCodecContext*)(Local<External> wrap = Local<External>::Cast(self->GetInternalField(0))->Value())

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
    
  // Accessors make us simulate read-only properties.
  templ->SetAccessor(String::New("codec_type"), GetType, NoopSet);
  templ->SetAccessor(String::New("width"), GetWidth, NoopSet);
  templ->SetAccessor(String::New("height"), GetHeight, NoopSet);
  templ->SetAccessor(String::New("bit_rate"), GetBitRate, NoopSet);
  templ->SetAccessor(String::New("sample_fmt"), GetSampleFmt, NoopSet);
  templ->SetAccessor(String::New("sample_rate"), GetSampleRate, NoopSet);
  templ->SetAccessor(String::New("channels"), GetChannels, NoopSet);
  templ->SetAccessor(String::New("framerate"), GetFramerate, NoopSet);
  
//  templ->SetAccessor(String::New("pix_fmt"), GetPixFmt, NoopSet);
//  templ->SetAccessor(String::New("id"), GetCodecId, NoopSet); 
    
  _AVCodecContext::templ = Persistent<ObjectTemplate>::New(templ);
}
  
Handle<Object> _AVCodecContext::New(AVCodecContext *pContext) {
  HandleScope scope;
    
  Local<Object> obj = _AVCodecContext::templ->NewInstance();
  obj->SetInternalField(0, External::New(pContext));
    
  NODE_SET_METHOD(obj, "open", Open);
  
  Local<Object> timeBase = Object::New();
  
  SET_KEY_VALUE(timeBase, "num", Integer::New(pContext->time_base.num));
  SET_KEY_VALUE(timeBase, "den", Integer::New(pContext->time_base.den));
  
  SET_KEY_VALUE(obj, "time_base", timeBase);
  SET_KEY_VALUE(obj, "ticks_per_frame", Integer::New(pContext->ticks_per_frame));

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
    
  AVCodecContext *pCodecCtx = GetCodecContext(info);
    
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
  AVCodecContext *pCodecCtx = GetCodecContext(info);
  Handle<Integer> width = Integer::New(pCodecCtx->width);
  return scope.Close(width);
}
  
Handle<Value> _AVCodecContext::GetHeight(Local<String> property, 
                                 const AccessorInfo& info) {
  HandleScope scope;
  AVCodecContext *pCodecCtx = GetCodecContext(info);
  Handle<Integer> height = Integer::New(pCodecCtx->height);
  return scope.Close(height);
}

Handle<Value> _AVCodecContext::GetBitRate(Local<String> property, 
                                         const AccessorInfo& info) {
  HandleScope scope;
  AVCodecContext *pCodecCtx = GetCodecContext(info);
  Handle<Integer> bit_rate = Integer::New(pCodecCtx->bit_rate);
  return scope.Close(bit_rate);
}

Handle<Value> _AVCodecContext::GetSampleFmt(Local<String> property, 
                                          const AccessorInfo& info) {
  HandleScope scope;
  AVCodecContext *pCodecCtx = GetCodecContext(info);
  Handle<Integer> sample_fmt = Integer::New(pCodecCtx->sample_fmt);
  return scope.Close(sample_fmt);
}
Handle<Value> _AVCodecContext::GetSampleRate(Local<String> property, 
                                             const AccessorInfo& info) {
  HandleScope scope;
  AVCodecContext *pCodecCtx = GetCodecContext(info);
  Handle<Integer> sample_rate = Integer::New(pCodecCtx->sample_rate);
  return scope.Close(sample_rate);
}
Handle<Value> _AVCodecContext::GetChannels(Local<String> property, 
                                          const AccessorInfo& info) {
  HandleScope scope;
  AVCodecContext *pCodecCtx = GetCodecContext(info);
  Handle<Integer> channels = Integer::New(pCodecCtx->channels);
  return scope.Close(channels);
}
Handle<Value> _AVCodecContext::GetFramerate(Local<String> property, 
                                         const AccessorInfo& info) {
  HandleScope scope;
  AVCodecContext *pCodecCtx = GetCodecContext(info);

  float framerate = 0.0;
  if (pCodecCtx->codec_type == AVMEDIA_TYPE_VIDEO) {
    framerate = (float) pCodecCtx->time_base.den / 
      (float) (pCodecCtx->time_base.num * pCodecCtx->ticks_per_frame);
  }

  Handle<Number> val = Number::New(framerate);
  return scope.Close(val);
}

