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
#include "navthumbnail.h"

#include "navformat.h"
#include "navframe.h"
#include "navutils.h"

using namespace v8;


#define MAX_BPP 4

NAVThumbnail::NAVThumbnail(){
  pContext = NULL;
  pBuffer = NULL;  
}

NAVThumbnail::~NAVThumbnail(){
  printf("NAVThumbnail destructor\n");
  av_free(pBuffer);
  if(pContext){
    avcodec_close(pContext);
    av_free(pContext);
  }
}

Persistent<FunctionTemplate> NAVThumbnail::templ;

void NAVThumbnail::Init(Handle<Object> target){
  HandleScope scope;
  
  // Our constructor
  Local<FunctionTemplate> templ = FunctionTemplate::New(New);
  
  NAVThumbnail::templ = Persistent<FunctionTemplate>::New(templ);
  
  NAVThumbnail::templ->InstanceTemplate()->SetInternalFieldCount(1); // 1 since this is a constructor function
  NAVThumbnail::templ->SetClassName(String::NewSymbol("NAVThumbnail"));
  
  NODE_SET_PROTOTYPE_METHOD(NAVThumbnail::templ, "write", Write);
  
  target->Set(String::NewSymbol("NAVThumbnail"), NAVThumbnail::templ->GetFunction());    
}

// (options)
Handle<Value> NAVThumbnail::New(const Arguments& args) {
  HandleScope scope;
  
  NAVThumbnail* instance = new NAVThumbnail();
  
  instance->Wrap(args.This());
    
  if(args.Length()<1){
    return ThrowException(Exception::TypeError(String::New("Missing input parameter (srcStream)")));
  }

  Local<Object> options = Local<Object>::Cast(args[0]);
  
  AVCodec *pCodec = avcodec_find_encoder(CODEC_ID_MJPEG);
	if (!pCodec) {
    return ThrowException(Exception::TypeError(String::New("Could not alloc codec")));
	}
  
  instance->pContext = avcodec_alloc_context3(pCodec);
	if (!instance->pContext) {
		return ThrowException(Exception::TypeError(String::New("Could not alloc codec context")));
	}
  
	instance->pContext->width = GET_OPTION_UINT32(options, width, 128);
	instance->pContext->height = GET_OPTION_UINT32(options, height, 128);;
	instance->pContext->pix_fmt = (PixelFormat) GET_OPTION_UINT32(options, pix_fmt, PIX_FMT_YUVJ420P);

  instance->pContext->time_base.num = 1;
  instance->pContext->time_base.den = 1;

  AVDictionary *pDict = NAVDictionary::New(options);
	if (avcodec_open2(instance->pContext, pCodec, &pDict) < 0) {
    av_dict_free(&pDict);
    delete instance;
    return ThrowException(Exception::TypeError(String::New("Could not open codec")));
	}
  av_dict_free(&pDict);

  instance->bufferSize = (instance->pContext->width * instance->pContext->height * MAX_BPP) / 8;
  instance->bufferSize = instance->bufferSize < FF_MIN_BUFFER_SIZE ? 
                         FF_MIN_BUFFER_SIZE:instance->bufferSize;

	instance->pBuffer = (uint8_t *) av_mallocz(instance->bufferSize);
  if (instance->pBuffer == NULL){
    return ThrowException(Exception::Error(String::New("Error allocating buffer")));
  }

  return args.This();
}

// (frame, filename, cb(err))
Handle<Value> NAVThumbnail::Write(const Arguments& args) {
  HandleScope scope;
  Local<Object> frame;
  Local<Function> callback;
  
  if(args.Length()<3){
    return ThrowException(Exception::TypeError(String::New("Missing input parameters (frame, filename, cb)")));
  }
  frame = Local<Array>::Cast(args[0]);
  
  AVFrame *pFrame = (node::ObjectWrap::Unwrap<NAVFrame>(frame))->pContext;

  String::Utf8Value v8str(args[1]);
  char *filename = strdup(*v8str);
  if(filename == NULL){
    return ThrowException(Exception::Error(String::New("Error allocating filename string")));
  }

  if(!(args[2]->IsFunction())){
    return ThrowException(Exception::TypeError(String::New("Third parameter must be a funcion")));
  }
  
  callback = Local<Function>::Cast(args[2]);
  
  NAVThumbnail* instance = UNWRAP_OBJECT(NAVThumbnail, args);

	pFrame->pts = 1;
	pFrame->quality = instance->pContext->global_quality;
	
  AVPacket packet;
  packet.data = instance->pBuffer;
  packet.size = instance->bufferSize;

  av_init_packet(&packet);
  
  int gotPacket;
  if(avcodec_encode_video2(instance->pContext, &packet, pFrame, &gotPacket) < 0){
    return ThrowException(Exception::Error(String::New("Error encoding thumbnail")));
  }
  int encodedSize = packet.size;

	FILE *fileHandle = fopen(filename, "wb");
  if(fileHandle == NULL){
    return ThrowException(Exception::Error(String::New("Error opening thumbnail file")));
  }

	int ret = fwrite(instance->pBuffer, 1, encodedSize, fileHandle);
	fclose(fileHandle);
  
  // TODO: Make asynchronous
  if (ret < encodedSize) {
    Local<Value> err = Exception::Error(String::New("Error writing thumbnail"));
    Local<Value> argv[] = { err };
    callback->Call(Context::GetCurrent()->Global(), 1, argv);
  } else {
    Local<Value> argv[] = { Local<Value>::New(Null()) };
    callback->Call(Context::GetCurrent()->Global(), 1, argv);
  }
  
  return Undefined();
}

