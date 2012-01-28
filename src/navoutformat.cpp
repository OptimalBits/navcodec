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

#undef NDEBUG
#include <assert.h>
#define NDEBUG

#include "navoutformat.h"
#include "avframe.h"

#include "navutils.h"

using namespace v8;

#define VIDEO_BUFFER_SIZE 2000000
#define AUDIO_BUFFER_SIZE 128*1024

void dumpFrame( AVCodecContext *pCodecContext, AVFrame *pFrame){
  printf("pts:%i\n", pFrame->pts);
  printf("quality:%i\n", pFrame->quality);
  printf("type:%i\n", pFrame->type);
  printf("nb_samples:%i\n", pFrame->nb_samples);
  printf("format:%i\n", pFrame->format);
  
  printf("frame_size:%i\n", pCodecContext->frame_size);
  
  if(pCodecContext->codec->capabilities & CODEC_CAP_VARIABLE_FRAME_SIZE){
    printf("VARIABLE_FRAME_SIZE\n");
  }
  if(pCodecContext->codec->capabilities & CODEC_CAP_SMALL_LAST_FRAME){
    printf("SMALL LAST FRAME\n");
  }
  if(pCodecContext->codec->capabilities & CODEC_CAP_DELAY){
    printf("DELAY FRAME\n");
  }
}

NAVOutputFormat::NAVOutputFormat(){
  pFormatCtx = NULL;
  pOutputFormat = NULL;
  filename = NULL;
  
  pVideoBuffer = NULL;
  videoBufferSize = 0;
  
  pAudioBuffer = NULL;
  audioBufferSize = 0;
  
  pAudioStream = NULL;
  
  pFifo = NULL;
}

NAVOutputFormat::~NAVOutputFormat(){
  av_free(pVideoBuffer);
  av_free(pAudioBuffer);
  av_free(pFormatCtx);
  free(filename);
  
  delete pFifo;
    
  printf("Called NAVOutputFormat destructor");
}

Persistent<FunctionTemplate> NAVOutputFormat::templ;

int NAVOutputFormat::outputAudio(AVFormatContext *pFormatContext,
                                 AVStream *pStream, 
                                 AVFrame *pFrame){
  int gotPacket = 0, ret;
  
  AVCodecContext *pCodecContext = pFrame->owner;
  
  AVPacket packet;
  av_init_packet(&packet);
  
  packet.data = pAudioBuffer;
  packet.size = audioBufferSize;
  
  ret = avcodec_encode_audio2(pCodecContext, &packet, pFrame, &gotPacket);
  if(ret<0){
    return ret;
  }
  
  if (gotPacket) {
    packet.flags |= AV_PKT_FLAG_KEY;
    packet.stream_index = pStream->index;
    
    if (packet.pts != AV_NOPTS_VALUE){
      packet.pts = av_rescale_q(packet.pts, 
                                pCodecContext->time_base, 
                                pStream->time_base);
    }
    
    if (packet.duration > 0){
      packet.duration = av_rescale_q(packet.duration, 
                                     pCodecContext->time_base, 
                                     pStream->time_base);
    }
    // ret = av_write_frame(instance->pFormatCtx, &packet);
    ret = av_interleaved_write_frame(pFormatContext, &packet);
  }
  
  return ret;
}


void NAVOutputFormat::Init(Handle<Object> target){
  HandleScope scope;
  
  // Our constructor
  Local<FunctionTemplate> templ = FunctionTemplate::New(New);
  
  NAVOutputFormat::templ = Persistent<FunctionTemplate>::New(templ);
  
  NAVOutputFormat::templ->InstanceTemplate()->SetInternalFieldCount(1);
  NAVOutputFormat::templ->SetClassName(String::NewSymbol("NAVOutputFormat"));

  NODE_SET_PROTOTYPE_METHOD(NAVOutputFormat::templ, "addStream", AddStream);
  NODE_SET_PROTOTYPE_METHOD(NAVOutputFormat::templ, "begin", Begin);
  NODE_SET_PROTOTYPE_METHOD(NAVOutputFormat::templ, "encode", Encode);
  NODE_SET_PROTOTYPE_METHOD(NAVOutputFormat::templ, "end", End);
  
  // Binding our constructor function to the target variable
  target->Set(String::NewSymbol("NAVOutputFormat"), NAVOutputFormat::templ->GetFunction());    
}

