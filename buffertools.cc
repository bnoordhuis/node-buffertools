#include <v8.h>
#include <node.h>
#include <node_buffer.h>

#include <cstring>

using namespace v8;
using namespace node;

namespace {

Buffer* unwrapBuffer(Local<Value> value) {
	if (Buffer::HasInstance(value)) {
		return Buffer::Unwrap<Buffer>(value->ToObject());
	}
	return 0;
}

// this is an application of the Curiously Recurring Template Pattern
template <class Derived> struct UnaryAction {
	Handle<Value> apply(Buffer& buffer, const Arguments& args, HandleScope& scope);

	Handle<Value> operator()(const Arguments& args) {
		Buffer* buffer = unwrapBuffer(args[0]);
		if (buffer != 0) {
			HandleScope scope;
			return static_cast<Derived*>(this)->apply(*buffer, args, scope);
		}
		static Persistent<String> illegalArgumentException = Persistent<String>::New(String::New("First argument should be a buffer."));
		return ThrowException(Exception::TypeError(illegalArgumentException));
	}
};

template <class Derived> struct BinaryAction {
	Handle<Value> apply(Buffer& buffer, const char* data, size_t size, const Arguments& args, HandleScope& scope);

	Handle<Value> operator()(const Arguments& args) {
		Buffer* a = unwrapBuffer(args[0]);
		if (a == 0) {
			static Persistent<String> illegalArgumentException = Persistent<String>::New(String::New(
					"First argument must be a buffer."));
			return ThrowException(Exception::TypeError(illegalArgumentException));
		}

		HandleScope scope;
		if (args[1]->IsString()) {
			String::AsciiValue s(args[1]->ToString());
			return static_cast<Derived*>(this)->apply(*a, *s, s.length(), args, scope);
		}

		Buffer* b = unwrapBuffer(args[1]);
		if (b != 0) {
			return static_cast<Derived*>(this)->apply(*a, b->data(), b->length(), args, scope);
		}

		static Persistent<String> illegalArgumentException = Persistent<String>::New(String::New(
				"Second argument must be a string or a buffer."));
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

int compare(Buffer& a, const char* data, size_t length) {
	if (a.length() != length) {
		return a.length() > length ? 1 : -1;
	}
	return memcmp(a.data(), data, length);
}

//
// actions
//
struct ClearAction: UnaryAction<ClearAction> {
	Handle<Value> apply(Buffer& buffer, const Arguments& args, HandleScope& scope) {
		const int c = args[1]->IsInt32() ? args[1]->ToInt32()->Int32Value() : 0;
		return clear(buffer, c);
	}
};

struct FillAction: UnaryAction<FillAction> {
	Handle<Value> apply(Buffer& buffer, const Arguments& args, HandleScope& scope) {
		if (args[1]->IsInt32()) {
			int c = args[1]->ToInt32()->Int32Value();
			return clear(buffer, c);
		}

		if (args[1]->IsString()) {
			String::AsciiValue s(args[1]->ToString());
			return fill(buffer, *s, s.length());
		}

		Buffer* other = unwrapBuffer(args[1]);
		if (other != 0) {
			return fill(buffer, other->data(), other->length());
		}

		static Persistent<String> illegalArgumentException = Persistent<String>::New(String::New(
				"Second argument should be either a string, a buffer or an integer."));
		return ThrowException(Exception::TypeError(illegalArgumentException));
	}
};

struct EqualsAction: BinaryAction<EqualsAction> {
	Handle<Value> apply(Buffer& buffer, const char* data, size_t size, const Arguments& args, HandleScope& scope) {
		return compare(buffer, data, size) == 0 ? True() : False();
	}
};

struct CompareAction: BinaryAction<CompareAction> {
	Handle<Value> apply(Buffer& buffer, const char* data, size_t size, const Arguments& args, HandleScope& scope) {
		return scope.Close(Integer::New(compare(buffer, data, size)));
	}
};

struct IndexOfAction: BinaryAction<IndexOfAction> {
	Handle<Value> apply(Buffer& buffer, const char* data, size_t size, const Arguments& args, HandleScope& scope) {
		// FIXME memmem is a GNU extension
		const char* p = (const char*) memmem(buffer.data(), buffer.length(), data, size);
		// FIXME ptrdiff_t may be larger than the range of a JS integer
		const ptrdiff_t offset = p ? p - buffer.data() : -1;
		return scope.Close(Integer::New(offset));
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
	return EqualsAction()(args);
}

Handle<Value> Compare(const Arguments& args) {
	return CompareAction()(args);
}

Handle<Value> IndexOf(const Arguments& args) {
	return IndexOfAction()(args);
}

extern "C" void init(Handle<Object> target) {
	HandleScope scope;
	target->Set(String::New("fill"), FunctionTemplate::New(Fill)->GetFunction());
	target->Set(String::New("clear"), FunctionTemplate::New(Clear)->GetFunction());
	target->Set(String::New("equals"), FunctionTemplate::New(Equals)->GetFunction());
	target->Set(String::New("compare"), FunctionTemplate::New(Compare)->GetFunction());
	target->Set(String::New("indexOf"), FunctionTemplate::New(IndexOf)->GetFunction());
}

}
