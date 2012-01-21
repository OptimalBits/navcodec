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

#include "avformat.h"
#include "avframe.h"
#include "navutils.h"

using namespace v8;

AVFormat::AVFormat(){
  filename = NULL;
  pFormatCtx = NULL;
}
  
AVFormat::~AVFormat(){
  avformat_close_input(&pFormatCtx);
  //av_close_input_file(pFormatCtx);
  free(filename);
}

Persistent<FunctionTemplate> AVFormat::templ;

void AVFormat::Init(Handle<Object> target){
  HandleScope scope;
    
  // Our constructor
  Local<FunctionTemplate> templ = FunctionTemplate::New(New);
    
  AVFormat::templ = Persistent<FunctionTemplate>::New(templ);
    
  AVFormat::templ->InstanceTemplate()->SetInternalFieldCount(1); // 1 since this is a constructor function
  AVFormat::templ->SetClassName(String::NewSymbol("AVFormat"));
    
  NODE_SET_PROTOTYPE_METHOD(AVFormat::templ, "dump", Dump);
  NODE_SET_PROTOTYPE_METHOD(AVFormat::templ, "decode", Decode);
    
  // Binding our constructor function to the target variable
  target->Set(String::NewSymbol("AVFormat"), AVFormat::templ->GetFunction());    
}

Handle<Value> AVFormat::New(AVFormatContext *pContext){

  return Undefined();
}

Handle<Value> AVFormat::New(const Arguments& args) {
  HandleScope scope;
    
  AVFormat* instance = new AVFormat();
  AVFormatContext *pFormatCtx;
    
  // Wrap our C++ object as a Javascript object
  instance->Wrap(args.This());
    
  instance->pFormatCtx = NULL;
    
  String::Utf8Value v8str(args[0]);
  instance->filename = (char*) malloc(strlen(*v8str)+1);
  strcpy(instance->filename, *v8str);
  
  // TODO: Thrown errors!
  int ret = avformat_open_input(&(instance->pFormatCtx), instance->filename, NULL, NULL);
  if(ret<0){
    return ThrowException(Exception::TypeError(String::New("Error Opening Intput")));
  }
      
  pFormatCtx = instance->pFormatCtx;
      
  ret = avformat_find_stream_info(pFormatCtx, NULL);
  if(ret<0){
    return ThrowException(Exception::TypeError(String::New("Error Finding Streams")));
  }
    
  if(pFormatCtx->nb_streams>0){
    Handle<Array> streams = Array::New(pFormatCtx->nb_streams);
          
    for(unsigned int i=0; i < pFormatCtx->nb_streams;i++){
      AVStream *stream = pFormatCtx->streams[i];
      streams->Set(i, _AVStream::New(stream));
    }
          
    //instance->streams = scope.Close(streams); // needed?
    args.This()->Set(String::NewSymbol("streams"), streams);
  }
  return args.This();
}

// ([streams], cb(stream, frame))
Handle<Value> AVFormat::Decode(const Arguments& args) {
  HandleScope scope;
  Local<Array> streams;
  Local<Function> callback;

  typedef struct{
    Handle<Object> stream;
    Handle<Object> frame;
    AVStream *pStream;
    AVFrame *pFrame;
    float duration;
  } StreamFrame;
     
  StreamFrame streamFrames[16];
  
  if(!(args[0]->IsArray())){
    return ThrowException(Exception::TypeError(String::New("First parameter must be an array")));
  }  
  if(!(args[1]->IsFunction())){
    return ThrowException(Exception::TypeError(String::New("Second parameter must be a funcion")));
  }
  
  AVFormat* instance = UNWRAP_OBJECT(AVFormat, args);
  
  streams = Local<Array>::Cast(args[0]);
  callback = Local<Function>::Cast(args[1]);
  
  Handle<Value> argv[3];
 
  //
  // Create the required frames and associate every frame to a stream.
  // And open codecs.
  //
  for(unsigned int i=0;i<streams->Length(); i++){
    AVStream *pStream;
    
    streamFrames[i].stream = streams->Get(i)->ToObject();
    pStream = ((_AVStream*)Local<External>::Cast(streamFrames[i].stream->GetInternalField(0))->Value())->pContext;

    streamFrames[i].pStream = pStream;
    streamFrames[i].pFrame = avcodec_alloc_frame();
    streamFrames[i].duration = pStream->duration * 
                               pStream->time_base.num /
                               (float)pStream->time_base.den;

    Handle<Object> frame = _AVFrame::New(streamFrames[i].pFrame);
    streamFrames[i].frame = frame;
    
    AVCodecContext *pCodecCtx = streamFrames[i].pStream->codec;
    
    AVCodec *pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
    if(pCodec==NULL){
      return ThrowException(Exception::Error(String::New("Could not find decoder!")));
    }
    
    if(avcodec_open2(pCodecCtx, pCodec, NULL)<0){
      return ThrowException(Exception::Error(String::New("Could not open decoder!")));
    }
  }
  
  //
  // Start decoding
  //
  AVPacket packet;
  int ret, finished;
  
  while ((ret=av_read_frame(instance->pFormatCtx, &packet))>=0) {
    unsigned int i;
    int res;
    
    AVStream *pStream = NULL;
    AVFrame *pFrame = NULL;
    
    for(i=0;i<streams->Length(); i++){
      if (streamFrames[i].pStream->index == packet.stream_index) {
        pStream = streamFrames[i].pStream;
        pFrame = streamFrames[i].pFrame;
        break;
      }
    }
        
    if(pStream){
      res = -1;
      
      if(pStream->codec->codec_type == AVMEDIA_TYPE_AUDIO){
        res = avcodec_decode_audio4(pStream->codec, pFrame, &finished, &packet);
      }
      if(pStream->codec->codec_type == AVMEDIA_TYPE_VIDEO){
        res = avcodec_decode_video2(pStream->codec, pFrame, &finished, &packet);
      }
      
      if(res<0){
        return ThrowException(Exception::Error(String::New("Error decoding frame")));
      }
      
      if(finished){
        argv[0] = streamFrames[i].stream;
        argv[1] = streamFrames[i].frame;
        
        float currentTime = pFrame->pkt_pts* pStream->time_base.num / 
                            (float) pStream->time_base.den;
        
        argv[2] = Number::New(currentTime/streamFrames[i].duration);
        
        callback->Call(Context::GetCurrent()->Global(), 3, argv);
      }
    }
    
    av_free_packet(&packet);
  }
  
  // Close all the codecs from the given streams.
  if (ret<0) {
    Local<Value> err = Exception::Error(String::New("Error reading frame"));
    Local<Value> argv[] = { err };
    callback->Call(Context::GetCurrent()->Global(), 1, argv);
  } else {
    Local<Value> argv[] = { Local<Value>::New(Null()) };
    callback->Call(Context::GetCurrent()->Global(), 1, argv);
  }
  
  // Clean up
  for(unsigned int i=0;i<streams->Length(); i++){
    av_free(streamFrames[i].pFrame);
    avcodec_close(streamFrames[i].pStream->codec);
  }

  return Integer::New(0);
}

Handle<Value> AVFormat::Dump(const Arguments& args) {
  HandleScope scope;
  
  AVFormat* instance = UNWRAP_OBJECT(AVFormat, args);
    
  av_dump_format(instance->pFormatCtx, 0, instance->filename, 0);
    
  return Integer::New(0);
}
