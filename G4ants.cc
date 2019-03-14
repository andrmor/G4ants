#include "SessionManager.hh"
#include "DetectorConstruction.hh"
#include "ActionInitialization.hh"

#include <chrono>

#include "G4RunManager.hh"
#include "G4UImanager.hh"
//#include "FTFP_BERT.hh"
#include "QGSP_BERT_HP.hh"
#include "G4StepLimiterPhysics.hh"
#include "G4VisExecutive.hh"
#include "G4UIExecutive.hh"
#include "G4GDMLParser.hh"

int main(int argc,char** argv)
{
    G4UIExecutive* ui = new G4UIExecutive(argc, argv);   //---------

    G4RunManager* runManager = new G4RunManager;

    SessionManager& SM = SessionManager::getInstance();
    std::string ConfigFile = "/home/andr/G4antsKraken/testJson.json"; //from arg !!!
    SM.ReadConfig(ConfigFile);

    G4GDMLParser parser;
    //parser.Read("/home/andr/G4antsKraken/G64_mat.gdml", false); //false - no validation
    parser.Read("/home/andr/G4antsKraken/My.gdml", false); //false - no validation

    runManager->SetUserInitialization(new DetectorConstruction(parser.GetWorldVolume()));

    G4VModularPhysicsList* physicsList = new QGSP_BERT_HP; //FTFP_BERT //GSP_BIC_HP;
    physicsList->RegisterPhysics(new G4StepLimiterPhysics());
    runManager->SetUserInitialization(physicsList);

    runManager->SetUserInitialization(new ActionInitialization());

    G4UImanager* UImanager = G4UImanager::GetUIpointer();
    UImanager->ApplyCommand("/run/initialize");
    UImanager->ApplyCommand("/control/verbose 0");
    UImanager->ApplyCommand("/run/verbose 0");
    UImanager->ApplyCommand("/hits/verbose 2");
    UImanager->ApplyCommand("/tracking/verbose 2"); //all details of the tracking
    UImanager->ApplyCommand("/control/saveHistory");

    // Initialize visualization
    G4VisManager* visManager = new G4VisExecutive("Quiet"); //G4VisExecutive can take a verbosity argument - see /vis/verbose guidance.
    visManager->Initialize();

    UImanager->ApplyCommand("/control/execute vis.mac"); //---------

    SM.startSession();
    SM.runSimulation();

    ui->SessionStart();   //------------

    delete ui;  //-----------
    delete visManager;
    delete runManager;

    SM.terminateSession("OK");
}
