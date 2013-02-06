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

#include <string.h>

#include "navformat.h"
#include "navframe.h"
#include "navdictionary.h"
#include "navutils.h"

#define MAX_NUM_STREAMFRAMES  16

using namespace v8;

class DecoderNotifier;

typedef struct {
  Persistent<Object> stream;
  Persistent<Object> frame;
  AVStream *pStream;
  AVFrame *pFrame;
} StreamFrame;

struct Baton {
  uv_work_t request;
  Persistent<Object> navformat;
  Persistent<Function> callback;
  Persistent<Object> notifier;
  
  // Input
  AVFormatContext *pFormatCtx;
  unsigned int numStreams;
  StreamFrame streamFrames[MAX_NUM_STREAMFRAMES];
  
  // Output
  int result;
  int streamIndex;
  
  const char *error;
};

//
// Decode Frame
//

static int decodeFrame(AVFormatContext *pFormatCtx,
                       unsigned int numStreams,
                       StreamFrame *streamFrames,
                       Baton *pBaton){
  AVPacket packet;
  
  int ret, finished;
  
  pBaton->error = NULL;
  pBaton->streamIndex = -1;
  
  while((ret=av_read_frame(pFormatCtx, &packet))>=0){
  
    unsigned int i;
    int res;
    
    AVStream *pStream = NULL;
    AVFrame *pFrame = NULL;
    
    for(i=0;i<numStreams; i++){
      if (streamFrames[i].pStream->index == packet.stream_index) {
        pStream = streamFrames[i].pStream;
        pFrame = streamFrames[i].pFrame;
        break;
      }
    }

    if(pStream){
      switch(pStream->codec->codec_type){
        case AVMEDIA_TYPE_AUDIO:
          res = avcodec_decode_audio4(pStream->codec, pFrame, &finished, &packet);
          break;
        case AVMEDIA_TYPE_VIDEO:
          res = avcodec_decode_video2(pStream->codec, pFrame, &finished, &packet);
          break;
        default:
          res = -1;
      }
      av_free_packet(&packet);
      
      if(res<0){
        pBaton->error = "Error decoding frame";
        break;
      }
      
      if(finished){
        streamFrames[i].pFrame->owner = pStream->codec;
        pBaton->streamIndex = i;
        pFrame->pts = packet.pts;
        break;
      }
    }
  }
  
  return ret;
}

static void AsyncWork(uv_work_t* req) {
  Baton* pBaton = static_cast<Baton*>(req->data);
  
  decodeFrame(pBaton->pFormatCtx,
              pBaton->numStreams,
              pBaton->streamFrames,
              pBaton);
}

static void CleanUp(Baton *pBaton){
  for(unsigned int i=0;i<pBaton->numStreams; i++){
    avcodec_close(pBaton->streamFrames[i].pStream->codec);
    pBaton->streamFrames[i].stream.Dispose();
    pBaton->streamFrames[i].frame.Dispose();
  }

  pBaton->navformat.Dispose();
  pBaton->callback.Dispose();
  pBaton->notifier.Dispose();
  delete pBaton;
}

static void AsyncAfter(uv_work_t* req) {
  HandleScope scope;
  Baton* pBaton = static_cast<Baton*>(req->data);
  
  if (pBaton->error) {
    Local<Value> err = Exception::Error(String::New(pBaton->error));
    Local<Value> argv[] = { err };
    
    //TryCatch try_catch;
    pBaton->callback->Call(Context::GetCurrent()->Global(), 1, argv);
    
    /*
    if (try_catch.HasCaught()) {
      node::FatalException(try_catch);
    }
    */
    
    CleanUp(pBaton);
  } else {
    if(pBaton->streamIndex >= 0){
       int i = pBaton->streamIndex;
      
      // Call callback with decoded frame
      Handle<Value> argv[] = {
        pBaton->streamFrames[i].stream,
        pBaton->streamFrames[i].frame,
        Integer::New(pBaton->pFormatCtx->pb->pos),
        pBaton->notifier
      };
      
      pBaton->callback->Call(Context::GetCurrent()->Global(), 4, argv);
    }else{
      Local<Value> argv[] = { Local<Value>::New(Null()) };
      pBaton->callback->Call(Context::GetCurrent()->Global(), 1, argv);
      
      // Close all the codecs from the given streams & dispose V8 objects
     CleanUp(pBaton);
    }
  }
}

// DecoderNotifyer

Persistent<ObjectTemplate> DecoderNotifier::templ;

DecoderNotifier::DecoderNotifier(Baton *pBaton){
  this->pBaton = pBaton;
}
  
DecoderNotifier::~DecoderNotifier(){
  printf("DecoderNotifier destructor\n");
}
  
void DecoderNotifier::Init(){
  HandleScope scope;
  
  Local<ObjectTemplate> templ = ObjectTemplate::New();
  templ->SetInternalFieldCount(1);
  
  DecoderNotifier::templ = Persistent<ObjectTemplate>::New(templ);
}

