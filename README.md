# node-buffertools

Utilities for manipulating buffers.

## Installing the module

Easy.

	node-waf configure build install

Now you can include the module in your project.

	var buffertools = require('buffertools');
	buffertools.clear(buf);

## Methods

The API is still in flux but here are the goodies so far.

Note that most methods that take a buffer as a second argument, will also accept a string.

### buffertools.clear(buffer)

Clear the buffer. This is equivalent to `buffertools.fill(buffer, 0)`.
Returns the buffer object so you can chain calls.

### buffertools.fill(buffer, integer|string|buffer)

Fill the buffer (repeatedly if necessary) with the second argument.
Returns the buffer object so you can chain calls.

### buffertools.equals(buffer, buffer|string)

Returns true if the first buffer equals the second one, false otherwise.

Buffers are considered equal when they are of the same length and contain
the same binary data.

Caveat emptor: If you store strings with different character encodings
in the buffers, they will most likely *not* be equal.

### buffertools.compare(buffer, buffer|string)

Lexicographically compare two buffers. Returns a number smaller than 1
if a < b, zero if a == b or a number larger than 1 if a > b.

Buffers are considered equal when they are of the same length and contain
the same binary data.

Smaller buffers are considered to be less than larger ones. This has hurt
some buffers' feelings.

### buffertools.indexOf(buffer, buffer|string)

Search the first buffer for the first occurrence of the second argument.
Returns the zero-based index or -1 if there is no match.

## TODO

* Extend Buffer.prototype with buffertools methods.
* Logical operations on buffers (AND, OR, XOR).
* bin2hex and hex2bin functionality.
* Add lastIndexOf() functions.
