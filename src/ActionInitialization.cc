#include "ActionInitialization.hh"
#include "SessionManager.hh"
#include "PrimaryGeneratorAction.hh"
#include "RunAction.hh"
//#include "EventAction.hh"
#include "TrackingAction.hh"
#include "SteppingAction.hh"

ActionInitialization::ActionInitialization()
    : G4VUserActionInitialization() {}

ActionInitialization::~ActionInitialization() {}

void ActionInitialization::Build() const
{
    SetUserAction(new PrimaryGeneratorAction);

    SetUserAction(new RunAction);

    //SetUserAction(new EventAction);

    SessionManager & SM = SessionManager::getInstance();
    if (SM.getNumEventsForTrackExport() > 0)
    {
        SetUserAction(new TrackingAction);
        SetUserAction(new SteppingAction);
    }
}
