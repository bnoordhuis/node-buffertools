#include <v8.h>
#include <node.h>
#include <node_buffer.h>

#include <stdint.h>
#include <sstream>
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
	Handle<Value> apply(Handle<Object>& buffer, const uint8_t* data, size_t size, const Arguments& args, HandleScope& scope);

	Handle<Value> operator()(const Arguments& args) {
		HandleScope scope;

		Local<Object> self = args.This();
		if (!Buffer::HasInstance(self)) {
			return ThrowException(Exception::TypeError(String::New(
					"Argument should be a buffer object.")));
		}

		if (args[0]->IsString()) {
			String::Utf8Value s(args[0]->ToString());
			return static_cast<Derived*>(this)->apply(self, (uint8_t*) *s, s.length(), args, scope);
		}
		if (Buffer::HasInstance(args[0])) {
			Local<Object> other = args[0]->ToObject();
			return static_cast<Derived*>(this)->apply(
					self, (const uint8_t*) Buffer::Data(other), Buffer::Length(other), args, scope);
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
	uint8_t* data = (uint8_t*) Buffer::Data(buffer);
	memset(data, c, length);
	return buffer;
}

Handle<Value> fill(Handle<Object>& buffer, void* pattern, size_t size) {
	size_t length = Buffer::Length(buffer);
	uint8_t* data = (uint8_t*) Buffer::Data(buffer);

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

int compare(Handle<Object>& buffer, const uint8_t* data2, size_t length2) {
	size_t length = Buffer::Length(buffer);
	if (length != length2) {
		return length > length2 ? 1 : -1;
	}

	const uint8_t* data = (const uint8_t*) Buffer::Data(buffer);
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
			String::Utf8Value s(args[0]->ToString());
			return fill(buffer, *s, s.length());
		}

		if (Buffer::HasInstance(args[0])) {
			Handle<Object> other = args[0]->ToObject();
			size_t length = Buffer::Length(other);
			uint8_t* data = (uint8_t*) Buffer::Data(other);
			return fill(buffer, data, length);
		}

		static Persistent<String> illegalArgumentException = Persistent<String>::New(String::New(
				"Second argument should be either a string, a buffer or an integer."));
		return ThrowException(Exception::TypeError(illegalArgumentException));
	}
};

struct ReverseAction: UnaryAction<ReverseAction> {
	// O(n/2) for all cases which is okay, might be optimized some more with whole-word swaps
	// XXX won't this trash the L1 cache something awful?
	Handle<Value> apply(Handle<Object>& buffer, const Arguments& args, HandleScope& scope) {
		uint8_t* head = (uint8_t*) Buffer::Data(buffer);
		uint8_t* tail = head + Buffer::Length(buffer) - 1;

		// xor swap, just because I can
		while (head < tail) *head ^= *tail, *tail ^= *head, *head ^= *tail, ++head, --tail;

		return buffer;
	}
};

struct EqualsAction: BinaryAction<EqualsAction> {
	Handle<Value> apply(Handle<Object>& buffer, const uint8_t* data, size_t size, const Arguments& args, HandleScope& scope) {
		return compare(buffer, data, size) == 0 ? True() : False();
	}
};

struct CompareAction: BinaryAction<CompareAction> {
	Handle<Value> apply(Handle<Object>& buffer, const uint8_t* data, size_t size, const Arguments& args, HandleScope& scope) {
		return scope.Close(Integer::New(compare(buffer, data, size)));
	}
};

struct IndexOfAction: BinaryAction<IndexOfAction> {
	Handle<Value> apply(Handle<Object>& buffer, const uint8_t* data2, size_t size2, const Arguments& args, HandleScope& scope) {
		const uint8_t* data = (const uint8_t*) Buffer::Data(buffer);
		const size_t size = Buffer::Length(buffer);

		int32_t start = args[1]->Int32Value();

		if (start < 0)
			start = size - std::min<size_t>(size, -start);
		else if (static_cast<size_t>(start) > size)
			start = size;

		const uint8_t* p = boyermoore_search(
			data + start, size - start, data2, size2);

		const ptrdiff_t offset = p ? (p - data) : -1;
		return scope.Close(Integer::New(offset));
	}
};

