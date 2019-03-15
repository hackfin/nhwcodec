def words_to_buf(seq):
	buf = ""
	for s in seq:
		buf += chr(s & 0xff)
		buf += chr((s >> 8) & 0xff)

	return buffer(buf)

def dumpbuf(buf):
	c = 0
	for i in range(len(buf)):
		if c == 16:
			c = 0
			print
		print "%02x" % ord(buf[i]),
		c += 1

	print

def find_mismatch(b0, b1):
	for i in range(len(b0)):
		if b0[i] != b1[i]:
			print "Mismatch from %d" % i,
			print "should be: %02x, is: %02x" % (ord(b0[i]), ord(b1[i]))
			break
	return i


