
#include <v8.h>
#include <node.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

using namespace v8;

class AVFormat : node::ObjectWrap {
private:
  static AVFormatContext *pFormatCtx;

public:
  AVFormat(){}
  ~AVFormat(){}

  static v8::Persistent<FunctionTemplate> persistent_function_template;

  static void Init(Handle<Object> target){
    HandleScope scope;

    // Our constructor
    v8::Local<FunctionTemplate> local_function_template = v8::FunctionTemplate::New(New);

    AVFormat::persistent_function_template = v8::Persistent<FunctionTemplate>::New(local_function_template);
    
    AVFormat::persistent_function_template->InstanceTemplate()->SetInternalFieldCount(1); // 1 since this is a constructor function
    AVFormat::persistent_function_template->SetClassName(v8::String::NewSymbol("AVFormat"));

    // Our getters and setters
    /*
    AVFormat::persistent_function_template->InstanceTemplate()->SetAccessor(String::New("title"), GetTitle, SetTitle);
    AVFormat::persistent_function_template->InstanceTemplate()->SetAccessor(String::New("icon"), GetIcon, SetIcon);

    // Our methods
    NODE_SET_PROTOTYPE_METHOD(AVFormat::persistent_function_template, "send", Send);
    */
    
    NODE_SET_PROTOTYPE_METHOD(AVFormat::persistent_function_template, "open", Open);
    
    // Binding our constructor function to the target variable
    target->Set(String::NewSymbol("avformat"), AVFormat::persistent_function_template->GetFunction());    
  }

  static Handle<Value> New(const Arguments& args) {
    HandleScope scope;
    AVFormat* avformat_instance = new AVFormat();
    
    // avformat_instance->title = "Node.js";
    // avformat_instance->icon = "terminal";

    // Wrap our C++ object as a Javascript object
    avformat_instance->Wrap(args.This());

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
  // this.icon
  static v8::Handle<Value> GetIcon(v8::Local<v8::String> property, const v8::AccessorInfo& info) {
    // Extract the C++ request object from the JavaScript wrapper.
    Gtknotify* gtknotify_instance = node::ObjectWrap::Unwrap<Gtknotify>(info.Holder());
    return v8::String::New(gtknotify_instance->icon.c_str());
  }
  // this.icon=
  static void SetIcon(Local<String> property, Local<Value> value, const AccessorInfo& info) {
    Gtknotify* gtknotify_instance = node::ObjectWrap::Unwrap<Gtknotify>(info.Holder());
    v8::String::Utf8Value v8str(value);
    gtknotify_instance->icon = *v8str;
  }
  */

  // Open file
  static v8::Handle<Value> Open(const Arguments& args) {
    v8::HandleScope scope;
    
    int ret;
		
    // Extract C++ object reference from "this"
		AVFormat* avformat_instance = node::ObjectWrap::Unwrap<AVFormat>(args.This());
		// Convert first argument to V8 String
		v8::String::Utf8Value v8str(args[0]);

		
    ret = avformat_open_input(&pFormatCtx, *v8str, NULL, NULL);
    if(ret<0){
      return Integer::New(ret);
    }
    /*
    ret = av_find_stream_info(pFormatCtx);
    */
		return Integer::New(ret);
	}
  
  // this.send()
  /*
  static v8::Handle<Value> Send(const Arguments& args) {
    v8::HandleScope scope;
    // Extract C++ object reference from "this"
    Gtknotify* gtknotify_instance = node::ObjectWrap::Unwrap<Gtknotify>(args.This());

    // Convert first argument to V8 String
    v8::String::Utf8Value v8str(args[0]);

    // For more info on the Notify library: http://library.gnome.org/devel/libnotify/0.7/NotifyNotification.html 
    Notify::init("Basic");
    // Arguments: title, content, icon
    Notify::Notification n(gtknotify_instance->title.c_str(), *v8str, gtknotify_instance->icon.c_str()); // *v8str points to the C string it wraps
    // Display the notification
    n.show();
    // Return value
    return v8::Boolean::New(true);
  }
  */
};

v8::Persistent<FunctionTemplate> AVFormat::persistent_function_template;

extern "C" { // Cause of name mangling in C++, we use extern C here
  static void init(Handle<Object> target) {
    AVFormat::Init(target);
  }
  // @see http://github.com/ry/node/blob/v0.2.0/src/node.h#L101
  NODE_MODULE(navcodec, init);
}
