#include "DetectorConstruction.hh"
#include "SensitiveDetector.hh"
#include "SessionManager.hh"

#include "G4SDManager.hh"
#include "G4LogicalVolumeStore.hh"

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
    G4LogicalVolumeStore* store = G4LogicalVolumeStore::GetInstance();

    const std::vector<std::string> & SVlist = SM.getListOfSensitiveVolumes();
    for (auto & sv : SVlist)
    {
        if (sv.size() == 0) continue;

        if (sv[sv.length()-1] == '*')
        {
            //Wildcard!
            std::string wildcard = sv.substr(0, sv.size()-1);
            //std::cout << "--> Wildcard found in sensitive volume names: " << wildcard << std::endl;

            for (G4LogicalVolumeStore::iterator pos=store->begin(); pos!=store->end(); pos++)
            {
                const std::string & volName = (*pos)->GetName();
                //std::cout << "   analysing vol:" << volName << std::endl;
                if (isAccordingTo(volName, wildcard))
                {
                    //std::cout << "   match!" << std::endl;
                    SetSensitiveDetector( *pos, pSD );
                }
            }
        }
        else
            SetSensitiveDetector(sv, pSD, true);
    }
}

bool DetectorConstruction::isAccordingTo(const std::string &name, const std::string & wildcard) const
{
    const size_t size = wildcard.size();
    if (name.size() < size) return false;

    return ( wildcard == name.substr(0, size) );
}
