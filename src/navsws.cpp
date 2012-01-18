
#include "navsws.h"

#include "avformat.h"
#include "avframe.h"
#include "navutils.h"

using namespace v8;

NAVSws::NAVSws(){
  pContext = NULL;
  pFrame = NULL;
}

NAVSws::~NAVSws(){
  sws_freeContext(pContext);
  av_free(pFrame);
}

Persistent<FunctionTemplate> NAVSws::templ;

void NAVSws::Init(Handle<Object> target){
  HandleScope scope;
  
  // Our constructor
  Local<FunctionTemplate> templ = FunctionTemplate::New(New);
  
  NAVSws::templ = Persistent<FunctionTemplate>::New(templ);
  
  NAVSws::templ->InstanceTemplate()->SetInternalFieldCount(1); // 1 since this is a constructor function
  NAVSws::templ->SetClassName(String::NewSymbol("NAVSws"));
  
  NODE_SET_PROTOTYPE_METHOD(NAVSws::templ, "convert", Convert);
  
  target->Set(String::NewSymbol("NAVSws"), NAVSws::templ->GetFunction());    
}

// (srcStream, dstStream, [options])
Handle<Value> NAVSws::New(const Arguments& args) {
  HandleScope scope;
  
  NAVSws* instance = new NAVSws();
  
  instance->Wrap(args.This());
    
  if(args.Length()<2){
    return ThrowException(Exception::TypeError(String::New("Missing input parameters (srcStream, dstStream)")));
  }

  Local<Object> stream = Local<Object>::Cast(args[0]);
  AVStream *pSrcStream = ((_AVStream*)Local<External>::Cast(stream->GetInternalField(0))->Value())->pContext;
  
  stream = Local<Object>::Cast(args[1]);
  AVStream *pDstStream = ((_AVStream*)Local<External>::Cast(stream->GetInternalField(0))->Value())->pContext;
  
  // TODO: Check if we need a conversor or if we can just do a passthrough
  
  instance->pContext = sws_getContext(pSrcStream->codec->width, 
                                      pSrcStream->codec->height, 
                                      pSrcStream->codec->pix_fmt, 
                                      pDstStream->codec->width,
                                      pDstStream->codec->height,
                                      pDstStream->codec->pix_fmt,
                                      SWS_BICUBIC, // -> put in options.
                                      NULL, NULL, NULL // Filters & Params (unused for now)
                                      );
  if(instance->pContext == NULL) {
    return ThrowException(Exception::TypeError(String::New("Could not init conversion context")));
  }
  
  uint8_t *frameBuffer;
  int frameBufferSize;
  
  instance->pFrame = avcodec_alloc_frame();
  if (!instance->pFrame){
    return ThrowException(Exception::TypeError(String::New("Error Allocating AVFrame")));
  }
  frameBufferSize = avpicture_get_size(pDstStream->codec->pix_fmt, 
                                       pDstStream->codec->width, 
                                       pDstStream->codec->height);
  
  frameBuffer = (uint8_t*) av_malloc(frameBufferSize); // Where is this buffer freed?
  if (!frameBuffer){
    av_free(instance->pFrame);
    instance->pFrame = NULL;
    return ThrowException(Exception::TypeError(String::New("Error Allocating AVFrame buffer")));
  }
  
  avpicture_fill((AVPicture *)instance->pFrame, 
                 frameBuffer,
                 pDstStream->codec->pix_fmt, 
                 pDstStream->codec->width, 
                 pDstStream->codec->height);

  return args.This();
}

// ([streams], cb(stream, frame))
Handle<Value> NAVSws::Convert(const Arguments& args) {
  HandleScope scope;
  Local<Object> srcFrame;
  Handle<Object> dstFrame;
  
  if(args.Length()<1){
    return ThrowException(Exception::TypeError(String::New("Missing input parameters (srcFrame)")));
  }
  
  NAVSws* instance = UNWRAP_OBJECT(NAVSws, args);
  
  srcFrame = Local<Array>::Cast(args[0]);
  
  // TODO: check that src frame is compatible with this converter.
  
  AVFrame *pSrcFrame = ((_AVFrame*)Local<External>::Cast(srcFrame->GetInternalField(0))->Value())->pContext;

  
  int ret = sws_scale(instance->pContext, 
                      pSrcFrame->data, 
                      pSrcFrame->linesize, 
                      0, 
                      instance->pFrame->height, 
                      instance->pFrame->data, 
                      instance->pFrame->linesize);
  if(ret) {
    return ThrowException(Exception::TypeError(String::New("Failed frame conversion")));
  }
  
  dstFrame = _AVFrame::New(instance->pFrame);
  
  return scope.Close(dstFrame);
}

