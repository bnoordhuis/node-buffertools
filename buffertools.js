buffertools = require('./buffertools.node');
SlowBuffer = require('buffer').SlowBuffer;
Buffer = require('buffer').Buffer;

// extend object prototypes
for (var property in buffertools) {
	exports[property] = Buffer.prototype[property] = SlowBuffer.prototype[property] = buffertools[property];
}

// bug fix, see https://github.com/bnoordhuis/node-buffertools/issues/#issue/6
Buffer.prototype.concat = SlowBuffer.prototype.concat = function() {
	var args = [this].concat(Array.prototype.slice.call(arguments));
	return buffertools.concat.apply(buffertools, args);
};
