/* This code is PUBLIC DOMAIN, and is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND. See the accompanying 
 * LICENSE file.
 */

#include <v8.h>
#include <node.h>

using namespace node;
using namespace v8;

const char* ToCString(const v8::String::Utf8Value& value) {
      return *value ? *value : "<string conversion failed>";
}

class EDFParser: ObjectWrap
{
private:
  int m_count;
public:

  static Persistent<FunctionTemplate> s_ct;
  static void Init(Handle<Object> target)
  {
    HandleScope scope;

    Local<FunctionTemplate> t = FunctionTemplate::New(New);

    s_ct = Persistent<FunctionTemplate>::New(t);
    s_ct->InstanceTemplate()->SetInternalFieldCount(1);
    s_ct->SetClassName(String::NewSymbol("EDFParser"));

    NODE_SET_PROTOTYPE_METHOD(s_ct, "parse", Parse);

    target->Set(String::NewSymbol("EDFParser"),
                s_ct->GetFunction());
  }

  EDFParser() :
    m_count(0)
  {
  }

  ~EDFParser()
  {
  }

  static Handle<Value> New(const Arguments& args)
  {
    HandleScope scope;
    EDFParser* hw = new EDFParser();
    hw->Wrap(args.This());
    return args.This();
  }

  static Handle<Value> Parse(const Arguments& args)
  {
    HandleScope scope;
    EDFParser* hw = ObjectWrap::Unwrap<EDFParser>(args.This());
    hw->m_count++;
    // for now, just return the first parameter as a string
    String::Utf8Value str(args[0]);
    const char* cstr = ToCString(str);
    Local<String> result = String::New(cstr);
    return scope.Close(result);
  }

};

Persistent<FunctionTemplate> EDFParser::s_ct;

extern "C" {
  static void init (Handle<Object> target)
  {
    EDFParser::Init(target);
  }

  NODE_MODULE(edfparser, init);
}