struct LastIndexOfAction: BinaryAction<LastIndexOfAction> {
	Handle<Value> apply(Handle<Object>& buffer, const uint8_t* data2, size_t size2, const Arguments& args, HandleScope& scope) {
		const uint8_t* data = (const uint8_t*) Buffer::Data(buffer);
		const size_t size = Buffer::Length(buffer);

		int32_t start = args[1]->Int32Value();

		if (start < 0)
			start = size - std::min<size_t>(size, -start);
		else if (static_cast<size_t>(start) > size)
			start = size;

		const uint8_t* p;
    const uint8_t* prev;
    while (true) {
      p = boyermoore_search(data + start, size - start, data2, size2);
      if (p) {
        prev = p;
        start = (prev - data) + 1;
      } else
        break;
    }

		const ptrdiff_t offset = prev ? (prev - data) : -1;
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

inline Handle<Value> decodeHex(const uint8_t* const data, const size_t size, const Arguments& args, HandleScope& scope) {
	if (size & 1) {
		return ThrowException(Exception::Error(String::New(
				"Odd string length, this is not hexadecimal data.")));
	}

	if (size == 0) {
		return String::Empty();
	}

	Handle<Object>& buffer = Buffer::New(size / 2)->handle_;

	uint8_t *src = (uint8_t *) data;
	uint8_t *dst = (uint8_t *) (const uint8_t*) Buffer::Data(buffer);

	for (size_t i = 0; i < size; i += 2) {
		int a = fromHexTable[*src++];
		int b = fromHexTable[*src++];

		if (a == -1 || b == -1) {
			return ThrowException(Exception::Error(String::New(
					"This is not hexadecimal data.")));
		}

		*dst++ = b | (a << 4);
	}

	return buffer;
}

struct FromHexAction: UnaryAction<FromHexAction> {
	Handle<Value> apply(Handle<Object>& buffer, const Arguments& args, HandleScope& scope) {
		const uint8_t* data = (const uint8_t*) Buffer::Data(buffer);
		size_t length = Buffer::Length(buffer);
		return decodeHex(data, length, args, scope);
	}
};

struct ToHexAction: UnaryAction<ToHexAction> {
	Handle<Value> apply(Handle<Object>& buffer, const Arguments& args, HandleScope& scope) {
		const size_t size = Buffer::Length(buffer);
		const uint8_t* data = (const uint8_t*) Buffer::Data(buffer);

		if (size == 0) {
			return String::Empty();
		}

		std::string s(size * 2, 0);
		for (size_t i = 0; i < size; ++i) {
			const uint8_t c = (uint8_t) data[i];
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

Handle<Value> Reverse(const Arguments& args) {
	return ReverseAction()(args);
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

Handle<Value> LastIndexOf(const Arguments& args) {
	return LastIndexOfAction()(args);
}

Handle<Value> FromHex(const Arguments& args) {
	return FromHexAction()(args);
}

Handle<Value> ToHex(const Arguments& args) {
	return ToHexAction()(args);
}

Handle<Value> Concat(const Arguments& args) {
	HandleScope scope;

	size_t size = 0;
	for (int index = 0, length = args.Length(); index < length; ++index) {
		Local<Value> arg = args[index];
		if (arg->IsString()) {
			// Utf8Length() because we need the length in bytes, not characters
			size += arg->ToString()->Utf8Length();
		}
		else if (Buffer::HasInstance(arg)) {
			size += Buffer::Length(arg->ToObject());
		}
		else {
			std::stringstream s;
			s << "Argument #" << index << " is neither a string nor a buffer object.";
			return ThrowException(
					Exception::TypeError(
							String::New(s.str().c_str())));
		}
	}

	Buffer& dst = *Buffer::New(size);
	uint8_t* s = (uint8_t*) Buffer::Data(dst.handle_);

	for (int index = 0, length = args.Length(); index < length; ++index) {
		Local<Value> arg = args[index];
		if (arg->IsString()) {
			String::Utf8Value v(arg->ToString());
			memcpy(s, *v, v.length());
			s += v.length();
		}
		else if (Buffer::HasInstance(arg)) {
			Local<Object> b = arg->ToObject();
			const uint8_t* data = (const uint8_t*) Buffer::Data(b);
			size_t length = Buffer::Length(b);
			memcpy(s, data, length);
			s += length;
		}
		else {
			return ThrowException(Exception::Error(String::New(
					"Congratulations! You have run into a bug: argument is neither a string nor a buffer object. "
					"Please make the world a better place and report it.")));
		}
	}

	return scope.Close(dst.handle_);
}

void RegisterModule(Handle<Object> target) {
	target->Set(String::NewSymbol("concat"),  FunctionTemplate::New(Concat)->GetFunction());
	target->Set(String::NewSymbol("fill"),    FunctionTemplate::New(Fill)->GetFunction());
	target->Set(String::NewSymbol("clear"),   FunctionTemplate::New(Clear)->GetFunction());
	target->Set(String::NewSymbol("reverse"), FunctionTemplate::New(Reverse)->GetFunction());
	target->Set(String::NewSymbol("equals"),  FunctionTemplate::New(Equals)->GetFunction());
	target->Set(String::NewSymbol("compare"), FunctionTemplate::New(Compare)->GetFunction());
	target->Set(String::NewSymbol("indexOf"), FunctionTemplate::New(IndexOf)->GetFunction());
  target->Set(String::NewSymbol("lastIndexOf"), FunctionTemplate::New(LastIndexOf)->GetFunction());
	target->Set(String::NewSymbol("fromHex"), FunctionTemplate::New(FromHex)->GetFunction());
	target->Set(String::NewSymbol("toHex"),   FunctionTemplate::New(ToHex)->GetFunction());
}

} // anonymous namespace

NODE_MODULE(buffertools, RegisterModule)
