buffertools = require('./buffertools.node');
SlowBuffer = require('buffer').SlowBuffer;
Buffer = require('buffer').Buffer;

// extend object prototypes
for (var property in buffertools) {
	exports[property] = Buffer.prototype[property] = SlowBuffer.prototype[property] = buffertools[property];
}
