
#include "avformat.h"

using namespace v8;

AVFormat::AVFormat(){
}
  
AVFormat::~AVFormat(){
  av_close_input_file(pFormatCtx);
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
    NODE_SET_PROTOTYPE_METHOD(AVFormat::templ, "Decode", Decode);
    
    // Global initiallization of libavcodec.
    av_register_all();
    avformat_network_init();
    
    // Binding our constructor function to the target variable
    target->Set(String::NewSymbol("AVFormat"), AVFormat::templ->GetFunction());    
  }
  
Handle<Value> AVFormat::New(const Arguments& args) {
    HandleScope scope;
    
    AVFormat* instance = new AVFormat();
    AVFormatContext *pFormatCtx;
    
    // Wrap our C++ object as a Javascript object
    instance->Wrap(args.This());
    
    instance->pFormatCtx = NULL;
    
    String::Utf8Value v8str(args[0]);
    
    instance->filename = *v8str;
    
    // TODO: Thrown errors!
    int ret = avformat_open_input(&(instance->pFormatCtx),  *v8str, NULL, NULL);
    if(ret>=0){
      pFormatCtx = instance->pFormatCtx;
      
      ret = avformat_find_stream_info(pFormatCtx, NULL);
      if(ret){
        if(pFormatCtx->nb_streams>0){
          Handle<Array> streams = Array::New(pFormatCtx->nb_streams);
          
          for(unsigned int i=0; i < pFormatCtx->nb_streams;i++){
            AVStream *stream = pFormatCtx->streams[i];
            streams->Set(i, _AVStream::New(stream));
          }
          
          //instance->streams = scope.Close(streams); // needed?
          args.This()->Set(String::NewSymbol("streams"), streams);
        }
      }
    }
    
    return args.This();
  }
  
  // this.title
  /*
   static v8::Handle<Value> GetTitle(v8::Local<v8::String> property, const v8::AccessorInfo& info) {
   // Extract the C++ request object from the JavaScript wrapper.
   Gtknotify* gtknotify_instance = node::ObjectWrap::Unwrap<Gtknotify>(info.Holder());
   return v8::String::New(gtknotify_instance->title.c_str());
   }
   // this.title=
   static void SetTitle(Local<String> property, Local<Value> value, const AccessorInfo& info) {
   Gtknotify* gtknotify_instance = node::ObjectWrap::Unwrap<Gtknotify>(info.Holder());
   v8::String::Utf8Value v8str(value);
   gtknotify_instance->title = *v8str;
   }
   */
  
  // ([streams], cb(stream, frame))
Handle<Value> AVFormat::Decode(const Arguments& args) {
    HandleScope scope;
    Handle<Array> streams;
    /*
     struct StreamFrame
     {
     
     int array[100];
     };
     
     
     AVFormat* instance = node::ObjectWrap::Unwrap<AVFormat>(args.This());
     
     // Open all the codecs from the given streams
     if(!args[0].isArray()){
     // Throw error
     }else{
     streams = args[0];
     }
     
     // Create the required frames and associate every frame to a stream.
     for(unsigned int i=0;i<streams.Length(); i++){
     streamFrame.stream = streams->Get(i);
     streamsFrame.frame = AVFrame::New();
     
     }
     
     // Start decoding
     AVPacket packet;
     void *streams;
     while (av_read_frame(instance->pFormatCtx, &packet) {
     AVStream *pStream = streamToDecode(packet, streams);
     if(pStream){
     avcodec_decode_video(pStream->codec, 
     pFrame,
     &finished, 
     packet.data, 
     packet.size);
     if(finished){
     // Call callback cb(pStream, pFrame);
     }
     }
     
     av_free_packet(&packet);
     }
     */
    
    // Close all the codecs from the given streams.
    
		return Integer::New(0);
	}
  
  Handle<Value> AVFormat::Dump(const Arguments& args) {
    HandleScope scope;
    
    // Extract C++ object reference from "this"
		AVFormat* instance = node::ObjectWrap::Unwrap<AVFormat>(args.This());
    
    dump_format(instance->pFormatCtx, 0, instance->filename, 0);
    
		return Integer::New(0);
	}


