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

#include "navdictionary.h"
#include "navutils.h"

using namespace v8;

Persistent<ObjectTemplate> NAVDictionary::templ;

NAVDictionary::NAVDictionary(){
}
  
NAVDictionary::~NAVDictionary(){
}
  
void NAVDictionary::Init(){
  HandleScope scope;
  
  Local<ObjectTemplate> templ = ObjectTemplate::New();
  templ = ObjectTemplate::New();
  templ->SetInternalFieldCount(1);
  
  NAVDictionary::templ = Persistent<ObjectTemplate>::New(templ);
}
  
Handle<Object> NAVDictionary::New(AVDictionary *pDictionary) {
  HandleScope scope;
  
  NAVDictionary *instance = new NAVDictionary();

  Local<Object> obj = NAVDictionary::templ->NewInstance();
  obj->SetInternalField(0, External::New(instance));
  
  AVDictionaryEntry *tag = NULL;
  while ((tag = av_dict_get(pDictionary, "", tag, AV_DICT_IGNORE_SUFFIX))){
    SET_KEY_VALUE(obj, tag->key, String::New(tag->value));
  }
  
  return scope.Close(obj);
}


