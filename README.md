# node-buffertools

Utilities for manipulating buffers.

## Installing the module

Easy.

	node-waf configure build install

Now you can include the module in your project.

	var buffertools = require('buffertools');
	buffertools.clear(buf);

## Methods

The API still in flux but here are the goodies so far.

### buffertools.clear(buffer)

Clear the buffer. This is equivalent to `buffertools.fill(buffer, 0)`.
Returns the buffer object so you can chain calls.

### buffertools.fill(buffer, integer|string|buffer)

Fill the buffer (repeatedly if necessary) with the second argument.
Returns the buffer object so you can chain calls.

### buffertools.equals(a, b)

Returns true if the first buffer equals the second one, false otherwise.
Buffers are considered equal when they are of the same length and contain
the same binary data.

Caveat emptor: If you store strings with different character encodings
in the buffers, they will most likely *not* be equal.

### buffertools.compare(a, b)

Lexicographically compare the two buffers. Returns a number smaller than 1
if a < b, zero if a == b or a number larger than 1 if a > b.

## TODO

* Extend Buffer.prototype with buffertools methods.
* Logical operations on buffers (AND, OR, XOR).
* bin2hex and hex2bin functionality.
