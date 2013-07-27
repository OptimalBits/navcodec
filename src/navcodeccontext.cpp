/* Copyright(c) 2012 Optimal Bits Sweden AB. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "navcodeccontext.h"
#include "navutils.h"

using namespace v8;

void NoopSet(Local<String> property, Local<Value> value, 
             const AccessorInfo& info) {};

AVCodecContext *GetCodecContext(const AccessorInfo& info){
  Local<Object> self = info.Holder();
  return node::ObjectWrap::Unwrap<NAVCodecContext>(self)->pContext;
}

#define GET_CODEC_CONTEXT(self)\
    (AVCodecContext*)(Local<External> wrap = Local<External>::Cast(self->GetInternalField(0))->Value())

Persistent<ObjectTemplate> NAVCodecContext::templ;

NAVCodecContext::NAVCodecContext(AVCodecContext *pContext){
  this->pContext = pContext;
}
  
NAVCodecContext::~NAVCodecContext(){
  fprintf(stderr, "NAVCodecContext destructor\n");
//  avcodec_close(pContext);
}
  
void NAVCodecContext::Init(){
  HandleScope scope;
    
  Local<ObjectTemplate> templ = ObjectTemplate::New();
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
    
  NAVCodecContext::templ = Persistent<ObjectTemplate>::New(templ);
}
  
Handle<Object> NAVCodecContext::New(AVCodecContext *pContext) {
  HandleScope scope;
    
  Local<Object> obj = NAVCodecContext::templ->NewInstance();  
  NAVCodecContext *instance = new NAVCodecContext(pContext);
  
  instance->Wrap(obj);
    
  NODE_SET_METHOD(obj, "open", Open);
  
  Local<Object> timeBase = Object::New();
  
  SET_KEY_VALUE(timeBase, "num", Integer::New(pContext->time_base.num));
  SET_KEY_VALUE(timeBase, "den", Integer::New(pContext->time_base.den));
  
  SET_KEY_VALUE(obj, "time_base", timeBase);
  SET_KEY_VALUE(obj, "ticks_per_frame", Integer::New(pContext->ticks_per_frame));
  SET_KEY_VALUE(obj, "pix_fmt", Integer::New(pContext->pix_fmt));

  return scope.Close(obj);
}
  
Handle<Value> NAVCodecContext::Open(const Arguments& args) {
  HandleScope scope;
    
  Local<Object> self = args.This();
  
  AVCodecContext *pCodecCtx = node::ObjectWrap::Unwrap<NAVCodecContext>(self)->pContext;
  
  AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
  if(pCodec==NULL) {
    // Throw error!
  }
    
  if(avcodec_open2(pCodecCtx, pCodec, NULL) < 0){
    // Throw error: Could not open codec
  }
    
  return Undefined();
}
  
Handle<Value> NAVCodecContext::GetType(Local<String> property, 
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

Handle<Value> NAVCodecContext::GetWidth(Local<String> property, 
                                const AccessorInfo& info) {
  HandleScope scope;
  AVCodecContext *pCodecCtx = GetCodecContext(info);
  Handle<Integer> width = Integer::New(pCodecCtx->width);
  return scope.Close(width);
}
  
Handle<Value> NAVCodecContext::GetHeight(Local<String> property, 
                                 const AccessorInfo& info) {
  HandleScope scope;
  AVCodecContext *pCodecCtx = GetCodecContext(info);
  Handle<Integer> height = Integer::New(pCodecCtx->height);
  return scope.Close(height);
}

Handle<Value> NAVCodecContext::GetBitRate(Local<String> property, 
                                         const AccessorInfo& info) {
  HandleScope scope;
  AVCodecContext *pCodecCtx = GetCodecContext(info);
  Handle<Integer> bit_rate = Integer::New(pCodecCtx->bit_rate);
  return scope.Close(bit_rate);
}

Handle<Value> NAVCodecContext::GetSampleFmt(Local<String> property, 
                                          const AccessorInfo& info) {
  HandleScope scope;
  AVCodecContext *pCodecCtx = GetCodecContext(info);
  Handle<Integer> sample_fmt = Integer::New(pCodecCtx->sample_fmt);
  return scope.Close(sample_fmt);
}
Handle<Value> NAVCodecContext::GetSampleRate(Local<String> property, 
                                             const AccessorInfo& info) {
  HandleScope scope;
  AVCodecContext *pCodecCtx = GetCodecContext(info);
  Handle<Integer> sample_rate = Integer::New(pCodecCtx->sample_rate);
  return scope.Close(sample_rate);
}
Handle<Value> NAVCodecContext::GetChannels(Local<String> property, 
                                          const AccessorInfo& info) {
  HandleScope scope;
  AVCodecContext *pCodecCtx = GetCodecContext(info);
  Handle<Integer> channels = Integer::New(pCodecCtx->channels);
  return scope.Close(channels);
}
Handle<Value> NAVCodecContext::GetFramerate(Local<String> property, 
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

