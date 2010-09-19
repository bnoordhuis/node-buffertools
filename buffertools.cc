#include <v8.h>
#include <node.h>
#include <node_buffer.h>

#include <cstring>

using namespace v8;
using namespace node;

namespace {

// this is an application of the Curiously Recurring Template Pattern
template <class T> struct Action {
  void apply(Buffer& buffer, const Arguments& args);

  Handle<Value> operator()(const Arguments& args) {
    if (args[0]->IsObject()) {
      Local<Object> object = args[0]->ToObject();
      if (Buffer::HasInstance(object)) {
        Buffer& buffer = *ObjectWrap::Unwrap<Buffer>(object);
        static_cast<T*>(this)->apply(buffer, args);
        return buffer.handle_;
      }
    }
    // XXX throw TypeError
    return Undefined();
  }
};

struct ClearAction: Action<ClearAction> {
  void apply(Buffer& buffer, const Arguments& args) {
    const int c = args[1]->IsInt32() ? args[1]->ToInt32()->Int32Value() : 0;
    memset(buffer.data(), c, buffer.length());
  }
};

Handle<Value> Clear(const Arguments& args) {
  return ClearAction()(args);
}

extern "C" void init(Handle<Object> target) {
  HandleScope scope;

  target->Set(String::New("clear"), FunctionTemplate::New(Clear)->GetFunction());
}

}
