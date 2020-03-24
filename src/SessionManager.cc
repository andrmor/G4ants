#include "SessionManager.hh"

#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
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

    //prepare monitors: populate particle pointers
    prepareMonitors();

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

    storeMonitorsData();

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

/*
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
*/

void SessionManager::saveDepoEventId()
{
    if (!outStreamDeposition) return;

    if (bBinaryOutput)
    {
        *outStreamDeposition << char(0xEE);

        const int iEvent = std::stoi( EventId.substr(1) );  // kill leading '#'
        outStreamDeposition->write((char*)&iEvent, sizeof(int));
    }
    else
    {
        *outStreamDeposition << EventId.data() << std::endl;
    }
}

void SessionManager::saveDepoRecord(int iPart, int iMat, double edep, double *pos, double time)
{
    if (!outStreamDeposition) return;

    // format:
    // partId matId DepoE X Y Z Time

    if (bBinaryOutput)
    {
        *outStreamDeposition << char(0xFF);

        outStreamDeposition->write((char*)&iPart,   sizeof(int));
        outStreamDeposition->write((char*)&iMat,    sizeof(int));
        outStreamDeposition->write((char*)&edep,    sizeof(double));
        outStreamDeposition->write((char*)pos,    3*sizeof(double));
        outStreamDeposition->write((char*)&time,    sizeof(double));
    }
    else
    {
        std::stringstream ss;
        ss.precision(Precision);

        ss << iPart << ' ';
        ss << iMat << ' ';
        ss << edep << ' ';
        ss << pos[0] << ' ' << pos[1] << ' ' << pos[2] << ' ';
        ss << time;

        *outStreamDeposition << ss.rdbuf() << std::endl;
    }
}

/*
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
*/

void SessionManager::saveTrackEventId()
{
    if (!outStreamTracks) return;

    if (bBinaryOutput)
    {
        *outStreamTracks << char(0xEE);

        const int iEvent = std::stoi( EventId.substr(1) );  // kill leading '#'
        outStreamTracks->write((char*)&iEvent, sizeof(int));
    }
    else
    {
        *outStreamTracks << EventId.data() << std::endl;
    }
}

void SessionManager::saveTrackStart(int trackID, int parentTrackID,
                                    const G4String & particleName,
                                    const G4ThreeVector & pos, double time, double kinE,
                                    int iMat, const std::string &volName, int volIndex)
{
    if (!outStreamTracks) return;

    // format:
    // > TrackID ParentTrackID Particle X Y Z Time E iMat VolName VolIndex

    if (bBinaryOutput)
    {
        *outStreamDeposition << char(0xFF);

        outStreamDeposition->write((char*)&trackID,       sizeof(int));
        outStreamDeposition->write((char*)&parentTrackID, sizeof(int));

        *outStreamDeposition << particleName << char(0x00);

        double posArr[3];
        posArr[0] = pos.x();
        posArr[1] = pos.y();
        posArr[2] = pos.z();
        outStreamDeposition->write((char*)posArr,  3*sizeof(double));
        outStreamDeposition->write((char*)&time,     sizeof(double));
        outStreamDeposition->write((char*)&kinE,     sizeof(double));

        outStreamDeposition->write((char*)&iMat,     sizeof(int));
        *outStreamDeposition << volName << char(0x00);
        outStreamDeposition->write((char*)&volIndex, sizeof(int));
    }
    else
    {
        std::stringstream ss;
        ss.precision(Precision);

        ss << '>';
        ss << trackID << ' ';
        ss << parentTrackID << ' ';
        ss << particleName << ' ';
        ss << pos[0] << ' ' << pos[1] << ' ' << pos[2] << ' ';
        ss << time << ' ';
        ss << kinE << ' ';
        ss << iMat << ' ';
        ss << volName << ' ';
        ss << volIndex;

        *outStreamTracks << ss.rdbuf() << std::endl;
    }

}

