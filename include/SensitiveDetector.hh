#ifndef SensitiveDetector_h
#define SensitiveDetector_h

#include "G4VSensitiveDetector.hh"

class G4Step;
class G4HCofThisEvent;

class SensitiveDetector : public G4VSensitiveDetector
{
public:
    SensitiveDetector(const G4String & name);
    virtual ~SensitiveDetector();

    virtual G4bool ProcessHits(G4Step* step, G4TouchableHistory* history);
};

#endif // SensitiveDetector_h
