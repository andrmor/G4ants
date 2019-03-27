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

TrackingAction::TrackingAction(){}

TrackingAction::~TrackingAction(){}

void TrackingAction::PreUserTrackingAction(const G4Track *track)
{
    SessionManager & SM = SessionManager::getInstance();
    if (SM.getNumEventsForTrackExport() == 0) return;

    std::stringstream ss;
    ss << '>';
    ss << track->GetTrackID() << ' ';
    ss << track->GetParentID() << ' ';
    ss << SM.findParticle( track->GetParticleDefinition()->GetParticleName() ) << ' ';
    const G4ThreeVector & pos = track->GetPosition();
    ss << pos[0] << ' ' << pos[1] << ' ' << pos[2] << ' ';
    ss << track->GetKineticEnergy()/keV;
    SM.sendLineToTracksOutput(ss);

    //std::stringstream st;
    //st << '+';
    //SM.sendLineToTracksOutput(track->GetPosition(), track->GetKineticEnergy()/keV, st);
}

/*
void TrackingAction::PostUserTrackingAction(const G4Track *)
{
    SessionManager & SM = SessionManager::getInstance();

    SM.sendLineToTracksOutput(" track removed");
}
*/
