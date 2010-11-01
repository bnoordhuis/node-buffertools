var buffertools = require('./buffertools.node');
var SlowBuffer = require('buffer').SlowBuffer;
var Buffer = require('buffer').Buffer;

var slice = Array.prototype.slice;

// extend object prototypes
Object.keys(buffertools).forEach(function(property) {

  // the globally extended 'prototype's
  // TODO: figure out a way to do this on a per-module basis
  Buffer.prototype[property] = SlowBuffer.prototype[property] = buffertools[property];

  // export a 'static' version of the helper function, where the
  // 'this' Buffer is passed as the first argument instead
  exports[property] = function() {
    var buffer = arguments[0];
    var args = slice.call(arguments, 1);
    return buffertools[property].apply(buffer, args);
  }

});
