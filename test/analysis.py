import struct

import sys

from nhwdefs import *
from dumpaux import *

advance_sizes = { 'B' : 1, 'H' : 2, 'HH' : 4, 'L' : 4, 'LL' : 8,
	'LH' : 6 }

class NHWReader:
	def __init__(self, data):
		self.data = data
		self.p = 0

	def parse(self, which):
		size = advance_sizes[which]
		q = self.p
		self.p += size

		return struct.unpack("<" + which, self.data[q:self.p])

	def read(self, nbytes):
		q = self.p
		self.p += nbytes
		return self.data[q:self.p]

	def get_offset(self):
		return self.p
	
class NHWHeader:
	def __init__(self):
		self.reshigh = 0
		self.QUALITY = 0

	def dump(self):
		variables = vars(self)
		attrs = variables.keys()
		attrs.sort()
		for a in attrs:
			v = variables[a]
			if type(v) == type(""):
				print "%s: buffer len %d" % (a, len(v))
			else:
				if a == "QUALITY":
					print "%s: %s" % (a, QUALITY_LEVELS[v])
				else:
					print "%s: %s" % (a, v)

def parse_nhwhdr(r):
	hdr = NHWHeader()

	hdr.reshigh = r.parse("B")[0]
	hdr.QUALITY = r.parse("B")[0]
	hdr.size_tree = r.parse("HH")
	# size_data[0] seems unused
	hdr.size_data = r.parse("LL")
	hdr.tree_end = r.parse("H")[0]
	hdr.exw_Y_end = r.parse("H")[0]

	if hdr.QUALITY > LOW8:
		hdr.res1_len = r.parse("H")[0]
		if hdr.QUALITY >= LOW1:
			hdr.res3_len = r.parse("HH")
		if hdr.QUALITY > LOW3:
			hdr.res4_len = r.parse("H")[0]
		hdr.res1_bit_len = r.parse("H")[0]
		if hdr.QUALITY >= HIGH1:
			hdr.res5_len = r.parse("HH")
			if hdr.QUALITY > HIGH1:
				hdr.res6_len = r.parse("LH")
				hdr.char_res1_len = r.parse("H")[0]
				if hdr.QUALITY > HIGH2:
					hdr.qset3_len = r.parse("H")[0]

	hdr.nhw_sel = r.parse("HH")
		
	if hdr.QUALITY > LOW5:
		hdr.highres_comp_len = r.parse("H")[0]

	hdr.end_ch_res = r.parse("H")[0]

	# Now comes the payload
		
	return hdr

def matching_headers(h0, h1):
	variables0 = vars(h0)
	variables1 = vars(h1)


	attr0 = variables0.keys()
	attr1 = variables1.keys()

	attr0.sort()
	attr1.sort()
	
	for e, item in enumerate(attr0):
		v0, v1 = variables0[item], variables1[attr1[e]]
		if v0 != v1:
			print "Mismatch in header for '%s'" % (item)
			print "Wants: %s, is: %s" % (v0, v1)
			return False

	return True


def diff_block(r0, r1, size, desc):
	off = r0.get_offset()
	data0 = r0.read(size)
	data1 = r1.read(size)

	if data0 != data1:
		print
		print "Block '%s' not matching" % desc
		i = find_mismatch(data0, data1)
		off += i
		print "Block offset: %d    File offset: %d / 0x%04x" % (i, off , off)
		dumpbuf(data0[i:i+16])
		dumpbuf(data1[i:i+16])
		return 1
	else:
		return 0

def diff_data(h, r0, r1):
	ret = 0
	ret += diff_block(r0, r1, h.size_tree[0], "TREE1")
	ret += diff_block(r0, r1, h.size_tree[1], "TREE2")

	ret += diff_block(r0, r1, h.exw_Y_end, "EXW_Y_END")

	if h.QUALITY > LOW8:
		ret += diff_block(r0, r1, h.res1_len, "RES1")
		# two times?
		ret += diff_block(r0, r1, h.res1_bit_len, "RES1_BIT")
		ret += diff_block(r0, r1, h.res1_bit_len, "RES1_WORD")

		if h.QUALITY > LOW3:
			ret += diff_block(r0, r1, h.res4_len, "RES4")
			if h.QUALITY >= LOW1:
				ret += diff_block(r0, r1, h.res3_len[0], "RES3")
				ret += diff_block(r0, r1, h.res3_len[1], "RES3_BIT")
				ret += diff_block(r0, r1, h.res3_len[1], "RES3_WORD")
				if h.QUALITY > HIGH1:
					if h.QUALITY == HIGH1:
						ret += diff_block(r0, r1, h.res5_len[0], "RES5")
						ret += diff_block(r0, r1, h.res5_len[1], "RES5_BIT")
						ret += diff_block(r0, r1, h.res5_len[1], "RES5_WORD")

					ret += diff_block(r0, r1, h.res6_len[0], "RES6")
					ret += diff_block(r0, r1, h.res6_len[1], "RES6_BIT")
					ret += diff_block(r0, r1, h.res6_len[1], "RES6_WORD")
					ret += diff_block(r0, r1, h.char_res1_len, "CHAR_RES1")

					if h.QUALITY > HIGH2:
						ret += diff_block(r0, r1, h.qset3_len, "QSET3")

	ret += diff_block(r0, r1, h.nhw_sel[0], "NHWSEL1")
	ret += diff_block(r0, r1, h.nhw_sel[1], "NHWSEL2")

	if h.QUALITY > LOW5:
		ret += diff_block(r0, r1, IM_DIM * 2, "U64")
		ret += diff_block(r0, r1, IM_DIM * 2, "V64")
		ret += diff_block(r0, r1, h.highres_comp_len, "HIGHRES_COMP")

	ret += diff_block(r0, r1, h.end_ch_res, "CH_RES")
	ret += diff_block(r0, r1, h.size_data[1], "DATA")
	

	return ret
	
l = len(sys.argv)

if l == 2:
	f = open(sys.argv[1], "rb")
	data = f.read()
	r = NHWReader(data)
	f.close()
	h = parse_nhwhdr(r)
	h.dump()
elif l > 2:
	f0 = open(sys.argv[1], "rb")
	f1 = open(sys.argv[2], "rb")
	data0 = f0.read()
	data1 = f1.read()
	r0 = NHWReader(data0)
	r1 = NHWReader(data1)

	f0.close()
	f1.close()
	h0 = parse_nhwhdr(r0)
	h1 = parse_nhwhdr(r1)

	if matching_headers(h0, h1):
		ret = diff_data(h0, r0, r1)
	else:
		print "Headers don't match"
		ret = 1

	sys.exit(ret)