Handle<Object> DecoderNotifier::New(Baton *pBaton) {
  HandleScope scope;
  
  DecoderNotifier *instance = new DecoderNotifier(pBaton);

  Handle<Object> obj = DecoderNotifier::templ->NewInstance();
  instance->Wrap(obj);
  
  SET_KEY_VALUE(obj, "done", FunctionTemplate::New(Done)->GetFunction());

  return scope.Close(obj);
}

Handle<Value> DecoderNotifier::Done(const Arguments& args) {
  HandleScope scope;

  DecoderNotifier* obj = ObjectWrap::Unwrap<DecoderNotifier>(args.This());
  
 if(args.Length() == 0){
    // Process next frame
    uv_queue_work(uv_default_loop(),
                  &(obj->pBaton->request),
                  AsyncWork,
                  AsyncAfter);
  }
  
  return Undefined();
}

//-------------------------------------------------------------------------------------------

NAVFormat::NAVFormat(){
  filename = NULL;
  pFormatCtx = NULL;
}
  
NAVFormat::~NAVFormat(){
  fprintf(stderr, "NAVFormat destructor\n");
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
  AVFormatContext *pFormatCtx;
  
  Local<Object> self = args.This();
    
  NAVFormat* instance = new NAVFormat();
    
  // Wrap our C++ object as a Javascript object
  instance->Wrap(self);
  
  if(args.Length()<1){
    return ThrowException(Exception::TypeError(String::New("Missing input filename")));
  }
  
  String::Utf8Value v8str(args[0]);
  instance->filename = strdup(*v8str);
  if(instance->filename == NULL){
    return ThrowException(Exception::Error(String::New("Error allocating filename string")));
  }
    
  int ret = avformat_open_input(&(instance->pFormatCtx), instance->filename, NULL, NULL);
  if(ret<0){
    return ThrowException(Exception::Error(String::New("Error Opening Intput")));
  }
      
  pFormatCtx = instance->pFormatCtx;
      
  ret = avformat_find_stream_info(pFormatCtx, NULL);
  if(ret<0){
    return ThrowException(Exception::Error(String::New("Error Finding Streams")));
  }
    
  if(pFormatCtx->nb_streams>0){
    Handle<Array> streams = Array::New(pFormatCtx->nb_streams);
    for(unsigned int i=0; i < pFormatCtx->nb_streams;i++){
      AVStream *stream = pFormatCtx->streams[i];
      streams->Set(i, NAVStream::New(stream));
    }
    SET_KEY_VALUE(self, "streams", streams);
    SET_KEY_VALUE(self, "duration", Number::New(pFormatCtx->duration / (float) AV_TIME_BASE));
  }

  SET_KEY_VALUE(self, "metadata", NAVDictionary::New(pFormatCtx->metadata));
  
  return self;
}

// ([streams], cb(stream, frame))
Handle<Value> NAVFormat::Decode(const Arguments& args) {
  HandleScope scope;
  Local<Array> streams;
  Local<Function> callback;
  
  StreamFrame streamFrames[MAX_NUM_STREAMFRAMES];
  
  if(!(args[0]->IsArray())){
    return ThrowException(Exception::TypeError(String::New("First parameter must be an array")));
  }  
  if(!(args[1]->IsFunction())){
    return ThrowException(Exception::TypeError(String::New("Second parameter must be a funcion")));
  }
  
  Local<Object> self = args.This();
  
  NAVFormat* instance = UNWRAP_OBJECT(NAVFormat, args);
  
  streams = Local<Array>::Cast(args[0]);
  callback = Local<Function>::Cast(args[1]);
 
  //
  // Create the required frames and associate every frame to a stream.
  // And open codecs.
  //
  for(unsigned int i=0;i<streams->Length(); i++){
    AVStream *pStream;
  
    Handle<Object> stream = streams->Get(i)->ToObject();
    streamFrames[i].stream = Persistent<Object>::New(stream);
    
    pStream = node::ObjectWrap::Unwrap<NAVStream>(streamFrames[i].stream)->pContext;
    
    streamFrames[i].pStream = pStream;
    streamFrames[i].pFrame = avcodec_alloc_frame();

    Handle<Object> frame = NAVFrame::New(streamFrames[i].pFrame);
    
    streamFrames[i].frame = Persistent<Object>::New(frame);
    
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
  
  Baton* pBaton = new Baton();
  
  pBaton->navformat = Persistent<Object>::New(self);
  pBaton->pFormatCtx = instance->pFormatCtx;
  pBaton->numStreams = streams->Length();
  
  memcpy((void*)&(pBaton->streamFrames), (void*)&streamFrames, sizeof(streamFrames));
  
  pBaton->notifier = Persistent<Object>::New(DecoderNotifier::New(pBaton));
  
  pBaton->request.data = pBaton;
  
  pBaton->callback = Persistent<Function>::New(callback);
  
  uv_queue_work(uv_default_loop(), 
                &pBaton->request,
                AsyncWork, 
                AsyncAfter);

  return Undefined();
}

Handle<Value> NAVFormat::Dump(const Arguments& args) {
  HandleScope scope;
  
  NAVFormat* instance = UNWRAP_OBJECT(NAVFormat, args);
    
  av_dump_format(instance->pFormatCtx, 0, instance->filename, 0);
    
  return Integer::New(0);
}
