#include "SessionManager.hh"

#include <iostream>
#include <sstream>
#include <fstream>
#include <map>

#include "G4ParticleDefinition.hh"
#include "G4ParticleTable.hh"
#include "G4IonTable.hh"
#include "G4UImanager.hh"
#include "Randomize.hh"

SessionManager &SessionManager::getInstance()
{
    static SessionManager instance; // Guaranteed to be destroyed, instantiated on first use.
    return instance;
}

SessionManager::SessionManager() {}

SessionManager::~SessionManager()
{
    delete outStreamDeposition;
    delete outStreamTracks;
    delete inStreamPrimaries;
}

void SessionManager::startSession()
{
    //populate particle collection
    prepareParticleCollection();

    // opening file with primaries
    prepareInputStream();

    // preparing ouptut for deposition data
    prepareOutputDepoStream();

    // preparing ouptut for track export
    if (CollectHistory != NotCollecting) prepareOutputTracks();

    //set random generator. The seed was provided in the config file
    CLHEP::RanecuEngine* randGen = new CLHEP::RanecuEngine();
    randGen->setSeed(Seed);
    G4Random::setTheEngine(randGen);

    executeAdditionalCommands();
}

void SessionManager::terminateSession(const std::string & ReturnMessage)
{
    std::cout << "$$>"<<ReturnMessage<<std::endl;

    bError = true;
    ErrorMessage = ReturnMessage;
    generateReceipt();

    exit(0);
}

void SessionManager::endSession()
{
    bError = false;
    ErrorMessage.clear();

    generateReceipt();
}

void SessionManager::runSimulation()
{
    G4UImanager* UImanager = G4UImanager::GetUIpointer();

    DepoByRegistered = 0;
    DepoByNotRegistered = 0;

    EventsDone = 0;
    ProgressLastReported = 0;
    if (NumEventsToDo != 0)
        ProgressInc = 100.0 / NumEventsToDo;

    while (!isEndOfInputFileReached())
        UImanager->ApplyCommand("/run/beamOn");
}

void SessionManager::onRunFinished()
{
    updateEventId();

    EventsDone++;
    double Progress = (double)EventsDone * ProgressInc;
    if (Progress - ProgressLastReported > 1.0) //1% intervales
    {
        std::cout << "$$progress>" << (int)Progress << "<$$"<< std::endl << std::flush;
        ProgressLastReported = Progress;
    }
}

void SessionManager::updateEventId()
{
    EventId = NextEventId;
}

std::vector<ParticleRecord> &SessionManager::getNextEventPrimaries()
{
    GeneratedPrimaries.clear();

    for( std::string line; getline( *inStreamPrimaries, line ); )
    {
        //std::cout << line << std::endl;
        if (line.size() < 1) continue; //allow empty lines

        if (line[0] == '#')
        {
            NextEventId = line;
            break; //event finished
        }

        ParticleRecord r;
        int Id;
        int numRead = std::sscanf(line.data(), "%d %lf %lf %lf %lf %lf %lf %lf %lf",
                                  &Id,
                                  &r.Energy,
                                  &r.Position[0],  &r.Position[1],  &r.Position[2],
                                  &r.Direction[0], &r.Direction[1], &r.Direction[2],
                                  &r.Time);
        if (numRead != 9)
            terminateSession("Unexpected format of file with primaries");

        if (Id >= 0 || Id < (int)ParticleCollection.size() )
            r.Particle = ParticleCollection[Id];
        if (!r.Particle)
            terminateSession("Use of unknown particle index");

        GeneratedPrimaries.push_back( r );
    }

    return GeneratedPrimaries;
}

bool SessionManager::isEndOfInputFileReached() const
{
    if (!inStreamPrimaries) return true;
    return inStreamPrimaries->eof();
}

int SessionManager::findParticle(const std::string & particleName)
{
    auto it = ParticleMap.find(particleName);
    if (it == ParticleMap.end())
    {
        //terminateSession("Found deposition by particle not listed in the config json: " + particleName);
        SeenNotRegisteredParticles.insert(particleName);
        return -1;
    }

    return it->second;
}

int SessionManager::findMaterial(const std::string &materialName)
{
    auto it = MaterialMap.find(materialName);
    if (it == MaterialMap.end())
        terminateSession("Found deposition in materials not listed in the config json: " + materialName);

    return it->second;
}

void SessionManager::sendLineToDepoOutput(const std::string & text)
{
    if (!outStreamDeposition) return;

    *outStreamDeposition << text.data() << std::endl;
}

void SessionManager::sendLineToDepoOutput(const std::stringstream & text)
{
    if (!outStreamDeposition) return;

    *outStreamDeposition << text.rdbuf() << std::endl;
}

