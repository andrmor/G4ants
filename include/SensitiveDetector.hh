#ifndef SensitiveDetector_h
#define SensitiveDetector_h

#include "G4VSensitiveDetector.hh"

class G4Step;
class G4HCofThisEvent;

namespace json11 {class Json;}

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

    int ParticleIndex;
    bool bDirect;
    bool bIndirect;
    bool bLower;
    bool bUpper;
    bool bPrimary;
    bool bSecondary;
    bool bStopTracking;

    int     angleBins;
    double  angleFrom;
    double  angleTo;

    int     energyBins;
    double  energyFrom;
    double  energyTo;
    int     energyUnitsInHist;

    int     timeBins;
    double  timeFrom;
    double  timeTo;

    int     xbins;
    int     ybins;
    int     shape;
    double  size1;
    double  size2;
};

#endif // SensitiveDetector_h
