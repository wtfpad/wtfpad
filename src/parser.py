from os.path import join, basename, dirname

import constants as ct
from constants import IN, OUT, TOR_CELL_SIZE

import logging

logger = logging.getLogger('wtfpad')


def parse(fpath):
    '''Parse a file assuming Tao's format.'''
    t = Trace()
    for line in open(fpath):
        timestamp, length = line.strip().split(ct.TRACE_SEP)
        direction = int(length) / abs(int(length))
        t.append(Packet(float(timestamp), direction, int(length)))
    return t


def dump(trace, fpath):
    '''Write trace packet into file `fpath`.'''
    with open(fpath, 'w') as fo:
        for packet in trace:
            fo.write(str(packet) + ct.NL)


class Packet(object):
    """Define a packet.

    Direction is defined in the wfpaddef const.py.
    """
    payload = None

    def __init__(self, timestamp, direction, length, dummy=False):
        self.timestamp = timestamp
        self.direction = direction
        self.length = length
        self.dummy = dummy

    def __lt__(self, other):
        return self.timestamp < other.timestamp

    def __str__(self):
        return '\t'.join(map(str, [self.timestamp, self.direction * self.length]))


class Trace(list):
    """Define trace as a list of Packets."""

    _index = 0

    def __init__(self,  list_packets=None):
        if list_packets:
            for p in list_packets:
                self.append(p)

    def __getslice__(self, i, j):
        return Trace(list_packets=list.__getslice__(self, i, j))

    def __add__(self, new_packet):
        t = Trace(self.pcap)
        l = list.__add__(self, new_packet)
        for e in l:
            t.append(e)
        return t

    def __mul__(self, other):
        return Trace(list.__mul__(self, other))

    def get_next_by_direction(self, i, direction):
        for j, p in enumerate(self[i + 1:]):
            if p.direction == direction:
                return i + j + 1
        raise IndexError("No packets in this direction.")

    def __next__(self):
        try:
            i = self[self._index]
        except IndexError:
            raise StopIteration
        self._index += 1
        return i


class Flow(Trace):
    """Provides a structure to keep flow dependent variables."""

    def __init__(self, direction):
        """Initialize direction and state of the flow."""
        self.direction = direction
        self.expired = False
        self.timeout = 0.0
        self.state = ct.BURST
        Trace.__init__(self)