void SessionManager::sendLineToTracksOutput(const std::string &text)
{
    if (!outStreamTracks) return;

    *outStreamTracks << text.data() << std::endl;
}

void SessionManager::sendLineToTracksOutput(const std::stringstream &text)
{
    if (!outStreamTracks) return;

    *outStreamTracks << text.rdbuf() << std::endl;
}

void SessionManager::prepareParticleCollection()
{
    ParticleMap.clear();
    ParticleCollection.clear();

    std::cout << "Config lists the following particles:" << std::endl;
    for (size_t i=0; i<ParticleJsonArray.size(); i++)
    {
        const json11::Json & j = ParticleJsonArray[i];
        std::string name = "";
        G4ParticleDefinition * pParticleDefinition = nullptr;
        if (j.is_string())
        {
            name = j.string_value();
            std::cout << name << std::endl;
            pParticleDefinition = G4ParticleTable::GetParticleTable()->FindParticle(name);
        }
        else if (j.is_array())
        {
            //ion
            std::vector<json11::Json> par = j.array_items();
            if (par.size() == 3)
            {
                name = par[0].string_value();
                int Z = par[1].int_value();
                int A = par[2].int_value();

                pParticleDefinition = G4ParticleTable::GetParticleTable()->GetIonTable()->GetIon(Z, A);
                std::cout << name << " Z=" << Z << " A=" << A << std::endl;
            }
        }
        if (name.empty())
            terminateSession("Bad format of particle record in config");
        if (!pParticleDefinition)
            terminateSession(name + " - particle not found in Geant4 particle/ion table!");

        ParticleMap[name] = (int)i;
        ParticleCollection.push_back(pParticleDefinition);
    }
}

void SessionManager::ReadConfig(const std::string &ConfigFileName)
{
    //opening config file
    std::ifstream in(ConfigFileName);
    std::stringstream sstr;
    sstr << in.rdbuf();
    std::string s = sstr.str();

    std::cout << s << std::endl;

    std::string err;
    json11::Json jo = json11::Json::parse(s, err);
    if (!err.empty())
        terminateSession(err);

    //extracting name of the receipt file - should be first so we can report back to known receipt file!
    FileName_Receipt = jo["File_Receipt"].string_value();
    if (FileName_Receipt.empty())
        terminateSession("File name for receipt was not provided");

    GDML = jo["GDML"].string_value();
    if (GDML.empty())
        terminateSession("GDML file name is not provided");

    PhysicsList = jo["PhysicsList"].string_value();
    if (PhysicsList.empty())
        terminateSession("Reference physics list is not provided");

    //extracting name of the file with primaries to generate
    FileName_Input = jo["File_Primaries"].string_value();
    if (FileName_Input.empty())
        terminateSession("File name with primaries to generate was not provided");

    //extracting name of the file for deposition output
    FileName_Output = jo["File_Deposition"].string_value();
    if (FileName_Output.empty())
        terminateSession("File name for deposition output was not provided");

    //read list of sensitive volumes - they will be linked to SensitiveDetector
    std::vector<json11::Json> arSV = jo["SensitiveVolumes"].array_items();
    if (arSV.empty())
        terminateSession("Sensitive volumes are not provided in the configuration file!");
    SensitiveVolumes.clear();
    std::cout << "Sensitive volumes:" << std::endl;
    for (auto & j : arSV)
    {
        std::string name = j.string_value();
        std::cout << name << std::endl;
        SensitiveVolumes.push_back(name);
    }

    //read on-start Geant4 commands
    std::vector<json11::Json> arC = jo["Commands"].array_items();
    OnStartCommands.clear();
    if (!arSV.empty())
    {
        std::cout << "On-start commands:" << std::endl;
        for (auto & j : arC)
        {
            std::string cmd = j.string_value();
            std::cout << cmd << std::endl;
            OnStartCommands.push_back(cmd);
        }
    }

    //read and configure random gen seed
    if (jo.object_items().count("Seed") == 0)
        terminateSession("Seed is not provided in the config file");
    if (!jo["Seed"].is_number())
        terminateSession("Format error for the random generator seed in the config file");
    Seed = jo["Seed"].int_value();
    if (Seed == 0)
        terminateSession("Seed: read from the config file failed");
    std::cout << "Random generator seed: " << Seed << std::endl;

    //extracting particle info
    ParticleJsonArray = jo["Particles"].array_items();
    if (ParticleJsonArray.empty())
        terminateSession("Particles are not defined in the configuration file!");

    //extracting defined materials
    MaterialMap.clear();
    std::vector<json11::Json> Marr = jo["Materials"].array_items();
    if (Marr.empty())
        terminateSession("Materials are not defined in the configuration file!");
    std::cout << "Config lists the following materials:" << std::endl;
    for (size_t i=0; i<Marr.size(); i++)
    {
        const json11::Json & j = Marr[i];
        std::string name = j.string_value();
        std::cout << name << std::endl;
        MaterialMap[name] = (int)i;
    }

    //extracting step limits
    StepLimitMap.clear();
    std::vector<json11::Json> StepLimitArray = jo["StepLimits"].array_items();
    if (!StepLimitArray.empty())
    {
        for (size_t i=0; i<StepLimitArray.size(); i++)
        {
            const json11::Json & el = StepLimitArray[i];
            std::vector<json11::Json> par = el.array_items();
            std::cout << "Defined step limiters:" << std::endl;
            if (par.size() > 1)
            {
                std::string vol = par[0].string_value();
                double step     = par[1].number_value();
                StepLimitMap[vol] = step;
                std::cout << vol << " -> " << step << " mm" << std::endl;
            }
        }
    }

    bGuiMode = jo["GuiMode"].bool_value();

    NumEventsToDo = jo["NumEvents"].int_value();

    //Tracks export
    //json["LogHistory"] = bLogHistory;
    //json["BuildTracks"] = bBuildTracks;
    //if (bBuildTracks) json["MaxTracks"] = maxTracks;
    //NumberEventsForTrackExport = jo["MaxEventsForTrackExport"].int_value();
    bool bBuildTracks = jo["BuildTracks"].bool_value();
    bool bLogHistory = jo["LogHistory"].bool_value();
    TracksToBuild = jo["MaxTracks"].int_value();
    FileName_Tracks = jo["File_Tracks"].string_value();
    if ( (bBuildTracks || bLogHistory) && FileName_Tracks.empty())
        terminateSession("File name with tracks to export was not provided");

    if (bLogHistory) CollectHistory = FullLog;
    else if (bBuildTracks && TracksToBuild > 0) CollectHistory = OnlyTracks;
    else CollectHistory = NotCollecting;
}

