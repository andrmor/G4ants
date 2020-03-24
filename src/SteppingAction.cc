#include "SteppingAction.hh"
#include "SessionManager.hh"

#include "G4Step.hh"
#include "G4StepPoint.hh"
#include "G4ThreeVector.hh"
#include "G4VProcess.hh"
#include "G4ProcessType.hh"
#include "G4SystemOfUnits.hh"
#include "G4VUserTrackInformation.hh"

#include <iostream>
#include <iomanip>
#include <vector>

SteppingAction::SteppingAction(){}

SteppingAction::~SteppingAction(){}

void SteppingAction::UserSteppingAction(const G4Step *step)
{
    SessionManager & SM = SessionManager::getInstance();

    if (SM.bMonitorsRequireSteppingAction)
        if (!step->GetTrack()->GetUserInformation()) //if exists, already marked as "indirect"
        {
            const G4VProcess * proc = step->GetPostStepPoint()->GetProcessDefinedStep();
            if (proc && proc->GetProcessType() != fTransportation) // on first non-transportation, the particle is marked as "indirect"
                step->GetTrack()->SetUserInformation(new G4VUserTrackInformation()); // owned by track!
        }

    if (SM.CollectHistory == SessionManager::NotCollecting) return; // the rest is only to record telemetry!

    if (SM.bStoppedOnMonitor) // bug fix for Geant4 - have to be removed when it is fixed! Currently track has one more step after kill
    {
        SM.bStoppedOnMonitor = false;
        return;
    }

    const G4VProcess * proc = step->GetPostStepPoint()->GetProcessDefinedStep();
    if (proc && proc->GetProcessType() == fTransportation)
        if (step->GetPostStepPoint()->GetStepStatus() != fWorldBoundary && SM.CollectHistory == SessionManager::OnlyTracks)
            return; // skip transportation if only collecting tracks

    // format for "T" processes:
    // ProcName X Y Z Time KinE DirectDepoE iMatTo VolNameTo VolIndexTo [secondaries]
    // not that if energy depo is present on T step, it is in the previous volume!

    // format for all other processes:
    // ProcName X Y Z Time KinE DirectDepoE [secondaries]


    bool bTransport = false;

    /*
    std::stringstream ss;
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
    */

    std::string procName;
    if (proc)
    {
        if (proc->GetProcessType() == fTransportation)
        {
            if (step->GetPostStepPoint()->GetStepStatus() != fWorldBoundary)
            {
                procName = 'T';
                bTransport = true;
            }
            else procName = 'O';
        }
        else procName = proc->GetProcessName();
    }
    else procName = '?';

    const G4ThreeVector & pos = step->GetPostStepPoint()->GetPosition();
    const double time = step->GetPostStepPoint()->GetGlobalTime()/ns;
    const double kinE = step->GetPostStepPoint()->GetKineticEnergy()/keV;
    const double depo = step->GetTotalEnergyDeposit()/keV;

    const std::vector<int> * secondaries = nullptr;
    const int numSec = step->GetNumberOfSecondariesInCurrentStep();
    if (numSec > 0)
    {
        TmpSecondaries.resize(numSec);
        for (int iSec = 0; iSec < numSec; iSec++)
        {
            TmpSecondaries[iSec] = SM.getPredictedTrackID();
            SM.incrementPredictedTrackID();
        }
        secondaries = &TmpSecondaries;
    }

    if (bTransport)
    {
        const int iMat = SM.findMaterial( step->GetPostStepPoint()->GetMaterial()->GetName() ); //will terminate session if not found!
        const std::string & VolNameTo = step->GetPostStepPoint()->GetPhysicalVolume()->GetLogicalVolume()->GetName();
        const int VolIndexTo = step->GetPostStepPoint()->GetPhysicalVolume()->GetCopyNo();

        SM.saveTrackRecord(procName, pos, time, kinE, depo, secondaries, iMat, VolNameTo, VolIndexTo);
    }
    else
        SM.saveTrackRecord(procName, pos, time, kinE, depo, secondaries);
}
