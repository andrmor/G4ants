#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include <string>
#include <vector>

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

        void runSimulation();

        const std::string & getEventId() const {return EventId;}
        void updateEventId();
        std::vector<ParticleRecord> & getNextEventPrimaries();
        bool isEndOfInputFileReached() const;
        const std::vector<std::string> & getListOfSensitiveVolumes() const {return SensitiveVolumes;}
        void sendLineToOutput(const std::string & text);
        void sendLineToOutput(const std::stringstream & text);

    private:        
        void prepareInputStream();
        void prepareOutputStream();
        void executeAdditionalCommands();

    private:
        std::string FileName_Input;
        std::string FileName_Output;
        std::string FileName_Receipt;
        long Seed = 0;
        std::string EventId;
        std::string NextEventId;
        std::vector<G4ParticleDefinition*> ParticleCollection; // does not own
        std::vector<std::string> SensitiveVolumes;
        std::vector<std::string> DefinedParticles;
        std::vector<std::string> OnStartCommands;
        std::ifstream * inStreamPrimaries = 0;
        std::ofstream * outStreamDeposition = 0;
        std::vector<ParticleRecord> GeneratedPrimaries;

};

#endif // SESSIONMANAGER_H