#include "G4LogicalVolumeStore.hh"
#include <QDebug>
#include "G4SystemOfUnits.hh"
#include "G4UserLimits.hh"
void SessionManager::SetStepControl()
{
    if (StepLimitMap.empty()) return;

    G4LogicalVolumeStore* lvs = G4LogicalVolumeStore::GetInstance();
    for (auto const & it : StepLimitMap)
    {
        std::string VolName = it.first;
        G4LogicalVolume * v = lvs->GetVolume("PrScint");
        if (!v)
        {
            terminateSession("Step limiter: not found volume " + VolName);
            return;
        }

        double step = it.second * mm;
        if (step == 0)
        {
            terminateSession("Found zero step limit for volume " + VolName);
            return;
        }

        G4UserLimits * stepLimit = new G4UserLimits(step);
        v->SetUserLimits(stepLimit);
        //stepLimit->SetMaxAllowedStep(maxStep);
    }
}

void SessionManager::prepareInputStream()
{
    inStreamPrimaries = new std::ifstream(FileName_Input);
    if (!inStreamPrimaries->is_open())
        terminateSession("Cannot open file with primaries to generate");

    getline( *inStreamPrimaries, EventId );
    if (EventId.size()<2 || EventId[0] != '#')
        terminateSession("Unexpected format of the file with primaries");

    std::cout << EventId << std::endl;
}

void SessionManager::prepareOutputDepoStream()
{
    outStreamDeposition = new std::ofstream();
    outStreamDeposition->open(FileName_Output);
    if (!outStreamDeposition->is_open())
        terminateSession("Cannot open file to store deposition data");
}

void SessionManager::prepareOutputTracks()
{
    outStreamTracks = new std::ofstream();
    outStreamTracks->open(FileName_Tracks);
    if (!outStreamTracks->is_open())
        terminateSession("Cannot open file to export tracks data");
}

void SessionManager::executeAdditionalCommands()
{
    G4UImanager* UImanager = G4UImanager::GetUIpointer();
    for (auto & cmd : OnStartCommands)
        UImanager->ApplyCommand(cmd);

    UImanager->ApplyCommand("/run/initialize");
}

void SessionManager::generateReceipt()
{
    json11::Json::object receipt;

    receipt["Success"] = !bError;
    if (bError) receipt["Error"] = ErrorMessage;

    receipt["DepoByRegistered"] = DepoByRegistered;
    receipt["DepoByNotRegistered"] = DepoByNotRegistered;

    json11::Json::array NRP;
    for (const std::string & snr : SeenNotRegisteredParticles)
        NRP.push_back(snr);
    if (!NRP.empty()) receipt["SeenNotRegisteredParticles"] = NRP;

    std::string json_str = json11::Json(receipt).dump();

    std::ofstream outStream;
    outStream.open(FileName_Receipt);
    if (outStream.is_open())
        outStream << json_str << std::endl;
    outStream.close();
}
