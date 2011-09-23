// ESPP_CLASS 
#ifndef _INTERACTION_VERLETLISTADRESSINTERACTIONTEMPLATE_HPP
#define _INTERACTION_VERLETLISTADRESSINTERACTIONTEMPLATE_HPP

//#include <typeinfo>

#include "System.hpp"
#include "bc/BC.hpp"

#include "types.hpp"
#include "Interaction.hpp"
#include "Real3D.hpp"
#include "Tensor.hpp"
#include "Particle.hpp"
#include "VerletListAdress.hpp"
#include "FixedTupleList.hpp"
#include "esutil/Array2D.hpp"

namespace espresso {
  namespace interaction {
    template < typename _Potential >
    class VerletListAdressInteractionTemplate: public Interaction {
    
    protected:
      typedef _Potential Potential;
    
    public:
      VerletListAdressInteractionTemplate
      (shared_ptr<VerletListAdress> _verletList, shared_ptr<FixedTupleList> _fixedtupleList)
                : verletList(_verletList), fixedtupleList(_fixedtupleList) {

          potentialArray = esutil::Array2D<Potential, esutil::enlarge>(0, 0, Potential());

          // AdResS stuff
          pidhy2 = M_PI/(verletList->getHy() * 2);
          dex = verletList->getEx();
          dexdhy = dex + verletList->getHy();
      }

      void
      setVerletList(shared_ptr < VerletListAdress > _verletList) {
        verletList = _verletList;
      }

      shared_ptr<VerletListAdress> getVerletList() {
        return verletList;
      }

      void
      setFixedTupleList(shared_ptr<FixedTupleList> _fixedtupleList) {
          fixedtupleList = _fixedtupleList;
      }

      void
      setPotential(int type1, int type2, const Potential &potential) {
          potentialArray.at(type1, type2) = potential;
          if (type1 != type2) { // add potential in the other direction
             potentialArray.at(type2, type1) = potential;
          }
      }

      Potential &getPotential(int type1, int type2) {
        return potentialArray.at(type1, type2);
      }

      virtual void addForces();
      virtual real computeEnergy();
      virtual real computeVirial();
      virtual void computeVirialTensor(Tensor& w);
      virtual real getMaxCutoff();
      virtual bool isBonded() { return false; }

    protected:
      int ntypes;
      shared_ptr<VerletListAdress> verletList;
      shared_ptr<FixedTupleList> fixedtupleList;
      esutil::Array2D<Potential, esutil::enlarge> potentialArray;

      // AdResS stuff
      real pidhy2; // pi / (dhy * 2)
      real dexdhy; // dex + dhy
      real dex;
      std::map<Particle*, real> weights;

    };

