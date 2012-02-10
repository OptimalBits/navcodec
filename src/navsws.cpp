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
#include "navsws.h"

#include "navformat.h"
#include "navframe.h"
#include "navutils.h"

using namespace v8;

NAVSws::NAVSws(){
  pContext = NULL;
  pFrame = NULL;
  pFrameBuffer = NULL;
  passthrough = true;
}

NAVSws::~NAVSws(){
  printf("NAVSws destructor\n");
  sws_freeContext(pContext);
  av_free(pFrame);
  av_free(pFrameBuffer);
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
  AVStream *pSrcStream = ((NAVStream*)Local<External>::Cast(stream->GetInternalField(0))->Value())->pContext;
  
  stream = Local<Object>::Cast(args[1]);
  AVStream *pDstStream = ((NAVStream*)Local<External>::Cast(stream->GetInternalField(0))->Value())->pContext;
  
  if( (pSrcStream->codec->width != pDstStream->codec->width) ||
      (pSrcStream->codec->height != pDstStream->codec->height) ||
      (pSrcStream->codec->pix_fmt != pDstStream->codec->pix_fmt) ){
  
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

    int frameBufferSize;
  
    instance->pFrame = avcodec_alloc_frame();
    if (!instance->pFrame){
      return ThrowException(Exception::TypeError(String::New("Error Allocating AVFrame")));
    }
    frameBufferSize = avpicture_get_size(pDstStream->codec->pix_fmt, 
                                         pDstStream->codec->width, 
                                         pDstStream->codec->height);
  
    instance->pFrameBuffer = (uint8_t*) av_mallocz(frameBufferSize); // Where is this buffer freed?
    if (!instance->pFrameBuffer ){
      av_freep(&(instance->pFrame));
      return ThrowException(Exception::TypeError(String::New("Error Allocating AVFrame buffer")));
    }
  
    avpicture_fill((AVPicture *)instance->pFrame, 
                   instance->pFrameBuffer,
                   pDstStream->codec->pix_fmt, 
                   pDstStream->codec->width, 
                   pDstStream->codec->height);
    
    instance->pFrame->width = pDstStream->codec->width;
    instance->pFrame->height = pDstStream->codec->height;

    instance->passthrough = false;
  }
  
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
  srcFrame = Local<Array>::Cast(args[0]);
  
  NAVSws* instance = UNWRAP_OBJECT(NAVSws, args);
  
  // TODO: check that src frame is really compatible with this converter.
  
  if(instance->passthrough){
    return scope.Close(srcFrame); // Do we really need to close here?
  } else {
    AVFrame *pSrcFrame = ((NAVFrame*)Local<External>::Cast(srcFrame->GetInternalField(0))->Value())->pContext;
  
    int ret = sws_scale(instance->pContext, 
                        pSrcFrame->data, 
                        pSrcFrame->linesize, 
                        0, 
                        pSrcFrame->height, 
                        instance->pFrame->data, 
                        instance->pFrame->linesize);
    if(ret==0) {
      return ThrowException(Exception::TypeError(String::New("Failed frame conversion")));
    }
  
    dstFrame = NAVFrame::New(instance->pFrame);
    return scope.Close(dstFrame);
  }
}

