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

#include "navformat.h"
#include "navframe.h"
#include "navdictionary.h"
#include "navutils.h"

using namespace v8;

NAVFormat::NAVFormat(){
  filename = NULL;
  pFormatCtx = NULL;
}
  
NAVFormat::~NAVFormat(){
  printf("Called NAVFormat destructor\n");
  avformat_close_input(&pFormatCtx);
  free(filename);
}

Persistent<FunctionTemplate> NAVFormat::templ;

void NAVFormat::Init(Handle<Object> target){
  HandleScope scope;
    
  // Our constructor
  Local<FunctionTemplate> templ = FunctionTemplate::New(New);
    
  NAVFormat::templ = Persistent<FunctionTemplate>::New(templ);
    
  NAVFormat::templ->InstanceTemplate()->SetInternalFieldCount(1); // 1 since this is a constructor function
  NAVFormat::templ->SetClassName(String::NewSymbol("NAVFormat"));
    
  NODE_SET_PROTOTYPE_METHOD(NAVFormat::templ, "dump", Dump);
  NODE_SET_PROTOTYPE_METHOD(NAVFormat::templ, "decode", Decode);
    
  // Binding our constructor function to the target variable
  target->Set(String::NewSymbol("NAVFormat"), NAVFormat::templ->GetFunction());    
}

Handle<Value> NAVFormat::New(const Arguments& args) {
  HandleScope scope;
  Local<Object> self = args.This();
    
  NAVFormat* instance = new NAVFormat();
  AVFormatContext *pFormatCtx;
    
  // Wrap our C++ object as a Javascript object
  instance->Wrap(self);
    
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
      streams->Set(i, NAVStream::New(stream));
    }
          
    //instance->streams = scope.Close(streams); // needed?
    SET_KEY_VALUE(self, "streams", streams);
    SET_KEY_VALUE(self, "metadata", NAVDictionary::New(pFormatCtx->metadata));
  }
  return self;
}

// ([streams], cb(stream, frame))
Handle<Value> NAVFormat::Decode(const Arguments& args) {
  HandleScope scope;
  Local<Array> streams;
  Local<Function> callback;

  typedef struct{
    Handle<Object> stream;
    Handle<Object> frame;
    AVStream *pStream;
    AVFrame *pFrame;
  } StreamFrame;
     
  StreamFrame streamFrames[16];
  
  if(!(args[0]->IsArray())){
    return ThrowException(Exception::TypeError(String::New("First parameter must be an array")));
  }  
  if(!(args[1]->IsFunction())){
    return ThrowException(Exception::TypeError(String::New("Second parameter must be a funcion")));
  }
  
  NAVFormat* instance = UNWRAP_OBJECT(NAVFormat, args);
  
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
    pStream = ((NAVStream*)Local<External>::Cast(streamFrames[i].stream->GetInternalField(0))->Value())->pContext;

    streamFrames[i].pStream = pStream;
    streamFrames[i].pFrame = avcodec_alloc_frame();

    Handle<Object> frame = NAVFrame::New(streamFrames[i].pFrame);
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
        streamFrames[i].pFrame->owner = pStream->codec;
        
        argv[2] = Integer::New(instance->pFormatCtx->pb->pos);        
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

Handle<Value> NAVFormat::Dump(const Arguments& args) {
  HandleScope scope;
  
  NAVFormat* instance = UNWRAP_OBJECT(NAVFormat, args);
    
  av_dump_format(instance->pFormatCtx, 0, instance->filename, 0);
    
  return Integer::New(0);
}
