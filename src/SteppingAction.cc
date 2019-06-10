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
#include <iomanip>

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

    // format for "T" processes:
    // ProcName X Y Z Time KinE DirectDepoE iMatTo VolNameTo VolIndexTo [secondaries]
    // not that if energy depo is present on T step, it is in the previous volume!

    // format for all other processes:
    // ProcName X Y Z Time KinE DirectDepoE [secondaries]

    std::stringstream ss;

    bool bTransport = false;

    if (proc)
    {
        if (proc->GetProcessType() == fTransportation)
        {
            if (step->GetPostStepPoint()->GetStepStatus() != fWorldBoundary)
            {
                ss << 'T';
                bTransport = true;
            }
            else ss << 'O';
        }
        else ss << proc->GetProcessName();
    }
    else ss << '?';

    const G4ThreeVector & pos = step->GetPostStepPoint()->GetPosition();
    ss.precision(SM.Precision);

    ss << ' ' << pos[0] << ' ' << pos[1] << ' ' << pos[2] << ' ';
    ss << step->GetPostStepPoint()->GetGlobalTime()/ns << ' ';
    ss << step->GetPostStepPoint()->GetKineticEnergy()/keV << ' ';
    ss << step->GetTotalEnergyDeposit()/keV;

    if (bTransport)
    {
        const int iMat = SM.findMaterial( step->GetPostStepPoint()->GetMaterial()->GetName() ); //will terminate session if not found!
        ss << ' ' << iMat << ' ';
        ss << step->GetPostStepPoint()->GetPhysicalVolume()->GetLogicalVolume()->GetName() << ' ';
        ss << step->GetPostStepPoint()->GetPhysicalVolume()->GetCopyNo() << ' ';
    }

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
