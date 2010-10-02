#include <v8.h>
#include <node.h>
#include <node_buffer.h>

#include <sstream>
#include <cstring>
#include <string>

using namespace v8;
using namespace node;

namespace {

// this is an application of the Curiously Recurring Template Pattern
template <class Derived> struct UnaryAction {
	Handle<Value> apply(Buffer& buffer, const Arguments& args, HandleScope& scope);

	Handle<Value> operator()(const Arguments& args) {
		HandleScope scope;
		return static_cast<Derived*>(this)->apply(*Buffer::Unwrap<Buffer>(args.This()), args, scope);
	}
};

template <class Derived> struct BinaryAction {
	Handle<Value> apply(Buffer& buffer, const char* data, size_t size, const Arguments& args, HandleScope& scope);

	Handle<Value> operator()(const Arguments& args) {
		HandleScope scope;

		Buffer& buffer = *Buffer::Unwrap<Buffer>(args.This());
		if (args[0]->IsString()) {
			String::AsciiValue s(args[0]->ToString());
			return static_cast<Derived*>(this)->apply(buffer, *s, s.length(), args, scope);
		}
		if (Buffer::HasInstance(args[0])) {
			Buffer& other = *Buffer::Unwrap<Buffer>(args[0]->ToObject());
			return static_cast<Derived*>(this)->apply(buffer, other.data(), other.length(), args, scope);
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
		return clear(buffer, 0);
	}
};

struct FillAction: UnaryAction<FillAction> {
	Handle<Value> apply(Buffer& buffer, const Arguments& args, HandleScope& scope) {
		if (args[0]->IsInt32()) {
			int c = args[0]->ToInt32()->Int32Value();
			return clear(buffer, c);
		}

		if (args[0]->IsString()) {
			String::AsciiValue s(args[0]->ToString());
			return fill(buffer, *s, s.length());
		}

		if (Buffer::HasInstance(args[0])) {
			Buffer& other = *Buffer::Unwrap<Buffer>(args[0]->ToObject());
			return fill(buffer, other.data(), other.length());
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

	Buffer& buffer = *Buffer::New(size / 2);
	for (char* s = buffer.data(); size > 0; size -= 2) {
		int a = fromHexTable[*data++];
		int b = fromHexTable[*data++];

		if (a == -1 || b == -1) {
			return ThrowException(Exception::Error(String::New(
					"This is not hexadecimal data.")));
		}

		*s++ = b | (a << 4);
	}

	return buffer.handle_;
}

struct FromHexAction: UnaryAction<FromHexAction> {
	Handle<Value> apply(Buffer& buffer, const Arguments& args, HandleScope& scope) {
		return decodeHex(buffer.data(), buffer.length(), args, scope);
	}
};

struct ToHexAction: UnaryAction<ToHexAction> {
	Handle<Value> apply(Buffer& buffer, const Arguments& args, HandleScope& scope) {
		const size_t size = buffer.length();

		if (size == 0) {
			return String::Empty();
		}

		std::string s(size * 2, 0);
		for (size_t i = 0; i < size; ++i) {
			const int c = buffer.data()[i];
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
			Buffer& b = *Buffer::Unwrap<Buffer>(arg->ToObject());
			size += b.length();
		}
		else {
			std::stringstream s;
			s << "Argument #" << index << " is neither a string nor a buffer object.";
			const char* message = s.str().c_str();
			return ThrowException(Exception::TypeError(String::New(message)));
		}
	}

	Buffer& dst = *Buffer::New(size);
	char* s = dst.data();

	for (int index = 0, length = args.Length(); index < length; ++index) {
		Local<Value> arg = args[index];
		if (arg->IsString()) {
			String::Utf8Value v(arg->ToString());
			memcpy(s, *v, v.length());
			s += v.length();
		}
		else if (Buffer::HasInstance(arg)) {
			Buffer& b = *Buffer::Unwrap<Buffer>(arg->ToObject());
			memcpy(s, b.data(), b.length());
			s += b.length();
		}
		else {
			return ThrowException(Exception::Error(String::New(
					"Congratulations! You have run into a bug: argument is neither a string nor a buffer object. "
					"Please make the world a better place and report it.")));
		}
	}

	return scope.Close(dst.handle_);
}

extern "C" void init(Handle<Object> target) {
	HandleScope scope;

	target->Set(String::NewSymbol("concat"), FunctionTemplate::New(Concat)->GetFunction());

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
