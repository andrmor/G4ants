#include "DetectorConstruction.hh"
#include "SensitiveDetector.hh"
#include "SessionManager.hh"

#include "G4SDManager.hh"

DetectorConstruction::DetectorConstruction(G4VPhysicalVolume *setWorld)
    : G4VUserDetectorConstruction(), fWorld(setWorld) {}

DetectorConstruction::~DetectorConstruction() {}

G4VPhysicalVolume* DetectorConstruction::Construct()
{
    return fWorld;
}

void DetectorConstruction::ConstructSDandField()
{
    G4String SensitiveDetectorName = "SD";
    SensitiveDetector* pSD = new SensitiveDetector(SensitiveDetectorName);
    G4SDManager::GetSDMpointer()->AddNewDetector(pSD);

    SessionManager & SM = SessionManager::getInstance();
    const std::vector<std::string> & SVlist = SM.getListOfSensitiveVolumes();
    for (auto & sv : SVlist)
        SetSensitiveDetector(sv, pSD, true);
}
