# node-buffertools

Utilities for manipulating buffers.

## Installing the module

Easy!

	node-waf configure build install

Now you can include the module in your project.

	require('buffertools');
	new Buffer(42).clear();

## Methods

The API is still in flux but here are the goodies so far.

Note that most methods that take a buffer as an argument, will also accept a string.

### Buffer.clear()

Clear the buffer. This is equivalent to `Buffer.fill(0)`.
Returns the buffer object so you can chain method calls.

### Buffer.fill(integer|string|buffer)

Fill the buffer (repeatedly if necessary) with the argument.
Returns the buffer object so you can chain method calls.

### Buffer.equals(buffer|string)

Returns true if this buffer equals the argument, false otherwise.

Buffers are considered equal when they are of the same length and contain
the same binary data.

Caveat emptor: If you store strings with different character encodings
in buffers, they will most likely *not* be equal.

### Buffer.compare(buffer|string)

Lexicographically compare two buffers. Returns a number smaller than 1
if a < b, zero if a == b or a number larger than 1 if a > b.

Buffers are considered equal when they are of the same length and contain
the same binary data.

Smaller buffers are considered to be less than larger ones. This has hurt
some buffers' feelings.

### Buffer.indexOf(buffer|string)

Search this buffer for the first occurrence of the argument.
Returns the zero-based index or -1 if there is no match.

## TODO

* Logical operations on buffers (AND, OR, XOR).
* bin2hex and hex2bin functionality.
* Add lastIndexOf() functions.
