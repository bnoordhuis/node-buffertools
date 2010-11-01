assert = require('assert'), buffertools = require('buffertools'), Buffer = require('buffer').Buffer;

// these trigger the code paths for UnaryAction and BinaryAction
assert.throws(function() { buffertools.clear({}); });
assert.throws(function() { buffertools.equals({}, {}); });

a = new Buffer('abcd'), b = new Buffer('abcd'),  c = new Buffer('efgh');
assert.ok(a.equals(b));
assert.ok(!a.equals(c));
assert.ok(a.equals('abcd'));
assert.ok(!a.equals('efgh'));

assert.ok(a.compare(a) == 0);
assert.ok(a.compare(c) < 0);
assert.ok(c.compare(a) > 0);

assert.ok(a.compare('abcd') == 0);
assert.ok(a.compare('efgh') < 0);
assert.ok(c.compare('abcd') > 0);

b = new Buffer('****');
assert.equal(b, b.clear());
assert.equal(b.inspect(), '<Buffer 00 00 00 00>');	// FIXME brittle test

b = new Buffer(4);
assert.equal(b, b.fill(42));
assert.equal(b.inspect(), '<Buffer 2a 2a 2a 2a>');

b = new Buffer(4);
assert.equal(b, b.fill('*'));
assert.equal(b.inspect(), '<Buffer 2a 2a 2a 2a>');

b = new Buffer(4);
assert.equal(b, b.fill('ab'));
assert.equal(b.inspect(), '<Buffer 61 62 61 62>');

b = new Buffer(4);
assert.equal(b, b.fill('abcd1234'));
assert.equal(b.inspect(), '<Buffer 61 62 63 64>');

b = new Buffer('Hello, world!');
assert.equal(-1, b.indexOf(new Buffer('foo')));
assert.equal(0,  b.indexOf(new Buffer('Hell')));
assert.equal(7,  b.indexOf(new Buffer('world')));
assert.equal(7,  b.indexOf(new Buffer('world!')));
assert.equal(-1, b.indexOf('foo'));
assert.equal(0,  b.indexOf('Hell'));
assert.equal(7,  b.indexOf('world'));
assert.equal(-1, b.indexOf(''));
assert.equal(-1, b.indexOf('x'));
assert.equal(7,  b.indexOf('w'));

b = new Buffer("\t \r\n");
assert.equal('09200d0a', b.toHex());
assert.equal(b.toString(), new Buffer('09200d0a').fromHex().toString());
