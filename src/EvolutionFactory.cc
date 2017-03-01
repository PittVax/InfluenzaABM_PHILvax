
#include "EvolutionFactory.h"
#include "Evolution.h"
#include "MSEvolution.h"
#include "Global.h"
#include "Utils.h"

Evolution *EvolutionFactory :: newEvolution(int type) {
    switch (type) {
    case 0:
        return new Evolution;
    case 1:
        return new MSEvolution;
    default:
        PHIL_WARNING("Unknown Evolution type (%d) supplied to EvolutionFactory.  Using the default.", type);
        return new Evolution;
    }
}
