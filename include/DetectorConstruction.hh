#ifndef DetectorConstruction_h
#define DetectorConstruction_h

#include "G4VUserDetectorConstruction.hh"

class G4VPhysicalVolume;

class DetectorConstruction : public G4VUserDetectorConstruction
{
public:
    DetectorConstruction(G4VPhysicalVolume *setWorld = 0);
    virtual ~DetectorConstruction();

public:
    virtual G4VPhysicalVolume* Construct();
    virtual void ConstructSDandField();

private:
    G4VPhysicalVolume *fWorld;
};

#endif // DetectorConstruction_h
