#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include <string>
#include <vector>
#include <map>

#include "G4ThreeVector.hh"

class G4ParticleDefinition;

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

        bool isGuiMode() const {return bGuiMode;}
        const std::string & getGDML() const {return GDML;}
        const std::string & getEventId() const {return EventId;}
        void updateEventId();
        std::vector<ParticleRecord> & getNextEventPrimaries();
        bool isEndOfInputFileReached() const;
        const std::vector<std::string> & getListOfSensitiveVolumes() const {return SensitiveVolumes;}
        int findParticle(const std::string & particleName);
        int findMaterial(const std::string & materialName);

        int getNumEventsForTrackExport() const {return NumberEventsForTrackExport;}

        void sendLineToDepoOutput(const std::string & text);
        void sendLineToDepoOutput(const std::stringstream & text);

        void sendLineToTracksOutput(const std::string & text);
        void sendLineToTracksOutput(const std::stringstream & text);

    private:        
        void prepareInputStream();
        void prepareOutputDepoStream();
        void prepareOutputTracks();
        void executeAdditionalCommands();

    private:
        std::string FileName_Input;
        std::string FileName_Output;
        std::string FileName_Receipt;
        std::string FileName_Tracks;
        long Seed = 0;
        std::string EventId;
        std::string NextEventId;
        std::string GDML;
        std::vector<G4ParticleDefinition*> ParticleCollection; // does not own
        std::map<std::string, int> ParticleMap;
        std::map<std::string, int> MaterialMap;
        std::vector<std::string> SensitiveVolumes;
        std::vector<std::string> DefinedParticles;
        std::vector<std::string> OnStartCommands;
        std::ifstream * inStreamPrimaries = 0;
        std::ofstream * outStreamDeposition = 0;
        std::ofstream * outStreamTracks = 0;
        std::vector<ParticleRecord> GeneratedPrimaries;
        bool bGuiMode = false;
        int NumberEventsForTrackExport = 0;

};

#endif // SESSIONMANAGER_H
