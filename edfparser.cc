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

static char json[262144];
static char tmp[262144]; // 128k is enough, right?

void recurse(EDF *tree, int root, int child, int depth)
{
    int loop = true;
    if (root) { tree->Root(); }
    if (child) { tree->Child(); }
    while (loop == true) {
        char *szType = NULL, *szMessage = NULL; //, *szEncoding = NULL;
        long lv; double dv;
        int t = tree->TypeGet(&szType, &szMessage, &lv, &dv);
        sprintf(json, "%s{\"tag\":\"%s\",\"value\":", json, szType);
        if (szType) { delete szType; } 
        switch (t) {
            case EDFElement::INT:
                sprintf(json,"%s%ld", json, lv);
                break;
            case EDFElement::FLOAT:
                sprintf(json,"%s%lf", json, dv);
                break;
            default:
                int i = 0, j = 0, l = 0;

                if (szMessage) { l = strlen(szMessage); }
                // let's brute force this bugger
                for(i=0; i<l; i++) {
                    switch(szMessage[i]) {
                        case '\n': tmp[j] = '\\'; j++; tmp[j] = 'n'; j++; break;
                        case  '"': tmp[j] = '\\'; j++; tmp[j] = '"'; j++; break;
                        default: tmp[j] = szMessage[i]; j++; break;
                    }
                }
                if (szMessage) { delete szMessage; } // guard this with a test
                tmp[j] = '\0';
                sprintf(json,"%s\"%s\"", json, tmp);
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

static Handle<Value> edf_parse(const Arguments& args)
{
  unsigned int offset=0, t_parsed=0;
  HandleScope scope;
  // for now, just return the first parameter as a string
  String::Utf8Value str(args[0]);
  const char* cstr = ToCString(str);
  EDF *pTest = new EDF();

  // set up our array of trees - this is fudge
  sprintf(json, "{\"trees\":[");
  while (offset < strlen(cstr)) { // whilst we have characters left
      strcpy(tmp, cstr+offset);
      int l_parsed = pTest->Read(tmp);
//        fprintf(stderr, "parsed %d\n", l_parsed);
      if (l_parsed <= 0) {
//            fprintf(stderr, "PARSE ERROR RETURNING BORK\n");
          Local<String> result = String::New("-1");
          delete pTest;
          return scope.Close(result);
      }
      recurse(pTest, 1, 0, 0);
      offset = offset + l_parsed;
      strcat(json, ", ");
      t_parsed++;
  }
  sprintf(json, "%s{\"end\":1}], \"parsed\":%d}", json, t_parsed);
  Local<String> result = String::New(json);
  delete pTest;
  return scope.Close(result);
}

extern "C" {
  void init (Handle<Object> target)
  {
    target->Set(String::New("parse"), FunctionTemplate::New(edf_parse)->GetFunction());
  }

}
