#undef NDEBUG
#include <assert.h>
#define NDEBUG

#include "navoutformat.h"
#include "avframe.h"

#include "navutils.h"

using namespace v8;

NAVOutputFormat::NAVOutputFormat(){
  pFormatCtx = NULL;
  pOutputFormat = NULL;
  filename = NULL;
  pVideoBuffer = NULL;
  videoBufferSize = 0;
}

NAVOutputFormat::~NAVOutputFormat(){
  // NAVOutputFormat_close_input(&pFormatCtx);
  av_free(pVideoBuffer);
  av_close_input_file(pFormatCtx);
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

//  obj->Has(String::New(#key))?
#define GET_OPTION(obj, key, conv, default_val) \
  obj->Get(String::New(#key))->conv()

#define GET_OPTION_UINT32(obj, key, val) \
  obj->Has(String::New(#key))?obj->Get(String::New(#key))->Uint32Value():val

// (stream_type, [codecId])
Handle<Value> NAVOutputFormat::AddStream(const Arguments& args) {
  HandleScope scope;
  
  Local<Object> options;
  Local<Object> self = args.This();
  
  CodecID codec_id;
  
  NAVOutputFormat* instance = UNWRAP_OBJECT(NAVOutputFormat, args);
  
  if (args.Length()<1){
    return ThrowException(Exception::TypeError(String::New("Not enough parameters")));
  }
  
  if (!args[0]->IsString()){
    return ThrowException(Exception::TypeError(String::New("Input parameter #0 should be a string")));
  }

  String::Utf8Value v8streamType(args[0]);

  codec_id = instance->pOutputFormat->video_codec;
  if (args.Length()>1){
    if(args[1]->IsNumber()){
      codec_id = (CodecID) args[1]->ToInteger()->Value();
    }else if(args[1]->IsObject()) {
      options = Local<Object>::Cast(args[1]);
    }else if((args.Length()>2) && (args[2]->IsObject())){
      options = Local<Object>::Cast(args[2]);
    }
  } 
    
  if(strcmp(*v8streamType, "Video") == 0){
    AVCodecContext *pCodecContext;
    AVStream *pStream;
    
    pStream = avformat_new_stream(instance->pFormatCtx, NULL);
    if (!pStream) {
      return ThrowException(Exception::Error(String::New("Could not create stream")));
    }
    
    pCodecContext = pStream->codec;
    
    pCodecContext->codec_id = codec_id;    
    pCodecContext->codec_type = AVMEDIA_TYPE_VIDEO;
    
    pCodecContext->bit_rate = GET_OPTION_UINT32(options, bit_rate, 400000);
      
    // TODO: Force dims to multiple of 2! (or maybe even 16)
    pCodecContext->width = GET_OPTION_UINT32(options, width, 480);
    pCodecContext->height = GET_OPTION_UINT32(options, height, 270);
    
    // time base: this is the fundamental unit of time (in seconds) in terms
    // of which frame timestamps are represented. for fixed-fps content,
    // timebase should be 1/framerate and timestamp increments should be
    // identically 1.
    
    pCodecContext->time_base.den = GET_OPTION_UINT32(options, framerate, 25);
    pCodecContext->time_base.num = 1;
    
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
    
    // some formats want stream headers to be separate
    if(instance->pFormatCtx->oformat->flags & AVFMT_GLOBALHEADER){
      pCodecContext->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }
    
    Local<Array> streams = Local<Array>::Cast(self->Get(String::New("streams")));
    Handle<Value> stream = _AVStream::New(pStream);
    
    streams->Set(0,stream);
    
    return stream;
  }

  return Undefined();
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
    
    // Do Video or Audio specific initializations...
    if(pCodecContext->codec_type == AVMEDIA_TYPE_VIDEO){
      hasVideo = true;
    }
    
    if(pCodecContext->codec_type == AVMEDIA_TYPE_AUDIO){
      hasAudio = true;
    }
  }
  
  if(hasVideo){
    if (!(instance->pFormatCtx->oformat->flags & AVFMT_RAWPICTURE)) {
      if(instance->pVideoBuffer){
        av_free(instance->pVideoBuffer);
      }
      instance->videoBufferSize = 2000000;
      instance->pVideoBuffer = (uint8_t*) av_malloc(instance->videoBufferSize);
    }
    
    // allocate the encoded raw picture
    /*
    picture = alloc_picture(c->pix_fmt, c->width, c->height);
    if (!picture) {
      fprintf(stderr, "Could not allocate picture\n");
      exit(1);
    }
    
    //if the output format is not YUV420P, then a temporary YUV420P
    // picture is needed too. It is then converted to the required
    // output format 
    tmp_picture = NULL;
    if (c->pix_fmt != PIX_FMT_YUV420P) {
      tmp_picture = alloc_picture(PIX_FMT_YUV420P, c->width, c->height);
      if (!tmp_picture) {
        fprintf(stderr, "Could not allocate temporary picture\n");
        exit(1);
      }
    }
    */
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

static AVFrame *alloc_picture(enum PixelFormat pix_fmt, int width, int height)
{
  AVFrame *picture;
  uint8_t *picture_buf;
  int size;
  
  picture = avcodec_alloc_frame();
  if (!picture)
    return NULL;
  size = avpicture_get_size(pix_fmt, width, height);
  picture_buf = (uint8_t*) av_malloc(size);
  if (!picture_buf) {
    av_free(picture);
    return NULL;
  }
  avpicture_fill((AVPicture *)picture, picture_buf,
                 pix_fmt, width, height);
  return picture;
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
    
      if (pCodecContext->coded_frame->pts != (int)AV_NOPTS_VALUE){
        packet.pts= av_rescale_q(pCodecContext->coded_frame->pts, 
                                 pCodecContext->time_base, 
                                 pCodecContext->time_base);
      }
      
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


