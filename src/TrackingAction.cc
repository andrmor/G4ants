#include "TrackingAction.hh"
#include "SessionManager.hh"

#include "G4Track.hh"
#include "G4Step.hh"
#include "G4StepPoint.hh"
#include "G4VProcess.hh"
#include "G4ParticleDefinition.hh"
#include "G4SystemOfUnits.hh"

#include <iostream>
#include <sstream>
#include <iomanip>

TrackingAction::TrackingAction(){}

TrackingAction::~TrackingAction(){}

void TrackingAction::PreUserTrackingAction(const G4Track *track)
{
    SessionManager & SM = SessionManager::getInstance();
    if (SM.CollectHistory == SessionManager::NotCollecting) return;

    // format:
    // TrackID ParentTrackID ParticleId X Y Z Time E

    std::stringstream ss;
    ss << '>';
    ss << track->GetTrackID() << ' ';
    ss << track->GetParentID() << ' ';
        //ss << SM.findParticle( track->GetParticleDefinition()->GetParticleName() ) << ' ';
    ss << track->GetParticleDefinition()->GetParticleName() << ' ';
        //ss << SM.findMaterial( track->GetVolume()->GetLogicalVolume()->GetMaterial()->GetName() ) << ' ';
    const G4ThreeVector & pos = track->GetPosition();
    ss.precision(SM.PrecisionXYZ);
    ss << pos[0] << ' ' << pos[1] << ' ' << pos[2] << ' ';
    ss.precision(SM.Precision);
    ss << track->GetGlobalTime()/ns << ' ';
    ss << track->GetKineticEnergy()/keV;
    SM.sendLineToTracksOutput(ss);
}

void TrackingAction::PostUserTrackingAction(const G4Track *)
{
    SessionManager & SM = SessionManager::getInstance();
    if (SM.CollectHistory == SessionManager::NotCollecting) return;

    if (SM.CollectHistory == SessionManager::OnlyTracks)
    {
        SM.TracksToBuild--;
        if (SM.TracksToBuild <= 0)
            SM.CollectHistory = SessionManager::NotCollecting;
    }
}
