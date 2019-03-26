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

    const G4VProcess * proc = step->GetPostStepPoint()->GetProcessDefinedStep();
    if (proc && proc->GetProcessType() == fTransportation)
        return; // skip transportation

    const G4StepPoint * pp = step->GetPostStepPoint();
    SM.sendLineToTracksOutput(pp->GetPosition(),
                              pp->GetTotalEnergy()/keV,
                              //step->GetTotalEnergyDeposit()/keV,
                              (proc ? proc->GetProcessName() : '?') );

    const int numSec = step->GetNumberOfSecondariesInCurrentStep();
    if (numSec > 0)
    {
        //G4StackManager * StM = G4EventManager::GetEventManager()->GetStackManager();
        //const int curStack = StM->GetNTotalTrack();
        //const int totalSec = step->GetSecondary()->size();
        //int predictedID = 2 + curStack + totalSec - numSec;

        std::stringstream ss;
        ss << " #secs: ";
        ss << step->GetNumberOfSecondariesInCurrentStep();
        ss << " predicted indeces: ";
        //const std::vector<const G4Track*>* sec = step->GetSecondaryInCurrentStep();
        //for (const G4Track * t : *sec) ss << ' ' << t->GetTrackID();

        for (int i=0; i<numSec; i++)
            ss << ' ' << SM.NextTrackID++;
        SM.sendLineToTracksOutput(ss);
    }
}
