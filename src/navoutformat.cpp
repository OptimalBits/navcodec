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
#define AUDIO_BUFFER_SIZE 10000


NAVOutputFormat::NAVOutputFormat(){
  pFormatCtx = NULL;
  pOutputFormat = NULL;
  filename = NULL;
  
  pVideoBuffer = NULL;
  videoBufferSize = 0;
  
  pAudioBuffer = NULL;
  audioBufferSize = 0;
}

NAVOutputFormat::~NAVOutputFormat(){
  // NAVOutputFormat_close_input(&pFormatCtx);
  av_free(pVideoBuffer);
  av_free(pAudioBuffer);
  //av_close_input_file(pFormatCtx);
  avformat_close_input(&pFormatCtx);
  free(filename);
}

Persistent<FunctionTemplate> NAVOutputFormat::templ;

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

    pCodecContext->bit_rate = GET_OPTION_UINT32(options, bit_rate, 128000);
    
    pCodecContext->sample_fmt = AV_SAMPLE_FMT_S16;
    pCodecContext->sample_rate = GET_OPTION_UINT32(options, sample_rate, 44100);
    pCodecContext->channels = GET_OPTION_UINT32(options, channels, 2);;
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
    
    if(pCodecContext->codec_type == AVMEDIA_TYPE_AUDIO){
      hasAudio = true;
    }
  }

  // Do Video or Audio specific initializations...
  if(hasVideo){
    if (!(instance->pFormatCtx->oformat->flags & AVFMT_RAWPICTURE)) {
      if(instance->pVideoBuffer){
        av_free(instance->pVideoBuffer);
      }
      instance->videoBufferSize = VIDEO_BUFFER_SIZE;
      instance->pVideoBuffer = (uint8_t*) av_malloc(instance->videoBufferSize);
    }
  }
  
  if(hasAudio){
    instance->audioBufferSize = AUDIO_BUFFER_SIZE;
    instance->pAudioBuffer = (uint8_t*) av_malloc(instance->audioBufferSize);
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
      AVPacket packet;
      av_init_packet(&packet);
        
      //packet.pts = pCodecContext->coded_frame->pts;
      /*
      if (pCodecContext->coded_frame->pts != (int)AV_NOPTS_VALUE){
        packet.pts = av_rescale_q(pCodecContext->coded_frame->pts, 
                                 pCodecContext->time_base, 
                                 pStream->time_base);
      }
      */
      if(pCodecContext->coded_frame->key_frame){
        packet.flags |= AV_PKT_FLAG_KEY;
      }
      
      packet.stream_index = pStream->index;
      packet.data = instance->pVideoBuffer;
      packet.size = outSize;
    
      // write the compressed frame in the media file
      ret = av_interleaved_write_frame(instance->pFormatCtx, &packet);
    }
  }
  
  if(pCodecContext->codec_type == AVMEDIA_TYPE_AUDIO){
    int gotPacket;
    AVPacket packet;
    av_init_packet(&packet);
    
    packet.size = avcodec_encode_audio(pCodecContext, 
                                       instance->pAudioBuffer, 
                                       instance->audioBufferSize, 
                                       (const short int*)pFrame->data[0]);
    packet.data = instance->pAudioBuffer;
    
    if (pCodecContext->coded_frame && 
        pCodecContext->coded_frame->pts != (int)AV_NOPTS_VALUE){
      packet.pts= av_rescale_q(pCodecContext->coded_frame->pts, 
                               pCodecContext->time_base, 
                               pStream->time_base);
    }
  
    packet.flags |= AV_PKT_FLAG_KEY;
    packet.stream_index = pStream->index;
    
    /*  // For version > 0.8  
     ret = avcodec_encode_audio2(pCodecContext, 
                                 &packet,
                                 pFrame,
                                 &gotPacket);
    
    if((ret == 0) && (gotPacket)){
      ret = av_interleaved_write_frame(instance->pFormatCtx, &packet);
    }
    */
    ret = av_interleaved_write_frame(instance->pFormatCtx, &packet);
  }
  
  if (ret) {
    return ThrowException(Exception::Error(String::New("Error writing frame")));
  }
  
  return Undefined();
}

Handle<Value> NAVOutputFormat::End(const Arguments& args) {
  HandleScope scope;
  
  Local<Object> self = args.This();
  
  NAVOutputFormat* instance = UNWRAP_OBJECT(NAVOutputFormat, args);
  
  // write the trailer, if any.  the trailer must be written
  // before you close the CodecContexts open when you wrote the
  // header; otherwise write_trailer may try to use memory that
  // was freed on av_codec_close()
  av_write_trailer(instance->pFormatCtx);
  
  /* close each codec */
  // For each stream in context
  /*
  {
    avcodec_close(st->codec);
    av_free(picture->data[0]);
    av_free(picture);
    if (tmp_picture) {
      av_free(tmp_picture->data[0]);
      av_free(tmp_picture);
    }
    av_free(video_outbuf);
  }
   
   av_freep(&oc->streams[i]->codec);
   av_freep(&oc->streams[i]);
  */
  
  if (!(instance->pOutputFormat->flags & AVFMT_NOFILE)) {
    avio_close(instance->pFormatCtx->pb);
  }
  
  av_free(instance->pFormatCtx);
  
  return Undefined();
}