    //////////////////////////////////////////////////
    // INLINE IMPLEMENTATION
    //////////////////////////////////////////////////
    template < typename _Potential > inline void
    VerletListAdressInteractionTemplate < _Potential >::
    addForces() {
      LOG4ESPP_INFO(theLogger, "add forces computed by the Verlet List");
      //std::cout << "add forces computed by the Verlet List" << "\n";

      // Pairs not inside the AdResS Zone (CG region)
      for (PairList::Iterator it(verletList->getPairs()); it.isValid(); ++it) {

        Particle &p1 = *it->first;
        Particle &p2 = *it->second;
        int type1 = p1.type();
        int type2 = p2.type();

        //Real3D dist = p1.position() - p2.position();
        //real distSqr = dist.sqr();
        //std::cout << "CG ids: " << p1.id() << " - " << p2.id() <<  " (" << sqrt(distSqr) << ")" << " \n";
        //std::cout << "CG types: " << type1 << ", " << type2 <<  " \n";

        const Potential &potential = getPotential(type1, type2);

        Real3D force(0.0, 0.0, 0.0);

        //std::cout << "CG forces ...\n";
        if(potential._computeForce(force, p1, p2)) {
          p1.force() += force;
          p2.force() -= force;

          //std::cout << p1.id() << "-" << p2.id() << " CG force: (" << force << ")\n";

          /*
          // loop over CG particles and overwrite AT forces and velocity
          FixedTupleList::iterator it3;
          FixedTupleList::iterator it4;
          it3 = fixedtupleList->find(&p1);
          it4 = fixedtupleList->find(&p2);

          if (it3 != fixedtupleList->end() && it4 != fixedtupleList->end()) {

              Real3D force1(0.0, 0.0, 0.0);
              Real3D force2(0.0, 0.0, 0.0);
              force1 = force * p1.getMass();
              force2 = force * p2.getMass();

              std::vector<Particle*> atList1;
              std::vector<Particle*> atList2;
              atList1 = it3->second;
              atList2 = it4->second;

              for (std::vector<Particle*>::iterator itv = atList1.begin();
                      itv != atList1.end(); ++itv) {
                  Particle &p3 = **itv;
                  p3.velocity() = p1.velocity(); // overwrite velocity
                  p3.force() += force1 / p1.getMass();
              }

              for (std::vector<Particle*>::iterator itv2 = atList2.begin();
                      itv2 != atList2.end(); ++itv2) {
                  Particle &p4 = **itv2;
                  p4.velocity() = p2.velocity(); // overwrite velocity
                  p4.force() -= force2 / p2.getMass();
              }
              //std::cout << "\n";
          }
          else {
              std::cout << " one of the VP particles not found in tuples: " << p1.id() <<
                      "-" << p1.ghost() << ", " << p2.id() << "-" << p2.ghost();
              std::cout << " (" << p1.position() << ") (" << p2.position() << ") ";
          }*/

        }
      }

      // loop over CG particles and overwrite AT forces and velocity
      std::set<Particle*> cgZone = verletList->getCGZone();
      for (std::set<Particle*>::iterator it=cgZone.begin();
                    it != cgZone.end(); ++it) {

            Particle &p1 = **it;

            FixedTupleList::iterator it3;
            it3 = fixedtupleList->find(&p1);

            if (it3 != fixedtupleList->end()) {

                std::vector<Particle*> atList1;
                atList1 = it3->second;

                for (std::vector<Particle*>::iterator itv = atList1.begin();
                        itv != atList1.end(); ++itv) {
                    Particle &p3 = **itv;
                    p3.velocity() = p1.velocity(); // overwrite velocity
                    p3.force() += p1.force();
                }

            }
            else {
                std::cout << " VP particle not found in tuples: " << p1.id() << "-" << p1.ghost();
            }
      }

      // compute center of mass and weights for virtual particles in Adress zone (HY and AT region)
      std::set<Particle*> adrZone = verletList->getAdrZone();
      //std::cout << "adrsize: " << adrZone.size() << "\n";
      //std::cout << "Computing CM ...\n";
      for (std::set<Particle*>::iterator it=adrZone.begin();
              it != adrZone.end(); ++it) {

          Particle &vp = **it;

          FixedTupleList::iterator it3;
          it3 = fixedtupleList->find(&vp);

          //std::cout << "it: " << vp.id() << "\n";
          if (it3 != fixedtupleList->end()) {

              std::vector<Particle*> atList;
              atList = it3->second;

              // compute center of mass
              Real3D cmp(0.0, 0.0, 0.0); // center of mass position
              Real3D cmv(0.0, 0.0, 0.0); // center of mass velocity
              real M = vp.getMass(); // sum of mass of AT particles
              //real M = 0.0;
              //std::cout << "vp id: " << vp.id()  << "-" << vp.ghost() << " pos: " << vp.position() << "\n";
              for (std::vector<Particle*>::iterator it2 = atList.begin();
                                   it2 != atList.end(); ++it2) {

                  Particle &at = **it2;
                  Real3D d1 = at.position() - vp.position();
                  //Real3D d1;
                  //verletList->getSystem()->bc->getMinimumImageVectorBox(d1, at.position(), vp.position());
                  cmp += at.mass() * d1;
                  cmv += at.mass() * at.velocity();

                  //M += at.mass();
                  //std::cout << " at id: " << at.id() << "-" << at.ghost() <<
                    //" pos: " << at.position() << "\n";
                    /*" d1: " << d1 <<
                    " adding to cmp " << at.mass() * d1 <<
                    " current cmp " << cmp << "\n";*/
              }
              cmp = cmp / M;
              cmv = cmv / M;
              cmp += vp.position(); // cmp is a relative position
              //std::cout << " cmp M: "  << M << "\n\n";
              //std::cout << "  moving VP to " << cmp << ", velocitiy is " << cmv << "\n";

              // update (overwrite) the position and velocity of the VP
              vp.position() = cmp;
              vp.velocity() = cmv;


              // compute weights of VP
              //std::cout << "minimim distances...\n";
              std::vector<Real3D*>::iterator it2 = verletList->getAdrPositions().begin();
              Real3D pa = **it2; // position of adress particle
              Real3D d1 = vp.position() - pa;
              real distsq1 = d1.sqr();
              //real min1 = sqrt(distsq1);
              real min1 = distsq1;
              //std::cout << sqrt(distsq1) << "\n";

              // calculate distance to nearest adress particle
              for (; it2 != verletList->getAdrPositions().end(); ++it2) {
                   pa = **it2;
                   d1 = vp.position() - pa;
                   distsq1 = d1.sqr();
                   //std::cout << pa << " " << sqrt(distsq1) << "\n";
                   if (distsq1 < min1) min1 = distsq1;
              }

              min1 = sqrt(min1);
              //std::cout << vp.id() << " min: " << min1 << "\n";
              //std::cout << vp.id() << " dex: " << dex << "\n";
              //std::cout << vp.id() << " dex+dhy: " << dexdhy << "\n";

              // calculate weight and write it in the map
              real w;
              if (dex > min1) w = 1;
              else if (dexdhy < min1) w = 0;
              else {
                   w = cos(pidhy2 * (min1 - dex));
                   w *= w;
              }

              weights.insert(std::make_pair(&vp, w));

              //if (w1 == 1 || w2 == 1) std::cout << p1.id() << " ";
              //std::cout << vp.id() << " weight: " << w << "\n";
          }
          else {
              std::cout << " VP particle " << vp.id() << "-" << vp.ghost() << " not found in tuples ";
              std::cout << " (" << vp.position() << ")\n";
          }
      }



      // Compute forces (AT and VP) of Pairs inside AdResS zone
      //std::cout << "Computing forces of adress pairs ...\n";
      for (PairList::Iterator it(verletList->getAdrPairs()); it.isValid(); ++it) {

         // these are the two VP interacting
         Particle &p1 = *it->first;
         Particle &p2 = *it->second;

         // read weights
         real w1 = weights.find(&p1)->second;
         real w2 = weights.find(&p2)->second;
         real w12 = w1 * w2;

         //TODO check distance and if necessary skip force calculation
         // force between VP particles
         int type1 = p1.type();
         int type2 = p2.type();
         //std::cout << " ---- get potential ---- \n";
         const Potential &potential = getPotential(type1, type2);
         Real3D forcevp(0.0, 0.0, 0.0);
         if (w12 != 1) { // calculate if in hybrid region (and not in AT region)
             //std::cout << "VP forces, ("<< p1.id() << ", " << p2.id() << ") w1: " << w1 << " w2: " << w2 << "\n";
             if(potential._computeForce(forcevp, p1, p2)) {
                 forcevp = (1 - w12) * forcevp;
                 p1.force() += forcevp;
                 p2.force() -= forcevp;

                 //std::cout << p1.id() << "-" << p2.id() << " VP force: (" << forcevp << ")\n";

             }
         }
         /*
         else {
             std::cout << "skipping VP forces...\n";
         }*/

         if (w12 != 0) { // calculate if not in CG region
             // iterate through atomistic particles in fixedtuplelist
             FixedTupleList::iterator it3;
             FixedTupleList::iterator it4;
             it3 = fixedtupleList->find(&p1);
             it4 = fixedtupleList->find(&p2);


             //std::cout << "Interaction " << p1.id() << " - " << p2.id() << "\n";
             if (it3 != fixedtupleList->end() && it4 != fixedtupleList->end()) {

                 std::vector<Particle*> atList1;
                 std::vector<Particle*> atList2;
                 atList1 = it3->second;
                 atList2 = it4->second;

                 //std::cout << "AT forces ...\n";
                 for (std::vector<Particle*>::iterator itv = atList1.begin();
                         itv != atList1.end(); ++itv) {

                     Particle &p3 = **itv;

                     for (std::vector<Particle*>::iterator itv2 = atList2.begin();
                                          itv2 != atList2.end(); ++itv2) {

                         Particle &p4 = **itv2;

                         // TODO check distance and if necessary skip force calculation
                         // AT forces
                         //std::cout << " ---- get potential ---- \n";
                         const Potential &potential = getPotential(p3.type(), p4.type());
                         Real3D force(0.0, 0.0, 0.0);
                         if(potential._computeForce(force, p3, p4)) {
                             force = w12 * force;
                             p3.force() += force;
                             p4.force() -= force;
                             //std::cout << " potential OK for " << p3.id() << " - " << p4.id() << "\n";

                             /*
                             real dist = sqrt((p3.position() - p4.position()).sqr());
                             if (dist < 0.8) {
                                std::cout << p3.id() << "-" << p3.ghost()
                                        << " - " << p4.id() << "-" << p4.ghost()
                                        << " AT force: (" << force << ") "
                                        << "(" << p3.position() << ") "
                                        << "(" << p4.position() << ") "
                                        << " dist (" << dist << ")\n";
                             }*/


                         }
                         /*
                         else {
                             std::cout << " too far " << p3.id() << " - " << p4.id() << "\n";
                         }*/

                     }

                 }
                 //std::cout << "\n";
             }
             else {
                 std::cout << " one of the VP particles not found in tuples: " << p1.id() << "-" <<
                         p1.ghost() << ", " << p2.id() << "-" << p2.ghost();
                 std::cout << " (" << p1.position() << ") (" << p2.position() << ")\n";
             }
         }
      }

      weights.clear();

      // distribute forces from VP to AT
      //std::cout << "Updating forces...\n";
      for (std::set<Particle*>::iterator it=adrZone.begin();
                it != adrZone.end(); ++it) {

        Particle &vp = **it;

        FixedTupleList::iterator it3;
        it3 = fixedtupleList->find(&vp);

        //std::cout << " VP id: " << vp.id() << "\n";

        if (it3 != fixedtupleList->end()) {

            std::vector<Particle*> atList;
            atList = it3->second;

            // total mass of AT particles belonging to a VP
            real M = vp.getMass(); // instead of the code below
            /*
            real M = 0.0;
            for (std::vector<Particle*>::iterator it2 = atList.begin();
                                 it2 != atList.end(); ++it2) {
                Particle &at = **it2;
                M += at.mass();
                //std::cout << "at id: " << at.id() << "-" << at.ghost() <<
                //" pos: " << at.position() << " mass: " << at.mass() << "\n";
            }
            */


            // update force of AT particles belonging to a VP
            for (std::vector<Particle*>::iterator it2 = atList.begin();
                                 it2 != atList.end(); ++it2) {
                Particle &at = **it2;
                //std::cout << at.id() << ": old AT force: " << at.force() << "\n";
                at.force() += at.mass() * vp.force() / M;
                //std::cout << " AT mass: " << at.mass() << " VP force: " << vp.force() << " M: " << M << "\n";
                //std::cout << at.id() << ": new AT force: " << at.force() << "\n";
            }
        }
        else {
            std::cout << " particle " << vp.id() << "-" << vp.ghost() << " not found in tuples ";
            std::cout << " (" << vp.position() << ")\n";
        }
      }

      //std::cout << "\n\n";
    }
    
