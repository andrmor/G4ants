#include "SteppingAction.hh"
#include "SessionManager.hh"

#include "G4Step.hh"
#include "G4StepPoint.hh"
#include "G4ThreeVector.hh"
#include "G4VProcess.hh"
#include "G4ProcessType.hh"
#include "G4SystemOfUnits.hh"

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

    if (step->GetNumberOfSecondariesInCurrentStep() > 0)
    {
        //std::stringstream ss;
        //const std::vector<const G4Track*>* sec = step->GetSecondaryInCurrentStep();
        //for (const G4Track * t : *sec) ss << ' ' << t->GetTrackID();
        //SM.sendLineToTracksOutput(ss);
    }
}
