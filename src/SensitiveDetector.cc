#include "SensitiveDetector.hh"
#include "SessionManager.hh"

#include <sstream>
#include <iomanip>

#include "G4Step.hh"
#include "G4ThreeVector.hh"
#include "G4SystemOfUnits.hh"

SensitiveDetector::SensitiveDetector(const G4String& name)
    : G4VSensitiveDetector(name) {}

SensitiveDetector::~SensitiveDetector() {}

G4bool SensitiveDetector::ProcessHits(G4Step* aStep, G4TouchableHistory*)
{  
    G4double edep = aStep->GetTotalEnergyDeposit()/keV;
    if (edep == 0.0) return false;

    SessionManager & SM = SessionManager::getInstance();

    const int iPart = SM.findParticle( aStep->GetTrack()->GetParticleDefinition()->GetParticleName() );
    const int iMat = SM.findMaterial( aStep->GetPreStepPoint()->GetMaterial()->GetName() ); //will terminate session if not found!
    const G4ThreeVector& pos = aStep->GetPostStepPoint()->GetPosition();

    // format:
    // partId matId DepoE X Y Z Time

    std::stringstream ss;
    ss.precision(SM.Precision);

    ss << iPart << ' ';
    ss << iMat << ' ';
    ss << edep << ' ';
    ss << pos[0] << ' ' << pos[1] << ' ' << pos[2] << ' ';
    ss << aStep->GetPostStepPoint()->GetGlobalTime()/ns;
    SM.sendLineToDepoOutput(ss);

    if (iPart < 0) SM.DepoByNotRegistered += edep;
    else SM.DepoByRegistered += edep;

    return true;
}

#include "json11.hh"
MonitorSensitiveDetector::MonitorSensitiveDetector(const G4String &name)
    : G4VSensitiveDetector(name) {}

MonitorSensitiveDetector::~MonitorSensitiveDetector() {}

G4bool MonitorSensitiveDetector::ProcessHits(G4Step *step, G4TouchableHistory *history)
{

}

void MonitorSensitiveDetector::readFromJson(const json11::Json &json)
{

}
