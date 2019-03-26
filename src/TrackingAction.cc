#include "TrackingAction.hh"
#include "SessionManager.hh"

#include "G4Track.hh"
#include "G4Step.hh"
#include "G4StepPoint.hh"
#include "G4VProcess.hh"
#include "G4ParticleDefinition.hh"
#include "G4SystemOfUnits.hh"

#include <iostream>

TrackingAction::TrackingAction(){}

TrackingAction::~TrackingAction(){}

void TrackingAction::PreUserTrackingAction(const G4Track *track)
{
    SessionManager & SM = SessionManager::getInstance();

    std::stringstream ss;
    ss << '>';
    ss << track->GetTrackID() << ' ';
    ss << track->GetParentID() << ' ';
    ss << SM.findParticle( track->GetParticleDefinition()->GetParticleName() );
    SM.sendLineToTracksOutput(ss);

    SM.sendLineToTracksOutput(track->GetPosition(), track->GetTotalEnergy()/keV, "+");
}

/*
void TrackingAction::PostUserTrackingAction(const G4Track *)
{
    SessionManager & SM = SessionManager::getInstance();

    SM.sendLineToTracksOutput(" track removed");
}
*/
