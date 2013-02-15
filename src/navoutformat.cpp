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
#include "navframe.h"
#include "navdictionary.h"
#include "navutils.h"
#include <libavresample/avresample.h>

using namespace v8;

#define VIDEO_BUFFER_SIZE 2000000
#define AUDIO_BUFFER_SIZE 128*1024

void dumpFrame( AVCodecContext *pCodecContext, AVFrame *pFrame){
  printf("pts:%llu\n", pFrame->pts);
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
  
  videoFrame = 0;
  skipVideo = 0;
}

NAVOutputFormat::~NAVOutputFormat(){
  fprintf(stderr, "NAVOutputFormat destructor\n");
  free(filename);
  av_free(pVideoBuffer);
  av_free(pAudioBuffer);
  
  avformat_free_context(pFormatCtx);
    
  delete pFifo;
}

Persistent<FunctionTemplate> NAVOutputFormat::templ;

int NAVOutputFormat::outputAudio(AVFormatContext *pFormatContext,
                                 AVStream *pStream, 
                                 AVFrame *pFrame){
  int gotPacket = 0, ret;
  
  AVCodecContext *pCodecContext = pStream->codec;
  
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
    
    if (packet.pts != (int64_t)AV_NOPTS_VALUE){
      packet.pts = av_rescale_q(packet.pts, 
                                pCodecContext->time_base, 
                                pStream->time_base);
    }
    
    if (packet.duration > 0){
      packet.duration = av_rescale_q(packet.duration,
                                     pCodecContext->time_base, 
                                     pStream->time_base);
    }
    //ret = av_write_frame(pFormatContext, &packet);
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
  
  Local<Array> streams = Array::New(0);
  self->Set(String::NewSymbol("streams"), streams);
  
  if (args.Length()==0){
    return ThrowException(Exception::TypeError(String::New("Not enough parameters")));
  }

  if (!args[0]->IsString()){
    return ThrowException(Exception::TypeError(String::New("Input parameter #0 should be a string")));
  }
  
  String::Utf8Value v8str(args[0]);
  instance->filename = strdup(*v8str);
  if(instance->filename == NULL){
    return ThrowException(Exception::Error(String::New("Error allocating filename string")));
  }
  
  if(args.Length()>1){
    if (!args[1]->IsString()){
      free(instance->filename);
      return ThrowException(Exception::TypeError(String::New("Input parameter #1 should be a string")));
    }
    String::Utf8Value v8codec_name(args[1]);
    codec_name = strdup(*v8codec_name);
  }
  if(args.Length()>2){
    if (!args[2]->IsString()){
      free((void*)codec_name);
      free(instance->filename);
      return ThrowException(Exception::TypeError(String::New("Input parameter #2 should be a string")));
    }
    String::Utf8Value v8mime_type(args[2]);
    mime_type = strdup(*v8mime_type);
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
  
  free((void*)codec_name);
  free((void*)mime_type);
    
  return self;
}

static PixelFormat get_pix_fmt(AVCodec *pCodec, PixelFormat pix_fmt){
  if(pCodec->pix_fmts){
    const PixelFormat *pFormats = pCodec->pix_fmts;
    while(*pFormats != -1){
      if(*pFormats == pix_fmt){
        return pix_fmt;
      }
      pFormats++;
    }
    return pCodec->pix_fmts[0];
  }else{
    return pix_fmt;
  }
}

// (stream_type, [options])
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

  options = Object::New();
  if (args.Length()>1){
    if(args[1]->IsObject()) {
      options = (Local<Object>::Cast(args[1]))->Clone();
    }else {
      return ThrowException(Exception::TypeError(String::New("Input parameter #1 should be an object")));
    }
  }
  
  String::Utf8Value v8streamType(args[0]);
  if(strcmp(*v8streamType, "Video") == 0){
    codec_type = AVMEDIA_TYPE_VIDEO;
    codec_id = instance->pOutputFormat->video_codec;
    instance->skipVideo = GET_OPTION_UINT32(options, skipVideoFrames, 0);
  } else if(strcmp(*v8streamType, "Audio") == 0){
    codec_type = AVMEDIA_TYPE_AUDIO;
    codec_id = instance->pOutputFormat->audio_codec;
  }
  
  codec_id = (CodecID) GET_OPTION_UINT32(options, codec, codec_id);
  
  AVCodec* pCodec = avcodec_find_encoder(codec_id);
  if (!pCodec) {
    return ThrowException(Exception::Error(String::New("Could not find codec")));
  }

  AVStream *pStream = avformat_new_stream(instance->pFormatCtx, pCodec);
  if (!pStream) {
    return ThrowException(Exception::Error(String::New("Could not create stream")));
  }
  AVCodecContext *pCodecContext = pStream->codec;
  avcodec_get_context_defaults3 (pCodecContext, pCodec);
    
  PixelFormat pix_fmt = (PixelFormat) options->Get(String::NewSymbol("pix_fmt"))->Uint32Value();
  
  pCodecContext->pix_fmt = get_pix_fmt(pCodec, pix_fmt);
  
  // TODO: Force dims to multiple of 2! (or maybe even 16) (or just give an error).
  pCodecContext->width = GET_OPTION_UINT32(options, width, 480);
  pCodecContext->height = GET_OPTION_UINT32(options, height, 270); 
  
  fprintf(stderr, "Sample format %i", pCodecContext->sample_fmt);
  
  if(codec_id == AV_CODEC_ID_MP3){
    pCodecContext->sample_fmt = AV_SAMPLE_FMT_S16P; // AV_SAMPLE_FMT_S16
  } else {
    pCodecContext->sample_fmt = AV_SAMPLE_FMT_S16;
  }
  
  pCodecContext->channel_layout = AV_CH_LAYOUT_STEREO;
  
  if(codec_type == AVMEDIA_TYPE_AUDIO){
    instance->pAudioStream = pStream;
  }
  
  // some formats want stream headers to be separate
  if(instance->pFormatCtx->oformat->flags & AVFMT_GLOBALHEADER){
    pCodecContext->flags |= CODEC_FLAG_GLOBAL_HEADER;
  }
  
  AVDictionary *pDict = NAVDictionary::New(options);
  if (avcodec_open2(pCodecContext, pCodec, &pDict) < 0) {
    NAVDictionary::Info(pDict);
    av_dict_free(&pDict);
    return ThrowException(Exception::Error(String::New("Could not open codec")));
  }
  av_dict_free(&pDict);
  
  Local<Array> streams = Local<Array>::Cast(self->Get(String::New("streams")));
  Handle<Value> stream = NAVStream::New(pStream);
  streams->Set(streams->Length(), stream);
    
  return scope.Close(stream);
}

