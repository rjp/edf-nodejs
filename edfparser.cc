/* This code is PUBLIC DOMAIN, and is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND. See the accompanying 
 * LICENSE file.
 */

#include <string.h>
#include <v8.h>
#include <node.h>

#include "EDF/EDF.h"

using namespace node;
using namespace v8;

const char* ToCString(const v8::String::Utf8Value& value) {
      return *value ? *value : "<string conversion failed>";
}

char json[1048576];

void recurse(EDF *tree, int root, int child, int depth)
{
    char *szType = NULL, *szMessage = NULL; //, *szEncoding = NULL;
    long lv; double dv;
    int loop = true;
    if (root) { tree->Root(); }
    if (child) { tree->Child(); }
    while (loop == true) {
        int t = tree->TypeGet(&szType, &szMessage, &lv, &dv);
        sprintf(json, "%s{\"tag\":\"%s\",\"value\":", json, szType);
        switch (t) {
            case EDFElement::INT:
                sprintf(json,"%s%ld", json, lv);
                break;
            case EDFElement::FLOAT:
                sprintf(json,"%s%lf", json, dv);
                break;
            default:
                sprintf(json,"%s\"%s\"", json, szMessage);
                break;
        }

        if (tree->Children() > 0) {
            strcat(json, ", \"children\":[");
            recurse(tree, 0, 1, depth+1);
            strcat(json, "]");
        }
        loop = tree->Next();
        if (loop == false) {
            strcat(json, ", \"end\":1}");
            tree->Parent();
        } else {
            strcat(json, "}, ");
        }
    }
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
    int l_parsed = 0;
    unsigned int offset=0, t_parsed=0;
    HandleScope scope;
    EDFParser* hw = ObjectWrap::Unwrap<EDFParser>(args.This());
    hw->m_count++;
    // for now, just return the first parameter as a string
    String::Utf8Value str(args[0]);
    const char* cstr = ToCString(str);
    EDF *pTest = new EDF();

    // set up our array of trees - this is fudge
    sprintf(json, "{\"trees\":[");
    while (offset < strlen(cstr)) { // whilst we have characters left
        char tmp[1048576];
        strcpy(tmp, cstr+offset);
        l_parsed = pTest->Read(tmp);
//        fprintf(stderr, "parsed %d\n", l_parsed);
        if (l_parsed <= 0) {
//            fprintf(stderr, "PARSE ERROR RETURNING BORK\n");
            Local<String> result = String::New("-1");
            return scope.Close(result);
        }
        recurse(pTest, 1, 0, 0);
        offset = offset + l_parsed;
        strcat(json, ", ");
        t_parsed++;
    }
    sprintf(json, "%s{\"end\":1}], \"parsed\":%d}", json, t_parsed);
    Local<String> result = String::New(json);
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
