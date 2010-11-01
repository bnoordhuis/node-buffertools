#include <v8.h>
#include <node.h>
#include <node_buffer.h>

#include <cstring>
#include <string>

#include "BoyerMoore.h"

using namespace v8;
using namespace node;

namespace {

// this is an application of the Curiously Recurring Template Pattern
template <class Derived> struct UnaryAction {
	Handle<Value> apply(Handle<Object>& buffer, const Arguments& args, HandleScope& scope);

	Handle<Value> operator()(const Arguments& args) {
		HandleScope scope;

		Local<Object> self = args.This();
		if (!Buffer::HasInstance(self)) {
			return ThrowException(Exception::TypeError(String::New(
					"Argument should be a buffer object.")));
		}

		return static_cast<Derived*>(this)->apply(self, args, scope);
	}
};

template <class Derived> struct BinaryAction {
	Handle<Value> apply(Handle<Object>& buffer, const char* data, size_t size, const Arguments& args, HandleScope& scope);

	Handle<Value> operator()(const Arguments& args) {
		HandleScope scope;

		Local<Object> self = args.This();
		if (!Buffer::HasInstance(self)) {
			return ThrowException(Exception::TypeError(String::New(
					"Argument should be a buffer object.")));
		}

		if (args[0]->IsString()) {
			String::AsciiValue s(args[0]->ToString());
			return static_cast<Derived*>(this)->apply(self, *s, s.length(), args, scope);
		}
		if (Buffer::HasInstance(args[0])) {
			Local<Object> other = args[0]->ToObject();
			return static_cast<Derived*>(this)->apply(
					self, Buffer::Data(other), Buffer::Length(other), args, scope);
		}

		static Persistent<String> illegalArgumentException = Persistent<String>::New(String::New(
				"Second argument must be a string or a buffer."));
		return ThrowException(Exception::TypeError(illegalArgumentException));
	}
};

//
// helper functions
//
Handle<Value> clear(Handle<Object>& buffer, int c) {
	size_t length = Buffer::Length(buffer);
	char* data = Buffer::Data(buffer);
	memset(data, c, length);
	return buffer;
}

Handle<Value> fill(Handle<Object>& buffer, void* pattern, size_t size) {
	size_t length = Buffer::Length(buffer);
	char* data = Buffer::Data(buffer);

	if (size >= length) {
		memcpy(data, pattern, length);
	} else {
		const int n_copies = length / size;
		const int remainder = length % size;
		for (int i = 0; i < n_copies; i++) {
			memcpy(data + size * i, pattern, size);
		}
		memcpy(data + size * n_copies, pattern, remainder);
	}

	return buffer;
}

int compare(Handle<Object>& buffer, const char* data2, size_t length2) {
	size_t length = Buffer::Length(buffer);
	if (length != length2) {
		return length > length2 ? 1 : -1;
	}

	char* data = Buffer::Data(buffer);
	return memcmp(data, data2, length);
}

//
// actions
//
struct ClearAction: UnaryAction<ClearAction> {
	Handle<Value> apply(Handle<Object>& buffer, const Arguments& args, HandleScope& scope) {
		return clear(buffer, 0);
	}
};

struct FillAction: UnaryAction<FillAction> {
	Handle<Value> apply(Handle<Object>& buffer, const Arguments& args, HandleScope& scope) {
		if (args[0]->IsInt32()) {
			int c = args[0]->ToInt32()->Int32Value();
			return clear(buffer, c);
		}

		if (args[0]->IsString()) {
			String::AsciiValue s(args[0]->ToString());
			return fill(buffer, *s, s.length());
		}

		if (Buffer::HasInstance(args[0])) {
			Handle<Object> other = args[0]->ToObject();
			size_t length = Buffer::Length(other);
			char* data = Buffer::Data(other);
			return fill(buffer, data, length);
		}

		static Persistent<String> illegalArgumentException = Persistent<String>::New(String::New(
				"Second argument should be either a string, a buffer or an integer."));
		return ThrowException(Exception::TypeError(illegalArgumentException));
	}
};

struct EqualsAction: BinaryAction<EqualsAction> {
	Handle<Value> apply(Handle<Object>& buffer, const char* data, size_t size, const Arguments& args, HandleScope& scope) {
		return compare(buffer, data, size) == 0 ? True() : False();
	}
};

struct CompareAction: BinaryAction<CompareAction> {
	Handle<Value> apply(Handle<Object>& buffer, const char* data, size_t size, const Arguments& args, HandleScope& scope) {
		return scope.Close(Integer::New(compare(buffer, data, size)));
	}
};

struct IndexOfAction: BinaryAction<IndexOfAction> {
	Handle<Value> apply(Handle<Object>& buffer, const char* data2, size_t size2, const Arguments& args, HandleScope& scope) {
		const char* data = Buffer::Data(buffer);
		size_t size = Buffer::Length(buffer);

		const char* p = BoyerMoore(data, size, data2, size2);
		const ptrdiff_t offset = p ? (p - data) : -1;

		return scope.Close(Integer::New(offset));
	}
};

static char toHexTable[] = "0123456789abcdef";

// CHECKME is this cache efficient?
static char fromHexTable[] = {
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,0,1,2,3,4,5,6,7,8,9,-1,-1,-1,-1,-1,-1,-1,
		10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
		10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
		-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
};

inline Handle<Value> decodeHex(const char* data, size_t size, const Arguments& args, HandleScope& scope) {
	if (size & 1) {
		return ThrowException(Exception::Error(String::New(
				"Odd string length, this is not hexadecimal data.")));
	}

	if (size == 0) {
		return String::Empty();
	}

	Handle<Object>& buffer = Buffer::New(size / 2)->handle_;
	for (char* s = Buffer::Data(buffer); size > 0; size -= 2) {
		int a = fromHexTable[*data++];
		int b = fromHexTable[*data++];

		if (a == -1 || b == -1) {
			return ThrowException(Exception::Error(String::New(
					"This is not hexadecimal data.")));
		}

		*s++ = b | (a << 4);
	}

	return buffer;
}

struct FromHexAction: UnaryAction<FromHexAction> {
	Handle<Value> apply(Handle<Object>& buffer, const Arguments& args, HandleScope& scope) {
		const char* data = Buffer::Data(buffer);
		size_t length = Buffer::Length(buffer);
		return decodeHex(data, length, args, scope);
	}
};

struct ToHexAction: UnaryAction<ToHexAction> {
	Handle<Value> apply(Handle<Object>& buffer, const Arguments& args, HandleScope& scope) {
		const size_t size = Buffer::Length(buffer);
		const char* data = Buffer::Data(buffer);

		if (size == 0) {
			return String::Empty();
		}

		std::string s(size * 2, 0);
		for (size_t i = 0; i < size; ++i) {
			const int c = data[i];
			s[i * 2] = toHexTable[c >> 4];
			s[i * 2 + 1] = toHexTable[c & 15];
		}

		return scope.Close(String::New(s.c_str(), s.size()));
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

Handle<Value> FromHex(const Arguments& args) {
	return FromHexAction()(args);
}

Handle<Value> ToHex(const Arguments& args) {
	return ToHexAction()(args);
}

extern "C" void init(Handle<Object> target) {
	HandleScope scope;

	Local<Object> proto = Buffer::New(0)->handle_->GetPrototype()->ToObject();
	proto->Set(String::NewSymbol("fill"), FunctionTemplate::New(Fill)->GetFunction());
	proto->Set(String::NewSymbol("clear"), FunctionTemplate::New(Clear)->GetFunction());
	proto->Set(String::NewSymbol("equals"), FunctionTemplate::New(Equals)->GetFunction());
	proto->Set(String::NewSymbol("compare"), FunctionTemplate::New(Compare)->GetFunction());
	proto->Set(String::NewSymbol("indexOf"), FunctionTemplate::New(IndexOf)->GetFunction());
	proto->Set(String::NewSymbol("fromHex"), FunctionTemplate::New(FromHex)->GetFunction());
	proto->Set(String::NewSymbol("toHex"), FunctionTemplate::New(ToHex)->GetFunction());
}

}
