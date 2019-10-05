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

MonitorSensitiveDetector::~MonitorSensitiveDetector()
{
    std::cout << "Deleting monitor object" << std::endl;
}

#include "G4VProcess.hh"
G4bool MonitorSensitiveDetector::ProcessHits(G4Step *step, G4TouchableHistory *)
{
    SessionManager & SM = SessionManager::getInstance();

    const G4VProcess * proc = step->GetPostStepPoint()->GetProcessDefinedStep();
    if (proc && proc->GetProcessType() == fTransportation)
        if (step->GetPostStepPoint()->GetStepStatus() == fGeomBoundary)
        {
            std::cout << "Enter!" << (step->GetTrack()->GetParticleDefinition() == pParticleDefinition ? " GOOD one!" : " wrong!") <<std::endl;

            // time info
            double time = step->GetPostStepPoint()->GetGlobalTime()/ns;
            int iTime = (time - timeFrom) / timeDelta;
            if (iTime < 0) vTime[0]++;
            else if (iTime >= timeBins) vTime[timeBins+1]++;
            else vTime[iTime+1]++;

            //position info
            G4StepPoint* p1 = step->GetPreStepPoint();
            G4ThreeVector coord1 = p1->GetPosition();
            const G4AffineTransform transformation = p1->GetTouchable()->GetHistory()->GetTopTransform();
            G4ThreeVector localPosition = transformation.TransformPoint(coord1);
            std::cout << "Local position: " << localPosition[0] << " " << localPosition[1] << " " << localPosition[2] << " " << std::endl;


        }

//    G4VUserTrackInformation* GetUserInformation() const;
//    void SetUserInformation(G4VUserTrackInformation* aValue) const;


//    const int iPart = SM.findParticle( aStep->GetTrack()->GetParticleDefinition()->GetParticleName() );
//    const G4ThreeVector& pos = aStep->GetPostStepPoint()->GetPosition();

//    ss << a

    return true;
}

void MonitorSensitiveDetector::readFromJson(const json11::Json &json)
{
    Name =          json["Name"].string_value();
    ParticleName =  json["ParticleName"].string_value();
    std::cout << "Monitor created for volume " << Name << " and particle " << ParticleName << std::endl;

    bLower =        json["bLower"].bool_value();
    bUpper =        json["bUpper"].bool_value();
    bStopTracking = json["bStopTracking"].bool_value();
    bDirect =       json["bDirect"].bool_value();
    bIndirect =     json["bIndirect"].bool_value();
    bPrimary =      json["bPrimary"].bool_value();
    bSecondary =    json["bSecondary"].bool_value();

    angleBins =     json["angleBins"].int_value();
    angleFrom =     json["angleFrom"].number_value();
    angleTo =       json["angleTo"].number_value();

    energyBins =    json["energyBins"].int_value();
    energyFrom =    json["energyFrom"].number_value();
    energyTo =      json["energyTo"].number_value();
    energyUnits =   json["energyUnitsInHist"].int_value();

    timeBins =      json["timeBins"].int_value();
    timeFrom =      json["timeFrom"].number_value();
    timeTo =        json["timeTo"].number_value();

    xbins =        json["xbins"].int_value();
    ybins =        json["ybins"].int_value();
    size1 =        json["size1"].number_value();
    int shape =    json["shape"].number_value();
    if (shape == 0) //rectangular
        size2 =   json["size2"].number_value();
    else
        size2 = size1;

    // creating histograms to store statistics
    vTime.resize(timeBins+2);
    timeDelta = (timeTo - timeFrom) / timeBins;
}
