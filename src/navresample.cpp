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
#include "navresample.h"

#include "navformat.h"
#include "navframe.h"
#include "navutils.h"

using namespace v8;

// Proper size?
#define AUDIO_BUFFER_SIZE 256*1024

NAVResample::NAVResample(){
  pContext = NULL;
  pFrame = NULL;
  pAudioBuffer = NULL;
  passthrough = true;
}

NAVResample::~NAVResample(){
  audio_resample_close(pContext);
  av_free(pFrame);
  av_free(pAudioBuffer);
}

Persistent<FunctionTemplate> NAVResample::templ;

void NAVResample::Init(Handle<Object> target){
  HandleScope scope;
  
  // Our constructor
  Local<FunctionTemplate> templ = FunctionTemplate::New(New);
  
  NAVResample::templ = Persistent<FunctionTemplate>::New(templ);
  
  NAVResample::templ->InstanceTemplate()->SetInternalFieldCount(1); // 1 since this is a constructor function
  NAVResample::templ->SetClassName(String::NewSymbol("NAVResample"));
  
  NODE_SET_PROTOTYPE_METHOD(NAVResample::templ, "convert", Convert);
  
  target->Set(String::NewSymbol("NAVResample"), NAVResample::templ->GetFunction());    
}

// (srcStream, dstStream, [options])
Handle<Value> NAVResample::New(const Arguments& args) {
  HandleScope scope;
  
  NAVResample* instance = new NAVResample();
  
  instance->Wrap(args.This());
  
  if(args.Length()<2){
    return ThrowException(Exception::TypeError(String::New("Missing input parameters (srcStream, dstStream)")));
  }
  
  Local<Object> stream = Local<Object>::Cast(args[0]);
  AVStream *pSrcStream = ((NAVStream*)Local<External>::Cast(stream->GetInternalField(0))->Value())->pContext;
  
  stream = Local<Object>::Cast(args[1]);
  AVStream *pDstStream = ((NAVStream*)Local<External>::Cast(stream->GetInternalField(0))->Value())->pContext;
  
  instance->pDstStream = pDstStream;
  
  AVCodecContext *pSrcCodec = pSrcStream->codec;
  AVCodecContext *pDstCodec = pDstStream->codec;
  
  printf("Channels %i %i\n", pSrcCodec->channels, pDstCodec->channels);
  printf("Sample Rate %i %i\n", pSrcCodec->sample_rate, pDstCodec->sample_rate);
  printf("Format %i %i\n", pSrcCodec->sample_fmt, pDstCodec->sample_fmt);
  printf("Bitrate %i %i\n", pSrcCodec->bit_rate, pDstCodec->bit_rate);

  if((pSrcCodec->channels != pDstCodec->channels) ||
     (pSrcCodec->sample_rate != pDstCodec->sample_rate) ||
     (pSrcCodec->sample_fmt != pDstCodec->sample_fmt) || // Sample format is irrelevant or not?
     (pSrcCodec->bit_rate > pDstCodec->bit_rate)){  // Bit rate is irrelevant here.
    
    instance->pContext = av_audio_resample_init(pDstCodec->channels, pSrcCodec->channels,
                                                pDstCodec->sample_rate, pSrcCodec->sample_rate,
                                                pDstCodec->sample_fmt, pSrcCodec->sample_fmt,
                                                16, 10, 0, 0.8 );
    if(instance->pContext == NULL) {
      return ThrowException(Exception::TypeError(String::New("Could not init resample context")));
    }
        
    instance->pFrame = avcodec_alloc_frame();
    if (!instance->pFrame){
      return ThrowException(Exception::TypeError(String::New("Error Allocating AVFrame")));
    }
    
    instance->pAudioBuffer = (uint8_t*) av_malloc(AUDIO_BUFFER_SIZE);
    if (!instance->pAudioBuffer){
      return ThrowException(Exception::TypeError(String::New("Error Allocating Audio Buffer")));
    }
    
    instance->passthrough = false;
  }
  
  return args.This();
}

// ([streams], cb(stream, frame))
Handle<Value> NAVResample::Convert(const Arguments& args) {
  HandleScope scope;
  Local<Object> srcFrame;
  Handle<Object> dstFrame;
  
  if(args.Length()<1){
    return ThrowException(Exception::TypeError(String::New("Missing input parameters (srcFrame)")));
  }
  srcFrame = Local<Array>::Cast(args[0]);
  
  NAVResample* instance = UNWRAP_OBJECT(NAVResample, args);
  
  // TODO: check that src frame is really compatible with this converter.
  
  if(instance->passthrough){
    return scope.Close(srcFrame); // Do we really need to close here?
  } else {
    AVFrame *pSrcFrame = 
      ((NAVFrame*)Local<External>::Cast(srcFrame->GetInternalField(0))->Value())->pContext;
  
    AVCodecContext *pCodecContext = instance->pDstStream->codec;
    
    avcodec_get_frame_defaults(instance->pFrame);
    
    instance->pFrame->quality = 1;
    instance->pFrame->pts = pSrcFrame->pts;
    instance->pFrame->format = AV_SAMPLE_FMT_S16;
    
    int nb_samples = audio_resample (instance->pContext, 
                                     (short int*) instance->pAudioBuffer, 
                                     (short int*) pSrcFrame->data[0], 
                                     pSrcFrame->nb_samples);
        
    if(nb_samples<0) {
      return ThrowException(Exception::TypeError(String::New("Failed frame conversion")));
    }
    
    instance->pFrame->nb_samples = nb_samples;
    
    int size = nb_samples * 
               pCodecContext->channels * 
               av_get_bytes_per_sample(pCodecContext->sample_fmt);
        
    int ret = avcodec_fill_audio_frame(instance->pFrame, 
                                       pCodecContext->channels, 
                                       pCodecContext->sample_fmt,
                                       instance->pAudioBuffer, 
                                       size, 
                                       1);
    if(ret<0) {
      return ThrowException(Exception::TypeError(String::New("Failed filling frame")));
    }
    
    instance->pFrame->owner = instance->pDstStream->codec;
    dstFrame = NAVFrame::New(instance->pFrame);
    return scope.Close(dstFrame);
  }
}

