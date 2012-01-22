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

#include "avstream.h"
#include "avcodeccontext.h"

using namespace v8;

Persistent<ObjectTemplate> _AVStream::templ;

_AVStream::_AVStream(AVStream *pStream){
  this->pContext = pStream;
}
_AVStream::~_AVStream(){
  av_free(this->pContext);
}

void _AVStream::Init(){
  HandleScope scope;
  
  Local<ObjectTemplate> templ = ObjectTemplate::New();
  templ = ObjectTemplate::New();
  templ->SetInternalFieldCount(1);
  
  _AVStream::templ = Persistent<ObjectTemplate>::New(templ);
}

Handle<Value> _AVStream::New(AVStream *pStream){
  HandleScope scope;
  
  _AVStream *instance = new _AVStream(pStream);
  
  Local<Object> obj = _AVStream::templ->NewInstance();
  obj->SetInternalField(0, External::New(instance));
    
  Handle<Value> codec = _AVCodecContext::New(pStream->codec);
  
  obj->Set(String::NewSymbol("codec"), codec);
  
  float duration = pStream->duration*pStream->time_base.num/
                   (float)pStream->time_base.den;
  
  duration = duration < 0? -1 : duration;
  
  obj->Set(String::NewSymbol("duration"), Number::New(duration));
  
  return scope.Close(obj);
}
HandleScope scope;
