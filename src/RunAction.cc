#include "RunAction.hh"
#include "SessionManager.hh"

#include "G4Run.hh"
#include "G4RunManager.hh"

RunAction::RunAction()
    : G4UserRunAction()
{ 
    // set printing event number per each 100 events
    //G4RunManager::GetRunManager()->SetPrintProgress(1000);
}

RunAction::~RunAction() {}

void RunAction::BeginOfRunAction(const G4Run*)
{
    SessionManager & SM = SessionManager::getInstance();

    SM.NextTrackID = 1;

    SM.sendLineToDepoOutput(SM.getEventId());

    if (SM.getNumEventsForTrackExport() > 0)
        SM.sendLineToTracksOutput(SM.getEventId());

    //inform the runManager to save random number seed             *** need?
    //G4RunManager::GetRunManager()->SetRandomNumberStore(false);
}

void RunAction::EndOfRunAction(const G4Run* )
{
    SessionManager & SM = SessionManager::getInstance();
    SM.updateEventId();
}
