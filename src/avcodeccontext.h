
#ifndef _AVCODECCONTEXT_H
#define _AVCODECCONTEXT_H

#include <v8.h>
#include <node.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

using namespace v8;

void NoopSet(Local<String> property, Local<Value> value, 
             const AccessorInfo& info);

class _AVCodecContext  {
private:
  AVCodecContext *pContext;
  
public:
  _AVCodecContext();
  ~_AVCodecContext();
  
  static Persistent<ObjectTemplate> templ;
  
  static void Init();
  
  static Handle<Object> New(AVCodecContext *pContext);
  
  static Handle<Value> Open(const Arguments& args);
  
  static Handle<Value> GetType(Local<String> property, 
                               const AccessorInfo& info);
  
  static Handle<Value> GetWidth(Local<String> property, 
                                const AccessorInfo& info);
  
  static Handle<Value> GetHeight(Local<String> property, 
                                 const AccessorInfo& info);

  static Handle<Value> GetBitRate(Local<String> property, 
                                  const AccessorInfo& info);
  
  static Handle<Value> GetFramerate(Local<String> property, 
                                    const AccessorInfo& info);
};

#endif // _AVCODECCONTEXT_H