Handle<Value> NAVOutputFormat::New(const Arguments& args) {
  HandleScope scope;
  
  Local<Object> self = args.This();

  const char *codec_name = NULL;
  const char *mime_type = NULL;
  
  NAVOutputFormat* instance = new NAVOutputFormat();
  instance->Wrap(self);
  
  instance->pFormatCtx = NULL;
  instance->pOutputFormat = NULL;
  
  Local<Array> streams = Array::New(0);
  self->Set(String::NewSymbol("streams"), streams);
  
  if (args.Length()==0){
    return ThrowException(Exception::TypeError(String::New("Not enough parameters")));
  }

  if (!args[0]->IsString()){
    return ThrowException(Exception::TypeError(String::New("Input parameter #0 should be a string")));
  }
  
  String::Utf8Value v8str(args[0]);
  instance->filename = (char*) malloc(strlen(*v8str)+1);
  strcpy(instance->filename, *v8str);
  
  if(args.Length()>1){
    if (!args[1]->IsString()){
      return ThrowException(Exception::TypeError(String::New("Input parameter #1 should be a string")));
    }
    String::Utf8Value v8codec_name(args[1]);
    codec_name = *v8codec_name;
  }
  if(args.Length()>2){
    if (!args[2]->IsString()){
      return ThrowException(Exception::TypeError(String::New("Input parameter #2 should be a string")));
    }
    String::Utf8Value v8mime_type(args[2]);
    mime_type = *v8mime_type;
  }
  
  instance->pOutputFormat = 
    av_guess_format(codec_name, instance->filename, mime_type);
  
  if(!instance->pOutputFormat){
    return ThrowException(Exception::Error(String::New("Could not find suitable output format")));
  }
  
  instance->pFormatCtx = avformat_alloc_context();
  if(!instance->pFormatCtx){
    return ThrowException(Exception::Error(String::New("Could not alloc format context")));
  }
  
  instance->pFormatCtx->oformat = instance->pOutputFormat;
  
  av_dump_format(instance->pFormatCtx, 0, instance->filename, 1);
  
  return self;
}

// (stream_type, [codecId])
Handle<Value> NAVOutputFormat::AddStream(const Arguments& args) {
  HandleScope scope;
  
  Local<Object> options;
  Local<Object> self = args.This();
  
  AVMediaType codec_type;
  CodecID codec_id;
  
  NAVOutputFormat* instance = UNWRAP_OBJECT(NAVOutputFormat, args);
  
  if (args.Length()<1){
    return ThrowException(Exception::TypeError(String::New("Not enough parameters")));
  }
  
  if (!args[0]->IsString()){
    return ThrowException(Exception::TypeError(String::New("Input parameter #0 should be a string")));
  }

  String::Utf8Value v8streamType(args[0]);
  
  if(strcmp(*v8streamType, "Video") == 0){
    codec_type = AVMEDIA_TYPE_VIDEO;
    codec_id = instance->pOutputFormat->video_codec;
  } else if(strcmp(*v8streamType, "Audio") == 0){
    codec_type = AVMEDIA_TYPE_AUDIO;
    codec_id = instance->pOutputFormat->audio_codec;
  }

  options = Object::New();
  if (args.Length()>1){
    if(args[1]->IsObject()) {
      options = Local<Object>::Cast(args[1]);
    }else {
      return ThrowException(Exception::TypeError(String::New("Input parameter #1 should be an object")));
    }
  }

  AVCodecContext *pCodecContext;
  AVStream *pStream;
  
  pStream = avformat_new_stream(instance->pFormatCtx, NULL);
  if (!pStream) {
    return ThrowException(Exception::Error(String::New("Could not create stream")));
  }
  
  pCodecContext = pStream->codec;  
  
  pCodecContext->codec_id = (CodecID) GET_OPTION_UINT32(options, codec, codec_id);
  
  if(codec_type == AVMEDIA_TYPE_VIDEO){
    pCodecContext->codec_type = AVMEDIA_TYPE_VIDEO;
        
    pCodecContext->bit_rate = GET_OPTION_UINT32(options, bit_rate, 400000);
    
    // TODO: Force dims to multiple of 2! (or maybe even 16) (or just give an error).
    pCodecContext->width = GET_OPTION_UINT32(options, width, 480);
    pCodecContext->height = GET_OPTION_UINT32(options, height, 270);
    
    // time base: this is the fundamental unit of time (in seconds) in terms
    // of which frame timestamps are represented. for fixed-fps content,
    // timebase should be 1/framerate and timestamp increments should be
    // identically 1.
    
    if(options->Has(String::New("time_base"))){
      Local<Object> timeBase = 
        Local<Object>::Cast(options->Get(String::New("time_base")));
      
      pCodecContext->time_base.num = GET_OPTION_UINT32(timeBase, num, 1);
      pCodecContext->time_base.den = GET_OPTION_UINT32(timeBase, den, 25);
    }else {
      pCodecContext->time_base.num = 1;
      pCodecContext->time_base.den = GET_OPTION_UINT32(options, framerate, 25);
    }

    pCodecContext->ticks_per_frame = GET_OPTION_UINT32(options, ticks_per_frame, 1);
    
    // emit one intra frame every gop_size frames at most
    pCodecContext->gop_size = GET_OPTION_UINT32(options, gop_size, 12);
    
    pCodecContext->pix_fmt = (PixelFormat) (GET_OPTION_UINT32(options, pix_fmt, PIX_FMT_YUV420P));
    
    if (pCodecContext->codec_id == CODEC_ID_MPEG2VIDEO) {
      // just for testing, we also add B frames
      pCodecContext->max_b_frames = 2;
    }
    if (pCodecContext->codec_id == CODEC_ID_MPEG1VIDEO){
      // Needed to avoid using macroblocks in which some coeffs overflow.
      // This does not happen with normal video, it just happens here as
      // the motion of the chroma plane does not match the luma plane.
      pCodecContext->mb_decision=2;
    }
   }
  
  if(codec_type == AVMEDIA_TYPE_AUDIO){
    pCodecContext->codec_type = AVMEDIA_TYPE_AUDIO;
    pCodecContext->sample_fmt = AV_SAMPLE_FMT_S16;
    
    pCodecContext->bit_rate = GET_OPTION_UINT32(options, bit_rate, 128000);
    pCodecContext->sample_rate = GET_OPTION_UINT32(options, sample_rate, 44100);
    pCodecContext->channels = GET_OPTION_UINT32(options, channels, 2);
    
    instance->pAudioStream = pStream;
  }
  
  // some formats want stream headers to be separate
  if(instance->pFormatCtx->oformat->flags & AVFMT_GLOBALHEADER){
    pCodecContext->flags |= CODEC_FLAG_GLOBAL_HEADER;
  }
  
  Local<Array> streams = Local<Array>::Cast(self->Get(String::New("streams")));
  Handle<Value> stream = _AVStream::New(pStream);
  
  streams->Set(streams->Length(),stream);
  
  return stream;
}

