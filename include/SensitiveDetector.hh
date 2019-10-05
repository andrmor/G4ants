#ifndef SensitiveDetector_h
#define SensitiveDetector_h

#include "G4VSensitiveDetector.hh"

class G4Step;
class G4HCofThisEvent;

//namespace json11 {class Json;}
#include "json11.hh"

class SensitiveDetector : public G4VSensitiveDetector
{
public:
    SensitiveDetector(const G4String & name);
    virtual ~SensitiveDetector();

    virtual G4bool ProcessHits(G4Step* step, G4TouchableHistory* history);
};

class MonitorSensitiveDetector : public G4VSensitiveDetector
{
public:
    MonitorSensitiveDetector(const G4String & name);
    virtual ~MonitorSensitiveDetector();

    virtual G4bool ProcessHits(G4Step* step, G4TouchableHistory* history);

    void readFromJson(const json11::Json & json);
    void writeToJson(json11::Json::object & json);

    std::string Name;
    std::string ParticleName;

    G4ParticleDefinition * pParticleDefinition = nullptr;

    bool bLower;
    bool bUpper;
    bool bStopTracking;
    bool bDirect;
    bool bIndirect;
    bool bPrimary;
    bool bSecondary;

    int     angleBins;
    double  angleFrom;
    double  angleTo;

    int     energyBins;
    double  energyFrom;
    double  energyTo;
    int     energyUnits; // 0,1,2,3 -> meV, eV, keV, MeV;

    int     timeBins;
    double  timeFrom;
    double  timeTo;

    int     xbins;
    int     ybins;
    double  size1;
    double  size2;

    std::vector<double> vTime;
    double timeDelta = 1.0;

    std::vector<double> vAngle;
    double angleDelta = 1.0;

    std::vector<double> vEnergy;
    double energyDelta = 1.0;

    std::vector<std::vector<double>> vSpatial; //[y][x]
    double xDelta = 1.0;
    double yDelta = 1.0;
};

#endif // SensitiveDetector_h
