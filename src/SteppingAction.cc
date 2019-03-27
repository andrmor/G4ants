#include "SteppingAction.hh"
#include "SessionManager.hh"

#include "G4Step.hh"
#include "G4StepPoint.hh"
#include "G4ThreeVector.hh"
#include "G4VProcess.hh"
#include "G4ProcessType.hh"
#include "G4SystemOfUnits.hh"
//#include "G4EventManager.hh"
//#include "G4StackManager.hh"

#include <iostream>

SteppingAction::SteppingAction(){}

SteppingAction::~SteppingAction(){}

void SteppingAction::UserSteppingAction(const G4Step *step)
{
    SessionManager & SM = SessionManager::getInstance();
    if (SM.getNumEventsForTrackExport() == 0) return; // use stepping action only for recording of telemetry

    const G4VProcess * proc = step->GetPostStepPoint()->GetProcessDefinedStep();
    if (proc && proc->GetProcessType() == fTransportation)
        if (step->GetPostStepPoint()->GetStepStatus() != fWorldBoundary)
            return; // skip transportation

    std::stringstream ss;
    ss << (proc ? proc->GetProcessName() : '?');
    const int numSec = step->GetNumberOfSecondariesInCurrentStep();
    if (numSec > 0)
    {
        for (int i=0; i<numSec; i++)
        {
            ss << ' ' << SM.getPredictedTrackID();
            SM.incrementPredictedTrackID();
        }
    }

    const G4StepPoint * pp = step->GetPostStepPoint();
    SM.sendLineToTracksOutput(pp->GetPosition(),
                              //pp->GetKineticEnergy()/keV,
                              step->GetTotalEnergyDeposit()/keV,
                              ss);
}