Handle<Value> NAVOutputFormat::Begin(const Arguments& args) {
  HandleScope scope;
  bool hasVideo = false;
  bool hasAudio = false;
  
  Local<Object> self = args.This();
  
  NAVOutputFormat* instance = UNWRAP_OBJECT(NAVOutputFormat, args);
   
  for(unsigned int i=0; i<instance->pFormatCtx->nb_streams;i++){
    AVStream *pStream;
    AVCodec *pCodec;
    AVCodecContext *pCodecContext;
  
    pStream = instance->pFormatCtx->streams[i];
    pCodecContext = pStream->codec;
    
    pCodec = avcodec_find_encoder(pCodecContext->codec_id);
    if (!pCodec) {
      return ThrowException(Exception::Error(String::New("Could not find codec")));
    }
  
    if (avcodec_open2(pCodecContext, pCodec, NULL) < 0) {
      return ThrowException(Exception::Error(String::New("Could not open codec")));
    }
    
    if(pCodecContext->codec_type == AVMEDIA_TYPE_VIDEO){
      hasVideo = true;
    }
    
    if((pCodecContext->codec_type == AVMEDIA_TYPE_AUDIO)&&!hasAudio){
      hasAudio = true;  
      instance->pFifo = new NAVAudioFifo(pCodecContext);
      if (!instance->pFifo) {
        return ThrowException(Exception::Error(String::New("Could not alloc audio fifo")));
      }
    }
  }

  // Do Video or Audio specific initializations...
  // Currently we assume max one stream for audio and one for video.
  if(hasVideo){
    if (!(instance->pFormatCtx->oformat->flags & AVFMT_RAWPICTURE)) {
      av_free(instance->pVideoBuffer);
      instance->videoBufferSize = VIDEO_BUFFER_SIZE;
      instance->pVideoBuffer = (uint8_t*) av_malloc(instance->videoBufferSize);
      if (!instance->pVideoBuffer) {
        return ThrowException(Exception::Error(String::New("Could not alloc video buffer")));
      }
    }
  }
  
  if(hasAudio){
    instance->audioBufferSize = AUDIO_BUFFER_SIZE;
    av_free(instance->pAudioBuffer);
    instance->pAudioBuffer = (uint8_t*) av_malloc(instance->audioBufferSize);
    if (!instance->pAudioBuffer) {
      return ThrowException(Exception::Error(String::New("Could not alloc audio buffer")));
    }
  }
  
  // --
  // open the output file, if needed
  if (!(instance->pOutputFormat->flags & AVFMT_NOFILE)) {
    if (avio_open(&(instance->pFormatCtx->pb), instance->filename, AVIO_FLAG_WRITE) < 0) {
      return ThrowException(Exception::Error(String::New("Could not open output file")));
    }
  }
  
  avformat_write_header(instance->pFormatCtx, NULL);
  
  return Undefined();
}

