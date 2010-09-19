assert = require('assert'), buffertools = require('buffertools'), Buffer = require('buffer').Buffer, SlowBuffer = require('buffer').SlowBuffer;

b = new SlowBuffer(4);
assert.equal(b, buffertools.clear(b, 42));
assert.equal(b.inspect(), '<SlowBuffer 2a 2a 2a 2a>');
