#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include "json11.hh" //https://github.com/dropbox/json11

#include <string>
#include <vector>
#include <unordered_set>
#include <map>

#include "G4ThreeVector.hh"

class G4ParticleDefinition;
class G4StepPoint;
class MonitorSensitiveDetector;

struct ParticleRecord
{
    G4ParticleDefinition * Particle = 0;
    G4double Energy = 0;
    G4ThreeVector Position  = {0, 0, 0};
    G4ThreeVector Direction = {0, 0, 0};
    G4double Time = 0;
};

class SessionManager
{
    public:
        static SessionManager& getInstance();

    private:
        SessionManager();
        ~SessionManager();

    public:
        SessionManager(SessionManager const&) = delete;
        void operator=(SessionManager const&) = delete;

        void ReadConfig(const std::string & ConfigFileName);

        void startSession();
        void terminateSession(const std::string & ReturnMessage); //calls exit()!
        void endSession();

        void runSimulation();

        void onRunFinished();
        bool isGuiMode() const {return bGuiMode;}
        const std::string & getGDML() const {return GDML;}
        const std::string & getPhysicsList() const {return PhysicsList;}
        const std::string & getEventId() const {return EventId;}
        void updateEventId();
        std::vector<ParticleRecord> & getNextEventPrimaries();
        bool isEndOfInputFileReached() const;
        const std::vector<std::string> & getListOfSensitiveVolumes() const {return SensitiveVolumes;}
        std::vector<MonitorSensitiveDetector*> & getMonitors() {return Monitors;}
        const std::map<std::string, double> & getStepLimitMap() const {return StepLimitMap;}
        int findParticle(const std::string & particleName);  // change to pointer search?
        int findMaterial(const std::string & materialName);  // change to pointer search?

        void sendLineToDepoOutput(const std::string & text);
        void sendLineToDepoOutput(const std::stringstream & text);

        void sendLineToTracksOutput(const std::string & text);
        void sendLineToTracksOutput(const std::stringstream & text);

        void resetPredictedTrackID() {NextTrackID = 1;}
        void incrementPredictedTrackID() {NextTrackID++;}
        int  getPredictedTrackID() {return NextTrackID;}

public:
        //runtime
        double DepoByRegistered = 0;
        double DepoByNotRegistered = 0;

        enum HistoryMode {NotCollecting, OnlyTracks, FullLog};
        HistoryMode CollectHistory = NotCollecting;
        int TracksToBuild = 0;

        int Precision    = 6;

        bool bStoppedOnMonitor = false; // bug fix for Geant4? used in (Monitor)SensitiveDetector and SteppingAction

    private:
        void prepareParticleCollection();
        void prepareMonitors();
        void prepareInputStream();
        void prepareOutputDepoStream();
        void prepareOutputTracks();
        void executeAdditionalCommands();
        void generateReceipt();
        void storeMonitorsData();

    private:
        std::string FileName_Input;
        std::string FileName_Output;
        std::string FileName_Monitors;
        std::string FileName_Receipt;
        std::string FileName_Tracks;
        long Seed = 0;
        std::string EventId;
        std::string NextEventId;
        std::string GDML;
        std::string PhysicsList;
        std::vector<json11::Json> ParticleJsonArray;
        std::vector<G4ParticleDefinition*> ParticleCollection; // does not own
        std::map<std::string, int> ParticleMap;
        std::map<std::string, int> MaterialMap;
        std::vector<std::string> SensitiveVolumes;
        std::vector<std::string> OnStartCommands;
        std::map<std::string, double> StepLimitMap;
        std::ifstream * inStreamPrimaries = 0;
        std::ofstream * outStreamDeposition = 0;
        std::ofstream * outStreamTracks = 0;
        std::vector<ParticleRecord> GeneratedPrimaries;
        bool bGuiMode = false;

        std::vector<MonitorSensitiveDetector*> Monitors; //can contain nullptr!

        int EventsDone = 0;
        int NumEventsToDo = 0;
        double ProgressLastReported = 0;
        double ProgressInc = 1.0;

        int NextTrackID = 1;

        std::unordered_set<std::string> SeenNotRegisteredParticles;

        //to report back to ants2
        bool bError;
        std::string ErrorMessage;
        std::vector<std::string> WarningMessages;
};

#endif // SESSIONMANAGER_H
