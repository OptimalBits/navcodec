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

#include "navstream.h"
#include "navcodeccontext.h"
#include "navutils.h"
#include "navdictionary.h"

using namespace v8;

Persistent<ObjectTemplate> NAVStream::templ;

NAVStream::NAVStream(AVStream *pStream){
  this->pContext = pStream;
}
NAVStream::~NAVStream(){
  printf("NAVStream destructor called");
  av_free(this->pContext);
}

void NAVStream::Init(){
  HandleScope scope;
  
  Local<ObjectTemplate> templ = ObjectTemplate::New();
  templ = ObjectTemplate::New();
  templ->SetInternalFieldCount(1);
  
  NAVStream::templ = Persistent<ObjectTemplate>::New(templ);
}

Handle<Value> NAVStream::New(AVStream *pStream){
  HandleScope scope;
  
  NAVStream *instance = new NAVStream(pStream);
  
  Handle<Object> obj = NAVStream::templ->NewInstance();
  instance->Wrap(obj);
    
  Handle<Value> codec = NAVCodecContext::New(pStream->codec);
  
  SET_KEY_VALUE(obj, "codec", codec);
  
  float duration = pStream->duration*pStream->time_base.num/
                   (float)pStream->time_base.den;
  
  duration = duration < 0? -1 : duration;
  
  SET_KEY_VALUE(obj, "duration", Number::New(duration));
  SET_KEY_VALUE(obj, "metadata", NAVDictionary::New(pStream->metadata));
  
  return obj;
}
HandleScope scope;
