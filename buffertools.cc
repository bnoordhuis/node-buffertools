#include <v8.h>
#include <node.h>
#include <node_buffer.h>

#include <cstring>

using namespace v8;
using namespace node;

namespace {

// this is an application of the Curiously Recurring Template Pattern
template <class T> struct UnaryAction {
  Handle<Value> apply(Buffer& buffer, const Arguments& args);

  Handle<Value> operator()(const Arguments& args) {
    if (args[0]->IsObject()) {
      Local<Object> object = args[0]->ToObject();
      if (Buffer::HasInstance(object)) {
        Buffer& buffer = *ObjectWrap::Unwrap<Buffer>(object);
        return static_cast<T*>(this)->apply(buffer, args);
      }
    }
    static Persistent<String> illegalArgumentException = Persistent<String>::New(String::New("First argument should be a buffer."));
    return ThrowException(Exception::TypeError(illegalArgumentException));
  }
};

template <class T> struct BinaryAction {
  Handle<Value> apply(Buffer& a, Buffer& b, const Arguments& args);

  Handle<Value> operator()(const Arguments& args) {
    if (args[0]->IsObject() && args[1]->IsObject()) {
      Local<Object> arg0 = args[0]->ToObject();
      Local<Object> arg1 = args[0]->ToObject();
      if (Buffer::HasInstance(arg0) && Buffer::HasInstance(arg1)) {
        Buffer& a = *ObjectWrap::Unwrap<Buffer>(arg0);
        Buffer& b = *ObjectWrap::Unwrap<Buffer>(arg1);
        return static_cast<T*>(this)->apply(a, b, args);
      }
    }
    static Persistent<String> illegalArgumentException = Persistent<String>::New(String::New("First and second argument should be a buffer."));
    return ThrowException(Exception::TypeError(illegalArgumentException));
  }
};

//
// helper functions
//
Handle<Value> clear(Buffer& buffer, int c) {
  memset(buffer.data(), c, buffer.length());
  return buffer.handle_;
}

Handle<Value> fill(Buffer& buffer, void* pattern, size_t size) {
  if (size >= buffer.length()) {
    memcpy(buffer.data(), pattern, buffer.length());
  } else {
    const int n_copies = buffer.length() / size;
    const int remainder = buffer.length() % size;
    for (int i = 0; i < n_copies; i++) {
      memcpy(buffer.data() + size * i, pattern, size);
    }
    memcpy(buffer.data() + size * n_copies, pattern, remainder);
  }
  return buffer.handle_;
}

int compare(Buffer& a, Buffer& b) {
  if (a.length() != b.length()) {
    return a.length() > b.length() ? 1 : -1;
  }
  return memcmp(a.data(), b.data(), a.length());
}

//
// actions
//
struct ClearAction: UnaryAction<ClearAction> {
  Handle<Value> apply(Buffer& buffer, const Arguments& args) {
    const int c = args[1]->IsInt32() ? args[1]->ToInt32()->Int32Value() : 0;
    return clear(buffer, c);
  }
};

struct FillAction: UnaryAction<FillAction> {
  Handle<Value> apply(Buffer& buffer, const Arguments& args) {
    if (args[1]->IsInt32()) {
      int c = args[1]->ToInt32()->Int32Value();
      return clear(buffer, c);
    }
    if (args[1]->IsString()) {
      String::AsciiValue s(args[1]->ToString());
      return fill(buffer, *s, s.length());
    }
    if (args[1]->IsObject()) {
      Local<Object> o = args[1]->ToObject();
      if (Buffer::HasInstance(o)) {
        Buffer& src = *Buffer::Unwrap<Buffer>(o);
        return fill(buffer, src.data(), src.length());
      }
    }
    static Persistent<String> illegalArgumentException = Persistent<String>::New(String::New(
        "Second argument should be either a string, a buffer or an integer."));
    return ThrowException(Exception::TypeError(illegalArgumentException));
  }
};

struct EqualsAction: BinaryAction<EqualsAction> {
  Handle<Value> apply(Buffer& a, Buffer& b, const Arguments& args) {
    HandleScope scope;
    return scope.Close(Boolean::New(compare(a, b)));
  }
};

struct CompareAction: BinaryAction<CompareAction> {
  Handle<Value> apply(Buffer& a, Buffer& b, const Arguments& args) {
    HandleScope scope;
    return scope.Close(Integer::New(compare(a, b)));
  }
};

//
// V8 function callbacks
//
Handle<Value> Clear(const Arguments& args) {
  return ClearAction()(args);
}

Handle<Value> Fill(const Arguments& args) {
  return FillAction()(args);
}

Handle<Value> Equals(const Arguments& args) {
  return CompareAction()(args);
}

Handle<Value> Compare(const Arguments& args) {
  return CompareAction()(args);
}

extern "C" void init(Handle<Object> target) {
  HandleScope scope;
  target->Set(String::New("fill"), FunctionTemplate::New(Fill)->GetFunction());
  target->Set(String::New("clear"), FunctionTemplate::New(Clear)->GetFunction());
  target->Set(String::New("equals"), FunctionTemplate::New(Equals)->GetFunction());
  target->Set(String::New("compare"), FunctionTemplate::New(Compare)->GetFunction());
}

}
