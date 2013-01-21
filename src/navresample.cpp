/* Copyright(c) 2012-2013 Optimal Bits Sweden AB. All rights reserved.
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

// Cheap layout guessing
int numChannesToLayout(int numChannels){
  fprintf(stderr, "num channels: %i", numChannels);
  switch(numChannels){
    case 2: return AV_CH_LAYOUT_STEREO;
    case 3: return AV_CH_LAYOUT_2POINT1;
    case 4: return AV_CH_LAYOUT_3POINT1;
    case 5: return AV_CH_LAYOUT_4POINT1;
    case 6: return AV_CH_LAYOUT_5POINT1;
    case 7: return AV_CH_LAYOUT_6POINT1;
    case 8: return AV_CH_LAYOUT_7POINT1;
  }
  return AV_CH_LAYOUT_STEREO;
}

NAVResample::NAVResample(){
  pContext = NULL;
  pFrame = NULL;
  pAudioBuffer = NULL;
  passthrough = true;
}

NAVResample::~NAVResample(){
  printf("NAVResample destructor\n");
  
  avresample_free(&pContext);
  
  frame.Dispose();
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
  Local<Object> self = args.This();
  
  NAVResample* instance = new NAVResample();
  
  instance->Wrap(self);
  
  if(args.Length()<2){
    return ThrowException(Exception::TypeError(String::New("Missing input parameters (srcStream, dstStream)")));
  }
  
  Local<Object> stream = Local<Object>::Cast(args[0]);
  AVStream *pSrcStream = (node::ObjectWrap::Unwrap<NAVStream>(stream))->pContext;
  
  stream = Local<Object>::Cast(args[1]);
  AVStream *pDstStream = (node::ObjectWrap::Unwrap<NAVStream>(stream))->pContext;

  instance->pSrcStream = pSrcStream;
  instance->pDstStream = pDstStream;
  
  AVCodecContext *pSrcCodec = pSrcStream->codec;
  AVCodecContext *pDstCodec = pDstStream->codec;
  
  if((pSrcCodec->channels != pDstCodec->channels) ||
     (pSrcCodec->sample_rate != pDstCodec->sample_rate) ||
     (pSrcCodec->sample_fmt != pDstCodec->sample_fmt) || // Sample format is irrelevant or not?
     (pSrcCodec->bit_rate > pDstCodec->bit_rate)){
    
    instance->pContext = avresample_alloc_context();
    if(instance->pContext == NULL) {
      return ThrowException(Exception::TypeError(String::New("Could not init resample context")));
    }
  
    {
      AVAudioResampleContext *avr  = instance->pContext;
      
      // TODO: Decide Channel layout based on input and output number of channels.
      av_opt_set_int(avr, "in_channel_layout",  numChannesToLayout(pSrcCodec->channels), 0);
      av_opt_set_int(avr, "out_channel_layout", numChannesToLayout(pDstCodec->channels), 0);
    
      av_opt_set_int(avr, "in_sample_rate", pSrcCodec->sample_rate, 0);
      av_opt_set_int(avr, "in_sample_fmt",  pSrcCodec->sample_fmt, 0);

      av_opt_set_int(avr, "out_sample_rate", pDstCodec->sample_rate, 0);
      av_opt_set_int(avr, "out_sample_fmt", pDstCodec->sample_fmt, 0);
    
      if(avresample_open(instance->pContext)){
        return ThrowException(Exception::TypeError(String::New("Could not open resampler")));
      }
    }
    
    instance->pFrame = avcodec_alloc_frame();
    if (!instance->pFrame){
      return ThrowException(Exception::TypeError(String::New("Error Allocating AVFrame")));
    }
  
    instance->frame = Persistent<Object>::New(NAVFrame::New(instance->pFrame));
    instance->passthrough = false;
  }
  
  return self;
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
    
  if(instance->passthrough){
    return srcFrame;
  } else {
    AVFrame *pSrcFrame = (node::ObjectWrap::Unwrap<NAVFrame>(srcFrame))->pContext;
  
    AVCodecContext *pCodecContext = instance->pDstStream->codec;
    
    avcodec_get_frame_defaults(instance->pFrame);
    
    instance->pFrame->quality = 1;
    instance->pFrame->pts = pSrcFrame->pts;
    //instance->pFrame->format = AV_SAMPLE_FMT_S16; // needed?
        
    {
      AVAudioResampleContext *avr  = instance->pContext;
      
      uint8_t *output;
      int out_linesize;
      
      int nb_samples = avresample_available(avr) +
                       av_rescale_rnd(avresample_get_delay(avr) +
                                      pSrcFrame->nb_samples,
                                      pCodecContext->sample_rate,
                                      instance->pSrcStream->codec->sample_rate,
                                      AV_ROUND_UP);
      av_samples_alloc(&output,
                       &out_linesize,
                       pCodecContext->channels,
                       nb_samples,
                       pCodecContext->sample_fmt,
                       0);
      if(output == NULL) {
        return ThrowException(Exception::TypeError(String::New("Failed allocating output samples buffer")));
      }
      
      nb_samples = avresample_convert(avr,
                                      &output,
                                      out_linesize,
                                      nb_samples,
                                      pSrcFrame->data,
                                      pSrcFrame->linesize[0],
                                      pSrcFrame->nb_samples);
      int size = nb_samples * 
                 pCodecContext->channels *
                 av_get_bytes_per_sample(pCodecContext->sample_fmt);
/*
      needed_size = av_samples_get_buffer_size(NULL,
       pCodecContext->channels,
       frame->nb_samples, sample_fmt,
*/
      instance->pFrame->nb_samples = nb_samples;
      
      int ret = avcodec_fill_audio_frame(instance->pFrame,
                                        pCodecContext->channels,
                                        pCodecContext->sample_fmt,
                                        output,
                                        size,
                                        1);
      if(ret<0) {
        fprintf(stderr, "avcodec_fill_audio_frame returned %i\n", ret);
        return ThrowException(Exception::TypeError(String::New("Failed filling frame")));
      }
      av_freep(&output);
    }
  
    instance->pFrame->owner = instance->pDstStream->codec;
    
    return instance->frame;
  }
}
// Available layouts:
/*
   AV_CH_LAYOUT_5POINT1
   AV_CH_LAYOUT_2_1
   AV_CH_LAYOUT_2_2
   AV_CH_LAYOUT_2POINT1
   AV_CH_LAYOUT_3POINT1
   AV_CH_LAYOUT_4POINT0
   AV_CH_LAYOUT_4POINT1
   AV_CH_LAYOUT_5POINT0
   AV_CH_LAYOUT_5POINT0_BACK
   AV_CH_LAYOUT_5POINT1
   AV_CH_LAYOUT_5POINT1_BACK
   AV_CH_LAYOUT_6POINT0
   AV_CH_LAYOUT_6POINT0_FRONT
   AV_CH_LAYOUT_6POINT1
   AV_CH_LAYOUT_6POINT1_BACK
   AV_CH_LAYOUT_6POINT1_FRONT
   AV_CH_LAYOUT_7POINT0
   AV_CH_LAYOUT_7POINT0_FRONT
   AV_CH_LAYOUT_7POINT1
   AV_CH_LAYOUT_7POINT1_WIDE
   AV_CH_LAYOUT_7POINT1_WIDE_BACK
   AV_CH_LAYOUT_HEXAGONAL
   AV_CH_LAYOUT_MONO
   AV_CH_LAYOUT_OCTAGONAL
   AV_CH_LAYOUT_QUAD
   AV_CH_LAYOUT_STEREO
   AV_CH_LAYOUT_STEREO_DOWNMIX
   AV_CH_LAYOUT_SURROUND
*/



