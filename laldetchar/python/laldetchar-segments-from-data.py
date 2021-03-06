#
# Copyright (C) 2013 Chris Pankow
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2 of the License, or (at your
# option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
# Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
"""Generate segments from thresholding data"""


import sys
import re
from optparse import OptionParser
from collections import defaultdict

from laldetchar import dqsegs

from laldetchar import git_version

__author__ = "Chris Pankow <chris.pankow@ligo.org>"
__version__ = git_version.id
__date__    = git_version.date

try:
	from pylal import Fr
except ImportError:
	sys.exit("Need pylal installed for frame reading support.")

try:
	from glue.segments import segment, segmentlist, segmentlistdict
	from glue.lal import Cache, LIGOTimeGPS

	from glue.ligolw.utils import segments as lw_segs
	from glue.ligolw.utils import process
	from glue.ligolw import utils, ligolw
except:
	sys.exit("Need glue installed for XML and segment support.")

ALLOWABLE_OPS = {
	"<": float.__lt__,
	">": float.__gt__,
	"=": float.__eq__,
	"&": int.__and__
}
OPS_REG = re.compile("(.*)[%s](.*)" % "".join(ALLOWABLE_OPS.keys()))

def stringify(op):
	if op == ">":
		return "GT"
	elif op == "<":
		return "LT"
	elif op == "=":
		return "EQ"
	elif op == "&":
		return "AND"

def parse_specification(specstr):
	"""
	Parse out the instrument, channel, logic, and RHS value of each string passed through the command line options.
	"""
	match = re.match(OPS_REG, specstr)
	if match is None:
		raise ValueError("%s is an invalid operational specification string." % specstr)
	chan, value = match.group(1), match.group(2)
	op = specstr[match.end(1):match.start(2)]
	assert op in ALLOWABLE_OPS.keys()
	inst, chan = chan.split(":")
	return inst, chan, op, value

optp = OptionParser()
optp.add_option("-s", "--specifier", action="append", help="Channel and action to take. Currently allowable actions are <, >, &, and =. It is *highly* recommended you place this in a string so that it is not expanded by the shell. Example: -s \"H1:IMC-MC2_TRANS_SUM_OUT16>900\"")
optp.add_option("-c", "--frame-cache", action="store", help="LIGO formatted frame cache to read data from.")
optp.add_option("-t", "--output-tag", action="store", default="SEGMENTS", help="Tag for file output. Default is SEGMENTS")
# FIXME: Do this
optp.add_option("-i", "--intersection", action="store_true", help="Take the intersection of the conditions to define the output segments")
optp.add_option("-u", "--union", action="store_true", help="Take the union of the conditions to define the output segments")
optp.add_option("-n", "--name-result", action="store", help="Name the result of --intersection or --union to this.")
optp.add_option("-v", "--verbose", action="store_true", help="Be verbose.")
opts, arg = optp.parse_args()

#
# Map the command line specifications into a set of conditions
#
conditions = map(parse_specification, opts.specifier)

#
# Put the conditions together
#
channel_cond = defaultdict(list)
for inst, channel_name, op, threshold in conditions:
	channel = "%s:%s" % (inst, channel_name)
	channel_cond[channel].append((op, threshold))

#
# Read the datas and such
#
ifos = list(set([c[:2] for c in channel_cond.keys()]))
cache = Cache.fromfile(open(opts.frame_cache))
seg = cache.to_segmentlistdict()[ifos[0][0]][0]
if opts.verbose:
	print "Loaded %s, total coverage time: %f" % (opts.frame_cache, abs(seg))

#
# Set up the XML document
#
xmldoc = ligolw.Document()
xmldoc.appendChild(ligolw.LIGO_LW())
# Append the process information
procrow = utils.process.append_process(xmldoc, program=sys.argv[0])
utils.process.append_process_params(xmldoc, procrow, process.process_params_from_dict(opts.__dict__))

#
# Segment storage
#
lwsegs = lw_segs.LigolwSegments(xmldoc)
all_segs = {}

#
# Loop over each channel, applying the set of conditions appropriate to it
#
for channel, opthresholds in channel_cond.iteritems():
	#
	# Can't have both an equality as well as an inequality via threshold
	#
	if len(opthresholds) > 1 and any([op == "==" for (op, t) in opthresholds]):
		raise ValueError("Equality and inequality are not compatible.")

	#
	# Determine lt and gt thresholds
	#
	min_threshold, max_threshold, equals, bit_mask = None, None, None, None
	for op, value in opthresholds:
		if opts.verbose:
			print channel, op, value
		if op == "<":
			max_threshold = float(value)
		elif op == ">":
			min_threshold = float(value)
		elif op == "=":
			equals = float(value)
		elif op == "&":
			try:
				bit_mask = int(value)
			except ValueError:
				bit_mask = int(value, 16) # try base 16 in case it's hex

	#
	# Are we looking outside the range rather than inside?
	#
	if min_threshold is not None and max_threshold is not None:
		invert = min_threshold >= max_threshold
	else:
		invert = False
	if opts.verbose:
		print "Inverted? %s"% str(invert)

	seglist = segmentlist([])
	for path in cache.pfnlist():
		#
		# Read data
		#
		data, start, _, dt, _, _ = Fr.frgetvect1d(path, channel)

		#
		# Apply conditions and transform samples to segments
		#
		if equals is not None:
			seglist.extend(dqsegs.equality_data_to_seglist(data, start, dt, equality=equals))
		if bit_mask is not None:
			seglist.extend(dqsegs.mask_data_to_seglist(data, start, dt, mask_on=bit_mask))
		else:
			seglist.extend(dqsegs.threshold_data_to_seglist(data, start, dt, min_threshold=min_threshold, max_threshold=max_threshold, invert=invert))

	seglist.coalesce()
	all_segs["%s: %s" % (channel, str(opthresholds))] = seglist

	if opts.verbose:
		print "Summary of segments for %s %s %s %s" % (inst, channel_name, op, str(threshold))
		int_time = reduce(float.__add__, [float(abs(seg & s)) for s in seglist], 0.0)
		print "Total time covered from cache: %f" % int_time

	for i, seg in enumerate(seglist):
		#
		# Segments are expected in LIGOTimeGPS format
		#
		seglist[i] = segment(LIGOTimeGPS(seg[0]), LIGOTimeGPS(seg[1]))
		if opts.verbose:
			print "\t" + str(seg)

	#
	# Fill in some metadata about the flags
	#
	name = "detchar %s threshold flags" % channel
	comment = " ".join(["%s %s %s" % (channel, stringify(op), str(v)) for op, v in opthresholds])
	lwsegs.insert_from_segmentlistdict(segmentlistdict({channel[:2]: seglist}), name=name, comment=comment)

#
# After recording segments, one can take the intersection (all must be on) or
# union (any can be on)
#
# Possible enhancement: instead of giving all keys, give a user selection
# corresponding to their demands
if opts.intersection:
	intersection = segmentlistdict(all_segs).intersection(all_segs.keys())
	# FIXME: ifos
	lwsegs.insert_from_segmentlistdict(segmentlistdict({channel[:2]: seglist}), name=opts.name_result or "INTERSECTION", comment="%s intersection" % " ".join(all_segs.keys()))
elif opts.union:
	union = segmentlistdict(all_segs).union(all_segs.keys())
	# FIXME: ifos
	lwsegs.insert_from_segmentlistdict(segmentlistdict({channel[:2]: seglist}), name=opts.name_result or "UNION", comment="%s union" % " ".join(all_segs.keys()))

#
# Finish up
#
lwsegs.finalize(procrow)

# FIXME: Determine the true extent of the cache
seg = cache.to_segmentlistdict()[ifos[0][0]][0]
utils.write_filename(xmldoc, "%s-%s-%d-%d.xml.gz" % ("".join(ifos), opts.output_tag, seg[0], abs(seg)), gz = True, verbose=opts.verbose)
