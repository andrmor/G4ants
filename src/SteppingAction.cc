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
    if (SM.CollectHistory == SessionManager::NotCollecting) return; // use stepping action only for recording of telemetry

    const G4VProcess * proc = step->GetPostStepPoint()->GetProcessDefinedStep();
    if (proc && proc->GetProcessType() == fTransportation)
        if (step->GetPostStepPoint()->GetStepStatus() != fWorldBoundary && SM.CollectHistory == SessionManager::OnlyTracks)
            return; // skip transportation if only collecting tracks

    // format:
    // X Y Z Time DirectDepoE ProcName [secondaries]

    std::stringstream ss;
    const G4ThreeVector & pos = step->GetPostStepPoint()->GetPosition();
    ss << pos[0] << ' ' << pos[1] << ' ' << pos[2] << ' ';
    ss << step->GetPostStepPoint()->GetGlobalTime()/ns << ' ';
    ss << step->GetTotalEnergyDeposit()/keV << ' ';
    if (proc)
    {
        if (proc->GetProcessType() == fTransportation)
        {
            if (step->GetPostStepPoint()->GetStepStatus() != fWorldBoundary) ss << 'T';
            else ss << "O";
        }
        else ss << proc->GetProcessName();
    }
    else ss << '?';

    const int numSec = step->GetNumberOfSecondariesInCurrentStep();
    if (numSec > 0)
    {
        for (int i=0; i<numSec; i++)
        {
            ss << ' ' << SM.getPredictedTrackID();
            SM.incrementPredictedTrackID();
        }
    }

    SM.sendLineToTracksOutput(ss);
}
