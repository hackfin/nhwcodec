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
		# print "Reading %d bytes from offset %0x" % (nbytes, q)
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

def compare_block(data0, data1, off, desc):
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


class NHWDiff:
	def __init__(self, r0, r1, h0, h1):
		self.r0 = r0
		self.r1 = r1
		self.h0 = h0
		self.h1 = h1

	def diff_block(self, varname, desc):
		off = self.r1.get_offset()
		len0, len1 = eval_sizes(self.h0, self.h1, varname)
		# print len0, len1
		data0 = self.r0.read(len0)
		data1 = self.r1.read(len1)

		return compare_block(data0, data1, off, desc)

	def diff_block_fixed(self, size, desc):
		off = self.r1.get_offset()
		len0, len1 = size, size
		data0 = self.r0.read(len0)
		data1 = self.r1.read(len1)

		return compare_block(data0, data1, off, desc)

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


def eval_sizes(h0, h1, name):
	return (eval("h0." + name), eval("h1." + name))


def diff_data(quality, diff):
	ret = 0


	ret += diff.diff_block("size_tree[0]", "TREE1")
	ret += diff.diff_block("size_tree[1]", "TREE2")

	ret += diff.diff_block("exw_Y_end", "EXW_Y_END")

	if quality > LOW8:
		ret += diff.diff_block("res1_len", "RES1")
		# two times?
		ret += diff.diff_block("res1_bit_len", "RES1_BIT")
		ret += diff.diff_block("res1_bit_len", "RES1_WORD")

		if quality > LOW3:
			ret += diff.diff_block("res4_len", "RES4")
			if quality >= LOW1:
				ret += diff.diff_block("res3_len[0]", "RES3")
				ret += diff.diff_block("res3_len[1]", "RES3_BIT")
				ret += diff.diff_block("res3_len[1]", "RES3_WORD")
				if quality > HIGH1:
					if quality == HIGH1:
						ret += diff.diff_block("res5_len[0]", "RES5")
						ret += diff.diff_block("res5_len[1]", "RES5_BIT")
						ret += diff.diff_block("res5_len[1]", "RES5_WORD")

					ret += diff.diff_block("res6_len[0]", "RES6")
					ret += diff.diff_block("res6_len[1]", "RES6_BIT")
					ret += diff.diff_block("res6_len[1]", "RES6_WORD")
					ret += diff.diff_block("char_res1_len", "CHAR_RES1")

					if quality > HIGH2:
						ret += diff.diff_block("qset3_len", "QSET3")

	ret += diff.diff_block("nhw_sel[0]", "NHWSEL1")
	ret += diff.diff_block("nhw_sel[1]", "NHWSEL2")

	if quality > LOW5:
		ret += diff.diff_block_fixed(IM_DIM * 2, "U64")
		ret += diff.diff_block_fixed(IM_DIM * 2, "V64")
		ret += diff.diff_block("highres_comp_len", "HIGHRES_COMP")

	ret += diff.diff_block("end_ch_res", "CH_RES")
	ret += diff.diff_block("size_data[1]", "DATA")
	

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
	rd0 = NHWReader(data0)
	rd1 = NHWReader(data1)

	f0.close()
	f1.close()
	hdr0 = parse_nhwhdr(rd0)
	hdr1 = parse_nhwhdr(rd1)

	matching_headers(hdr0, hdr1)
	if hdr0.QUALITY != hdr1.QUALITY:
		print "Different quality settings, not comparing"
		ret = 1
	else:
		diff = NHWDiff(rd0, rd1, hdr0, hdr1)
		ret = diff_data(hdr0.QUALITY, diff)

	sys.exit(ret)
