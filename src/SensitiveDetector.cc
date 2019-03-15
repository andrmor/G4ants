#include "SensitiveDetector.hh"
#include "SessionManager.hh"

#include <sstream>

#include "G4Step.hh"
#include "G4ThreeVector.hh"
#include "G4SystemOfUnits.hh"

SensitiveDetector::SensitiveDetector(const G4String& name)
    : G4VSensitiveDetector(name) {}

SensitiveDetector::~SensitiveDetector() {}

G4bool SensitiveDetector::ProcessHits(G4Step* aStep, G4TouchableHistory*)
{  
    G4double edep = aStep->GetTotalEnergyDeposit();
    if (edep == 0.0) return false;

    SessionManager & SM = SessionManager::getInstance();

    const int iPart = SM.findParticle( aStep->GetTrack()->GetParticleDefinition()->GetParticleName() ); //will terminate session if not found!
    const int iMat = SM.findMaterial( aStep->GetPreStepPoint()->GetMaterial()->GetName() ); //will terminate session if not found!
    const G4ThreeVector& pos = aStep->GetPostStepPoint()->GetPosition();

    std::stringstream ss;
    ss << iPart << ' ';
    ss << iMat << ' ';
    ss << edep/keV << ' ';
    ss << pos[0] << ' ' << pos[1] << ' ' << pos[2] << ' ';
    ss << aStep->GetPostStepPoint()->GetGlobalTime()/ns;

    SM.sendLineToOutput(ss);

    return true;
}
