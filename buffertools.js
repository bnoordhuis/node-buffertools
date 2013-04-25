/* Copyright (c) 2010, Ben Noordhuis <info@bnoordhuis.nl>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

var buffertools = require('./build/Release/buffertools.node');
var SlowBuffer = require('buffer').SlowBuffer;
var Buffer = require('buffer').Buffer;

// requires node 3.1
var events = require('events');
var util = require('util');

// extend object prototypes
for (var key in buffertools) {
	var val = buffertools[key];
	SlowBuffer.prototype[key] = val;
	Buffer.prototype[key] = val;
	exports[key] = val;
}

// bug fix, see https://github.com/bnoordhuis/node-buffertools/issues/#issue/6
Buffer.prototype.concat = SlowBuffer.prototype.concat = function() {
	var args = [this].concat(Array.prototype.slice.call(arguments));
	return buffertools.concat.apply(buffertools, args);
};

//
// WritableBufferStream
//
// - never emits 'error'
// - never emits 'drain'
//
function WritableBufferStream() {
	this.writable = true;
	this.buffer = null;
}

util.inherits(WritableBufferStream, events.EventEmitter);

WritableBufferStream.prototype._append = function(buffer, encoding) {
	if (!this.writable) {
		throw new Error('Stream is not writable.');
	}

	if (Buffer.isBuffer(buffer)) {
		// no action required
	}
	else if (typeof buffer == 'string') {
		// TODO optimize
		buffer = new Buffer(buffer, encoding || 'utf8');
	}
	else {
		throw new Error('Argument should be either a buffer or a string.');
	}

	// FIXME optimize!
	if (this.buffer) {
		this.buffer = buffertools.concat(this.buffer, buffer);
	}
	else {
		this.buffer = new Buffer(buffer.length);
		buffer.copy(this.buffer);
	}
};

WritableBufferStream.prototype.write = function(buffer, encoding) {
	this._append(buffer, encoding);

	// signal that it's safe to immediately write again
	return true;
};

WritableBufferStream.prototype.end = function(buffer, encoding) {
	if (buffer) {
		this._append(buffer, encoding);
	}

	this.emit('close');

	this.writable = false;
};

WritableBufferStream.prototype.getBuffer = function() {
	if (this.buffer) {
		return this.buffer;
	}
	return new Buffer(0);
};

WritableBufferStream.prototype.toString = function() {
	return this.getBuffer().toString();
};

exports.WritableBufferStream = WritableBufferStream;
