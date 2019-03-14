#include "SessionManager.hh"
#include "json11.hh" //https://github.com/dropbox/json11

#include <iostream>
#include <sstream>
#include <fstream>

#include "G4ParticleDefinition.hh"
#include "G4ParticleTable.hh"
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
    delete inStreamPrimaries;
}

void SessionManager::startSession(const std::string & ConfigFileName)
{
    // read json configuration file
    readConfig(ConfigFileName);

    // opening file with primaries
    prepareInputStream();

    // preparing ouptut for deposition data
    prepareOutputStream();

    //set random generator. The seed was provided in the config file
    CLHEP::RanecuEngine* randGen = new CLHEP::RanecuEngine();
    randGen->setSeed(Seed);
    G4Random::setTheEngine(randGen);

    executeAdditionalCommands();
}

void SessionManager::terminateSession(const std::string & ReturnMessage)
{
    std::cout << "Terminating session with the message:\n"<<ReturnMessage<<std::endl;

    std::ofstream outStream;
    outStream.open(FileName_Receipt);
    if (outStream.is_open())
        outStream << ReturnMessage << std::endl;

    exit(0);
}

void SessionManager::runSimulation()
{
    G4UImanager* UImanager = G4UImanager::GetUIpointer();

    while (!isEndOfInputFileReached())
        UImanager->ApplyCommand("/run/beamOn");
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
        std::cout << line << std::endl;
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

void SessionManager::sendLineToOutput(const std::string & text)
{
    if (!outStreamDeposition) return;

    *outStreamDeposition << text.data() << std::endl;
}

void SessionManager::sendLineToOutput(const std::stringstream & text)
{
    if (!outStreamDeposition) return;

    *outStreamDeposition << text.rdbuf() << std::endl;
}

void SessionManager::readConfig(const std::string &ConfigFileName)
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

    //extracting name of the file with primaries to generate
    FileName_Input = jo["File_Primaries"].string_value();
    if (FileName_Input.empty())
        terminateSession("File name with primaries to generate was not provided");

    //extracting name of the file for deposition output
    FileName_Output = jo["File_Deposition"].string_value();
    if (FileName_Output.empty())
        terminateSession("File name for deposition output was not provided");

    //extracting name of the receipt file
    FileName_Receipt = jo["File_Receipt"].string_value();
    if (FileName_Output.empty())
        terminateSession("File name for receipt was not provided");

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

    //extracting defined particles
    ParticleCollection.clear();
    std::vector<json11::Json> arr = jo["Particles"].array_items();
    std::cout << "Number of defined particles: " << arr.size() << std::endl;
    if (arr.empty())
        terminateSession("No particles defined in the configuration file!");

    //populating particle collection
    for (auto & j : arr)
    {
        std::string name = j.string_value();
        std::cout << name << std::endl;
        G4ParticleDefinition * pParticleDefinition = G4ParticleTable::GetParticleTable()->FindParticle(name);
        if (!pParticleDefinition)
            terminateSession("Particle not found in Geant4 particle list!");
        ParticleCollection.push_back(pParticleDefinition);
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

void SessionManager::prepareOutputStream()
{
    outStreamDeposition = new std::ofstream();
    outStreamDeposition->open(FileName_Output);
    if (!outStreamDeposition->is_open())
        terminateSession("Cannot open file to store deposition data");
}

void SessionManager::executeAdditionalCommands()
{
    G4UImanager* UImanager = G4UImanager::GetUIpointer();
    for (auto & cmd : OnStartCommands)
        UImanager->ApplyCommand(cmd);
}