    template < typename _Potential >
    inline real
    VerletListAdressInteractionTemplate < _Potential >::
    computeEnergy() {
      LOG4ESPP_INFO(theLogger, "compute energy of the Verlet list pairs");

      //std::cout << "compute energy of the Verlet list pairs" << "\n";
      real e = 0.0;
      for (PairList::Iterator it(verletList->getPairs()); 
	   it.isValid(); ++it) {
        Particle &p1 = *it->first;
        Particle &p2 = *it->second;
        int type1 = p1.type();
        int type2 = p2.type();
        const Potential &potential = getPotential(type1, type2);
        e += potential._computeEnergy(p1, p2);
      }

      // adress TODO
      //std::cout << "compute energy of the AdReSs pairs" << "\n";
      for (PairList::Iterator it(verletList->getAdrPairs());
             it.isValid(); ++it) {
              Particle &p1 = *it->first;
              Particle &p2 = *it->second;
              int type1 = p1.type();
              int type2 = p2.type();
              const Potential &potential = getPotential(type1, type2);
              e += potential._computeEnergy(p1, p2);
      }

      real esum;
      boost::mpi::reduce(*getVerletList()->getSystem()->comm, e, esum, std::plus<real>(), 0);
      return esum;
    }





    template < typename _Potential > inline real
    VerletListAdressInteractionTemplate < _Potential >::
    computeVirial() {
      LOG4ESPP_INFO(theLogger, "compute the virial for the Verlet List");
      

      real w = 0.0;
      for (PairList::Iterator it(verletList->getPairs());                
           it.isValid(); ++it) {                                         
        Particle &p1 = *it->first;                                       
        Particle &p2 = *it->second;                                      
        int type1 = p1.type();                                           
        int type2 = p2.type();
        const Potential &potential = getPotential(type1, type2);

        Real3D force(0.0, 0.0, 0.0);
        if(potential._computeForce(force, p1, p2)) {
          Real3D dist = p1.position() - p2.position();
          w = w + dist * force;
        }
      }

      //adress TODO
      for (PairList::Iterator it(verletList->getAdrPairs());
                 it.isValid(); ++it) {
              Particle &p1 = *it->first;
              Particle &p2 = *it->second;
              int type1 = p1.type();
              int type2 = p2.type();
              const Potential &potential = getPotential(type1, type2);

              Real3D force(0.0, 0.0, 0.0);
              if(potential._computeForce(force, p1, p2)) {
                Real3D dist = p1.position() - p2.position();
                w = w + dist * force;
              }
      }

      real wsum;
      boost::mpi::reduce(*mpiWorld, w, wsum, std::plus<real>(), 0);
      return wsum; 
    }