void SessionManager::saveTrackRecord(const std::string & procName,
                                     const G4ThreeVector & pos, double time,
                                     double kinE, double depoE,
                                     const std::vector<int> * secondaries,
                                     int iMatTo, const std::string & volNameTo, int volIndexTo)
{
    if (!outStreamTracks) return;

    // format for "T" processes:
    // ascii: ProcName  X Y Z Time KinE DirectDepoE iMatTo VolNameTo  VolIndexTo [secondaries] \n
    // bin:   ProcName0 X Y Z Time KinE DirectDepoE iMatTo VolNameTo0 VolIndexTo numSec [secondaries]
    // not that if energy depo is present on T step, it is in the previous volume!
    if (bBinaryOutput)
    {
        *outStreamDeposition << char( iMatTo == -1 ? 0xFF    // not a transportation step, next material is saved too
                                                   : 0xF8 ); // transportation step

        *outStreamDeposition << procName << char(0x00);

        double posArr[3];
        posArr[0] = pos.x();
        posArr[1] = pos.y();
        posArr[2] = pos.z();
        outStreamDeposition->write((char*)posArr,  3*sizeof(double));
        outStreamDeposition->write((char*)&time,     sizeof(double));

        outStreamDeposition->write((char*)&kinE,     sizeof(double));
        outStreamDeposition->write((char*)&depoE,    sizeof(double));

        if (iMatTo != -1)
        {
            outStreamDeposition->write((char*)&iMatTo,     sizeof(int));
            *outStreamDeposition << volNameTo << char(0x00);
            outStreamDeposition->write((char*)&volIndexTo, sizeof(int));
        }

        int numSec = (secondaries ? secondaries->size() : 0);
        outStreamDeposition->write((char*)numSec, sizeof(int));
        if (secondaries)
        {
            for (const int & iSec : *secondaries)
                outStreamDeposition->write((char*)iSec, sizeof(int));
        }
    }
    else
    {
        std::stringstream ss;
        ss.precision(Precision);

        ss << procName << ' ';

        ss << ' ' << pos[0] << ' ' << pos[1] << ' ' << pos[2] << ' ';
        ss << time << ' ';

        ss << kinE << ' ';
        ss << depoE;

        if (iMatTo != -1)
        {
            ss << ' ';
            ss << iMatTo << ' ';
            ss << volNameTo << ' ';
            ss << volIndexTo;
        }

        if (secondaries)
        {
            for (const int & isec : *secondaries)
                ss << ' ' << isec;
        }

        *outStreamTracks << ss.rdbuf() << std::endl;
    }
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

#include "SensitiveDetector.hh"
void SessionManager::prepareMonitors()
{
    for (MonitorSensitiveDetector * m : Monitors)
    {
        if (!m->ParticleName.empty())
            m->pParticleDefinition = G4ParticleTable::GetParticleTable()->FindParticle(m->ParticleName);
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

    //extracting name of the receipt file - should be first so we can report back to the known receipt file!
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

    //extracting name of the monitor output
    FileName_Monitors = jo["File_Monitors"].string_value();
    //if (FileName_Monitors.empty())
    //    terminateSession("File name for monitor data output was not provided");

    //read list of sensitive volumes - they will be linked to SensitiveDetector
    std::vector<json11::Json> arSV = jo["SensitiveVolumes"].array_items();
    if (arSV.empty())
        WarningMessages.push_back("Sensitive volumes are not provided in the configuration file!");
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
        std::cout << "Defined step limiters:" << std::endl;
        for (size_t i=0; i<StepLimitArray.size(); i++)
        {
            const json11::Json & el = StepLimitArray[i];
            std::vector<json11::Json> par = el.array_items();
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

    if (jo.object_items().count("BinaryOutput") == 0)
        bBinaryOutput = false;
    else
        bBinaryOutput = jo["BinaryOutput"].bool_value();
    std::cout << "Binary output?" << bBinaryOutput << std::endl;

    NumEventsToDo = jo["NumEvents"].int_value();

    bool bBuildTracks = jo["BuildTracks"].bool_value();
    bool bLogHistory = jo["LogHistory"].bool_value();
    TracksToBuild = jo["MaxTracks"].int_value();
    FileName_Tracks = jo["File_Tracks"].string_value();
    if ( (bBuildTracks || bLogHistory) && FileName_Tracks.empty())
        terminateSession("File name with tracks to export was not provided");

    if (bLogHistory) CollectHistory = FullLog;
    else if (bBuildTracks && TracksToBuild > 0) CollectHistory = OnlyTracks;
    else CollectHistory = NotCollecting;

    Precision = jo["Precision"].int_value();

    if (!FileName_Monitors.empty()) //compatibility while the corresponding ANTS version is not on master
    {
        std::vector<json11::Json> MonitorArray = jo["Monitors"].array_items();
        for (size_t i=0; i<MonitorArray.size(); i++)
        {
            const json11::Json & mjs = MonitorArray[i];
            std::string Name = mjs["Name"].string_value();
            MonitorSensitiveDetector * mobj = new MonitorSensitiveDetector(Name);
            mobj->readFromJson(mjs);
            Monitors.push_back(mobj);
            if (!mobj->bAcceptDirect || !mobj->bAcceptIndirect) bMonitorsRequireSteppingAction = true;
        }
        std::cout << "Monitors require stepping action: " << bMonitorsRequireSteppingAction << std::endl;
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

    if (bBinaryOutput)
        outStreamDeposition->open(FileName_Output, std::ios::out | std::ios::binary);
    else
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

void SessionManager::storeMonitorsData()
{
    json11::Json::array Arr;

    for (MonitorSensitiveDetector * mon : Monitors)
    {
        json11::Json::object json;
        mon->writeToJson(json);
        Arr.push_back(json);
    }

    std::ofstream outStream;
    outStream.open(FileName_Monitors);
    if (outStream.is_open())
    {
        std::string json_str = json11::Json(Arr).dump();
        outStream << json_str << std::endl;
    }
    outStream.close();
}
