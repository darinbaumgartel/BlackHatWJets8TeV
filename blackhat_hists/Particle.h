#ifndef PARTICLE_H
#define PARTICLE_H

#include "TLorentzVector.h"
#include <iostream>

using namespace std;

class Particle : public TLorentzVector {
  
 private:
  int _pdgid;
  
 public:
  
  Particle():
    TLorentzVector(), _pdgid(0){}
 
  Particle(TLorentzVector v, int id): 
    TLorentzVector(v), _pdgid(id){}
    
  inline const int pdgid() const { return _pdgid; }
  inline const int id() const { return _pdgid; }
  inline void pdgid(int id){ _pdgid=id; }
  
};
#endif
