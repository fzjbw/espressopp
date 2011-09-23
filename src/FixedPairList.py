from espresso import pmi
import _espresso 
import espresso
from espresso.esutil import cxxinit

class FixedPairListLocal(_espresso.FixedPairList):
    'The (local) fixed pair list.'

    def __init__(self, storage):
        'Local construction of a fixed pair list'
        if pmi.workerIsActive():
            cxxinit(self, _espresso.FixedPairList, storage)

    def add(self, pid1, pid2):
        'add pair to fixed pair list'
        if pmi.workerIsActive():
            return self.cxxclass.add(self, pid1, pid2)

    def size(self):
        'count number of bonds in GlobalPairList, involves global reduction'
        if pmi.workerIsActive():
            return self.cxxclass.size(self)

    def addBonds(self, bondlist):
        """
        Each processor takes the broadcasted bondlist and
        adds those pairs whose first particle is owned by
        this processor.
        """
        
        if pmi.workerIsActive():
            for bond in bondlist:
                pid1, pid2 = bond
                self.cxxclass.add(self, pid1, pid2)

    def getBonds(self):
        'return the bonds of the GlobalPairList'
        if pmi.workerIsActive():
          bonds=self.cxxclass.getBonds(self)
          return bonds 

if pmi.isController:
    class FixedPairList(object):
        __metaclass__ = pmi.Proxy
        pmiproxydefs = dict(
            cls = 'espresso.FixedPairListLocal',
            localcall = [ "add" ],
            pmicall = [ "addBonds" ],
            pmiinvoke = ['getBonds', 'size']
            )