    template < typename _Potential > inline void
    VerletListAdressInteractionTemplate < _Potential >::
    computeVirialTensor(Tensor& w) {
      LOG4ESPP_INFO(theLogger, "compute the virial tensor for the Verlet List");

      for (PairList::Iterator it(verletList->getPairs());
           it.isValid(); ++it) {
        Particle &p1 = *it->first;
        Particle &p2 = *it->second;
        int type1 = p1.type();
        int type2 = p2.type();
        const Potential &potential = getPotential(type1, type2);

        Real3D force(0.0, 0.0, 0.0);
        if(potential._computeForce(force, p1, p2)) {
          Real3D dist = p1.position() - p2.position();
          w += Tensor(dist, force);
        }
      }

      // adress TODO
      for (PairList::Iterator it(verletList->getAdrPairs());
                 it.isValid(); ++it) {
              Particle &p1 = *it->first;
              Particle &p2 = *it->second;
              int type1 = p1.type();
              int type2 = p2.type();
              const Potential &potential = getPotential(type1, type2);

              Real3D force(0.0, 0.0, 0.0);
              if(potential._computeForce(force, p1, p2)) {
                Real3D dist = p1.position() - p2.position();
                w += Tensor(dist, force);
              }
      }

    }
 
    template < typename _Potential >
    inline real
    VerletListAdressInteractionTemplate< _Potential >::
    getMaxCutoff() {
      real cutoff = 0.0;
      for (int i = 0; i < ntypes; i++) {
        for (int j = 0; j < ntypes; j++) {
          cutoff = std::max(cutoff, getPotential(i, j).getCutoff());
        }
      }
      return cutoff;
    }
  }
}
#endif