Handle<Value> NAVOutputFormat::Encode(const Arguments& args) {
  HandleScope scope;
  int gotPacket = 0;
  AVPacket packet;
  
  int ret = 0;

  if(args.Length()<2){
    return ThrowException(Exception::TypeError(String::New("This Function requires 2 parameters")));
  }

  NAVOutputFormat* instance = UNWRAP_OBJECT(NAVOutputFormat, args);
  
  Local<Object> stream = Local<Object>::Cast(args[0]);
  Local<Object> frame = Local<Object>::Cast(args[1]);
  
  AVStream *pStream = ((_AVStream*)Local<External>::Cast(stream->GetInternalField(0))->Value())->pContext;
  AVFrame *pFrame = ((_AVFrame*)Local<External>::Cast(frame->GetInternalField(0))->Value())->pContext;

  AVCodecContext *pCodecContext = pStream->codec;
  
  if((pCodecContext->codec_type != AVMEDIA_TYPE_VIDEO) && 
     (pCodecContext->codec_type != AVMEDIA_TYPE_AUDIO)){
    return Undefined();
  }
  
  if(pCodecContext->codec_type == AVMEDIA_TYPE_VIDEO){
    int outSize = avcodec_encode_video(pCodecContext, 
                                       instance->pVideoBuffer, 
                                       instance->videoBufferSize, 
                                       pFrame);
    if(outSize < 0){
      return ThrowException(Exception::Error(String::New("Error encoding frame")));
    }
  
    // if zero size, it means the image was buffered
    if (outSize > 0) {
      av_init_packet(&packet);

      if(pCodecContext->coded_frame->key_frame){
        packet.flags |= AV_PKT_FLAG_KEY;
      }
      
      packet.stream_index = pStream->index;
      packet.data = instance->pVideoBuffer;
      packet.size = outSize;
      
      if (pCodecContext->coded_frame && 
          pCodecContext->coded_frame->pts != (int)AV_NOPTS_VALUE){
        packet.pts= av_rescale_q(pCodecContext->coded_frame->pts, 
                                 pCodecContext->time_base, 
                                 pStream->time_base);
      }
      
      ret = av_interleaved_write_frame(instance->pFormatCtx, &packet);
      if (ret) {
        return ThrowException(Exception::Error(String::New("Error writing frame")));
      }
    }
  }
  
  if(pCodecContext->codec_type == AVMEDIA_TYPE_AUDIO){    
    ret = instance->pFifo->put(pFrame);
    if(ret<0){
      return ThrowException(Exception::Error(String::New("Error encoding adding frame to fifo")));
    }
    
    while(instance->pFifo->moreFrames()){
      AVFrame *pFifoFrame;
      
      pFifoFrame = instance->pFifo->get();
      
      ret = instance->outputAudio(instance->pFormatCtx, 
                                  pStream, 
                                  pFifoFrame);
      if(ret<0){
        return ThrowException(Exception::Error(String::New("Error outputing audio frame")));
      }
    }
  }
  
  return Undefined();
}

Handle<Value> NAVOutputFormat::End(const Arguments& args) {
  HandleScope scope;
  
  Local<Object> self = args.This();
  
  NAVOutputFormat* instance = UNWRAP_OBJECT(NAVOutputFormat, args);

  if(instance->pFifo->dataLeft()){
    AVFrame *pFifoFrame;
    
    pFifoFrame = instance->pFifo->getLast();
    dumpFrame(pFifoFrame->owner, pFifoFrame);
    
    if(pFifoFrame->owner->codec->capabilities & CODEC_CAP_SMALL_LAST_FRAME){
      int ret = instance->outputAudio(instance->pFormatCtx, 
                                      instance->pAudioStream,
                                      pFifoFrame);
      if(ret<0){
        return ThrowException(Exception::Error(String::New("Error outputing audio frame")));
      }
    }
  }
  
  av_write_trailer(instance->pFormatCtx);
  
  Local<Array> streams = Local<Array>::Cast(self->Get(String::New("streams")));
  
  for(unsigned int i=0; i<streams->Length();i++){
    Local<Object> stream = Local<Object>::Cast(streams->Get(i));
    AVStream *pStream = ((_AVStream*)Local<External>::Cast(stream->GetInternalField(0))->Value())->pContext;

    avcodec_close(pStream->codec);
    av_free(pStream->codec);
    av_free(pStream);
  }
    
  if (!(instance->pOutputFormat->flags & AVFMT_NOFILE)) {
    avio_close(instance->pFormatCtx->pb);
  }
  
  av_free(instance->pFormatCtx);
  instance->pFormatCtx = NULL;
  
  return Undefined();
}


