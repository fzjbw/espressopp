from espresso import pmi
import _espresso 
import espresso
from espresso.esutil import cxxinit

class VerletListAdressLocal(_espresso.VerletListAdress):
    'The (local) verlet list.'

    def __init__(self, system, cutoff, exclusionlist=[]):
        'Local construction of a verlet list'
        if pmi.workerIsActive():
            if (exclusionlist == []):
                # rebuild list in constructor
                cxxinit(self, _espresso.VerletListAdress, system, cutoff, True)
            else:
                # do not rebuild list in constructor
                cxxinit(self, _espresso.VerletListAdress, system, cutoff, False)
                # add exclusions
                for pair in exclusionlist:
                    pid1, pid2 = pair
                    self.cxxclass.exclude(self, pid1, pid2)
                # now rebuild list with exclusions
                self.cxxclass.rebuild(self)
                
            
    def totalSize(self):
        'count number of pairs in VerletList, involves global reduction'
        if pmi.workerIsActive():
            return self.cxxclass.totalSize(self)
        
    def exclude(self, exclusionlist):
        """
        Each processor takes the broadcasted exclusion list
        and adds it to its list.
        """
        if pmi.workerIsActive():
            for pair in exclusionlist:
                pid1, pid2 = pair
                self.cxxclass.exclude(self, pid1, pid2)
            # rebuild list with exclusions
            self.cxxclass.rebuild(self)


    def addAtParticle(self, pids):
        """
        Each processor takes the broadcasted atomistic particles
        and adds it to its list.
        """
        if pmi.workerIsActive():
            for pid in pids:
                self.cxxclass.addAtParticle(self, pid)


if pmi.isController:
    class VerletListAdress(object):
        __metaclass__ = pmi.Proxy
        pmiproxydefs = dict(
            cls = 'espresso.VerletListAdressLocal',
            pmiproperty = [ 'builds' ],
            pmicall = [ 'totalSize', 'exclude', 'addAtParticle' ]
            )