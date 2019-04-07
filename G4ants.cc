#include "SessionManager.hh"
#include "DetectorConstruction.hh"
#include "ActionInitialization.hh"

//#include <chrono>

#include "G4RunManager.hh"
#include "G4UImanager.hh"
//#include "FTFP_BERT.hh"
//#include "QGSP_BERT_HP.hh"
#include "QGSP_BIC_HP.hh"
#include "G4StepLimiterPhysics.hh"
#include "G4VisExecutive.hh"
#include "G4UIExecutive.hh"
#include "G4GDMLParser.hh"

int main(int argc, char** argv)
{
    SessionManager& SM = SessionManager::getInstance();
    if (argc < 2)
        SM.terminateSession("Config file not provided as the first argument");
    SM.ReadConfig(argv[1]);
    bool bGui = SM.isGuiMode();

    G4UIExecutive* ui =  0;
    if (bGui) ui = new G4UIExecutive(argc, argv);

    G4RunManager* runManager = new G4RunManager;
    G4GDMLParser parser;
    parser.Read(SM.getGDML(), false); //false - no validation
    // need to implement own G4excpetion-based handler class  ->  SM.terminateSession("Error parsing GDML file");
    runManager->SetUserInitialization(new DetectorConstruction(parser.GetWorldVolume()));

    //G4VModularPhysicsList* physicsList = new QGSP_BERT_HP; //FTFP_BERT
    G4VModularPhysicsList* physicsList = new QGSP_BIC_HP;
    physicsList->RegisterPhysics(new G4StepLimiterPhysics());
    runManager->SetUserInitialization(physicsList);

    runManager->SetUserInitialization(new ActionInitialization());

    G4UImanager* UImanager = G4UImanager::GetUIpointer();
    //UImanager->ApplyCommand("/run/initialize");
    UImanager->ApplyCommand("/control/verbose 0");
    UImanager->ApplyCommand("/run/verbose 0");
    if (bGui)
    {
        UImanager->ApplyCommand("/hits/verbose 2");
        UImanager->ApplyCommand("/tracking/verbose 2");
        UImanager->ApplyCommand("/control/saveHistory");
    }

    SM.startSession();

    G4VisManager* visManager = 0;

    if (!SM.isGuiMode())
        SM.runSimulation();
    else
    {

        visManager = new G4VisExecutive("Quiet");
        visManager->Initialize();
        UImanager->ApplyCommand("/control/execute vis.mac");
        ui->SessionStart();
    }

    delete visManager;
    delete runManager;
    delete ui;

    SM.endSession();
}
