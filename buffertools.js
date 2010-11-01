buffertools = require('./buffertools.node');
SlowBuffer = require('buffer').SlowBuffer;
Buffer = require('buffer').Buffer;

// extend object prototypes
for (var property in buffertools) {
	SlowBuffer.prototype[property] = SlowBuffer[property];
	Buffer.prototype[property] = buffertools[property];
}