Handle<Value> NAVOutputFormat::Begin(const Arguments& args) {
  HandleScope scope;
  bool hasVideo = false;
  bool hasAudio = false;
  
  NAVOutputFormat* instance = UNWRAP_OBJECT(NAVOutputFormat, args);
  
  av_dump_format(instance->pFormatCtx, 0, instance->filename, 1);
  
  for(unsigned int i=0; i<instance->pFormatCtx->nb_streams;i++){
    AVCodecContext *pCodecContext = instance->pFormatCtx->streams[i]->codec;
    
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

#if 0
const char *NAVOutputFormat::EncodeVideoFrame(AVStream *pStream, AVFrame *pFrame, int *outSize){
  int ret;
  AVCodecContext *pContext = pStream->codec;
  
  if(pFrame){
    pFrame->pts = videoFrame;
    videoFrame++;
  }
  
  if(pFrame == NULL || skipVideo == 0 || (videoFrame % skipVideo) == 0){
    AVPacket packet;
    packet.data = pVideoBuffer;
    packet.size = videoBufferSize;

    av_init_packet(&packet);
    
    packet.stream_index = pStream->index;
    packet.pts = pFrame->pts;
    
    if (pContext->coded_frame && pContext->coded_frame->pts != (int64_t)AV_NOPTS_VALUE){
      packet.pts = av_rescale_q(pContext->coded_frame->pts,
                                  pContext->time_base,
                                  pStream->time_base);
    }
  
    int gotPacket;
    if(avcodec_encode_video2(pContext, &packet, pFrame, &gotPacket) < 0){
      return "Error encoding frame";
    }
  
    if (gotPacket > 0) {
      if(pContext->coded_frame->key_frame){
        packet.flags |= AV_PKT_FLAG_KEY;
      }
      
      ret = av_interleaved_write_frame(pFormatCtx, &packet);
      if (ret) {
        return "Error writing video frame";
      }
    }
  }

  return NULL;
}

#else
const char * NAVOutputFormat::EncodeVideoFrame(AVStream *pStream, AVFrame *pFrame, int *outSize)
{
  int ret;
  AVCodecContext *pContext = pStream->codec;
  
  if(pFrame){
    pFrame->pts = videoFrame;
    videoFrame++;
  }
  
  *outSize = avcodec_encode_video(pContext, pVideoBuffer, videoBufferSize, pFrame);
  
  if(*outSize < 0){
    return "Error encoding frame";
  }
  
  if (*outSize > 0) {
    AVPacket packet;
    av_init_packet(&packet);
      
    if(pContext->coded_frame->key_frame){
      packet.flags |= AV_PKT_FLAG_KEY;
    }
    
    packet.stream_index = pStream->index;
    packet.data = pVideoBuffer;
    packet.size = *outSize;
      
    if (pContext->coded_frame && 
        pContext->coded_frame->pts != (int64_t)AV_NOPTS_VALUE){
      packet.pts= av_rescale_q(pContext->coded_frame->pts, 
                               pContext->time_base, 
                               pStream->time_base);
    }
      
    ret = av_interleaved_write_frame(pFormatCtx, &packet);
    if (ret) {
      return "Error writing video frame";
    }
  }

  return NULL;
}
#endif

static const char *EncodeFrame(NAVOutputFormat* instance, AVStream *pStream, AVFrame *pFrame){
  int ret = 0;
  
  AVCodecContext *pCodecContext = pStream->codec;
  
  switch (pCodecContext->codec_type) {
    case AVMEDIA_TYPE_VIDEO:
      int outSize;
      return instance->EncodeVideoFrame(pStream, pFrame, &outSize);
      break;
    case AVMEDIA_TYPE_AUDIO:
      ret = instance->pFifo->put(pFrame);
      if(ret<0){
        return "Error encoding adding frame to fifo";
      }
    
      while(instance->pFifo->moreFrames()){
        AVFrame *pFifoFrame;
      
        pFifoFrame = instance->pFifo->get();
      
        ret = instance->outputAudio(instance->pFormatCtx,
                                    pStream,
                                    pFifoFrame);
        if(ret<0){
          return "Error outputing audio frame";
        }
      }
      break;
    default:
      break;
  }
  return NULL;
}

struct Baton {
  uv_work_t request;
  Persistent<Function> callback;
  
  // Input
  NAVOutputFormat* pOutputFormat;
  AVStream *pStream;
  AVFrame *pFrame;
  
  // Output
  
  const char *error;
};

static void AsyncWork(uv_work_t* req) {
  
  Baton* pBaton = static_cast<Baton*>(req->data);
  
  pBaton->error = EncodeFrame(pBaton->pOutputFormat,
                              pBaton->pStream,
                              pBaton->pFrame);
  }

static void AsyncAfter(uv_work_t* req) {
  HandleScope scope;
  Baton* pBaton = static_cast<Baton*>(req->data);
    
  Local<Value> argv[1];
  Local<Value> err;
  int numArgs = 0;
  
  if (pBaton->error) {
    err = Exception::Error(String::New(pBaton->error));
    argv[0] = err;
    numArgs = 1;
  }
  
  TryCatch try_catch;
  pBaton->callback->Call(Context::GetCurrent()->Global(), numArgs, argv);
    
  if (try_catch.HasCaught()) {
    node::FatalException(try_catch);
  }
  
  pBaton->callback.Dispose();
  delete pBaton;
}

Handle<Value> NAVOutputFormat::Encode(const Arguments& args) {
  HandleScope scope;

  if(args.Length()<3){
    return ThrowException(Exception::TypeError(String::New("This Function requires 3 parameters")));
  }

  NAVOutputFormat* instance = UNWRAP_OBJECT(NAVOutputFormat, args);
  
  Local<Object> stream = Local<Object>::Cast(args[0]);
  Local<Object> frame = Local<Object>::Cast(args[1]);
  Local<Function> callback = Local<Function>::Cast(args[2]);
  
  AVStream *pStream = node::ObjectWrap::Unwrap<NAVStream>(stream)->pContext;  
  AVFrame *pFrame = (node::ObjectWrap::Unwrap<NAVFrame>(frame))->pContext;
  
  Baton* pBaton = new Baton();
  
  pBaton->pStream = pStream;
  pBaton->pFrame = pFrame;
  pBaton->pOutputFormat = instance;
    
  pBaton->request.data = pBaton;
  
  pBaton->callback = Persistent<Function>::New(callback);
  
  uv_queue_work(uv_default_loop(),
                &(pBaton->request),
                AsyncWork,
                AsyncAfter);
  
  return Undefined();
  
  //return EncodeFrame(instance, pStream, pFrame);
}

Handle<Value> NAVOutputFormat::End(const Arguments& args) {
  HandleScope scope;
  
  Handle<Value> result;
    
  NAVOutputFormat* instance = UNWRAP_OBJECT(NAVOutputFormat, args);
   
  for(unsigned int i=0; i<instance->pFormatCtx->nb_streams;i++){
    AVStream *pStream = instance->pFormatCtx->streams[i];

    // Flush encoders
    if(pStream->codec->codec_type == AVMEDIA_TYPE_VIDEO){
      int outSize;
      do {
        (void)instance->EncodeVideoFrame(pStream, NULL, &outSize);
        if(outSize < 0){
          return ThrowException(Exception::Error(String::New("Error flushing video encoder")));
        }
      } while (outSize > 0);
    }
  }
  
  if((instance->pFifo) && instance->pFifo->dataLeft()){
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
    
  avio_flush(instance->pFormatCtx->pb);
  if (!(instance->pOutputFormat->flags & AVFMT_NOFILE)) {
    avio_close(instance->pFormatCtx->pb);
  }
  
  return Undefined();
}
