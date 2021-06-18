#include "main_loop_stub.h"

#include "../../modules/stubs/stub_adc.h"

#include "../../../../src/modules/buttons.h"
#include "../../../../src/modules/finda.h"
#include "../../../../src/modules/fsensor.h"
#include "../../../../src/modules/globals.h"
#include "../../../../src/modules/idler.h"
#include "../../../../src/modules/leds.h"
#include "../../../../src/modules/motion.h"
#include "../../../../src/modules/permanent_storage.h"
#include "../../../../src/modules/selector.h"

#include <new> // bring in placement new

logic::CommandBase *currentCommand = nullptr;

// just like in the real FW, step all the known automata
uint16_t millis = 0;

void main_loop() {
    modules::buttons::buttons.Step(millis);
    modules::leds::leds.Step(millis);
    modules::finda::finda.Step(millis);
    modules::fsensor::fsensor.Step(millis);
    modules::idler::idler.Step();
    modules::selector::selector.Step();
    modules::motion::motion.Step();
    if (currentCommand)
        currentCommand->Step();
    ++millis;
}

void ForceReinitAllAutomata() {
    // This woodoo magic with placement new is just a forced reinit of global instances of firmware's state machines
    // just for the purposes of separate unit tests. Each unit test needs a "freshly booted firmware" and since all unit tests
    // in the test binary share the same global data structures, we need some way of making them fresh each time.
    //
    // This approach mimics the firmware behavior closely as the firmware initializes its global data structures
    // on its very start once (by copying static init data from PROGMEM into RAM) - and we need exactly this approach in the unit tests.
    //
    // There are multiple other approaches, one of them is adding a special Init() function into each of these state machines.
    // As this approach might look like a standard and safer way of doing stuff, it has several drawbacks, especially
    // it needs an explicit call to the Init function every time an object like this is created - this can have negative influence on firmware's code size

    new (&modules::buttons::buttons) modules::buttons::Buttons();
    new (&modules::leds::leds) modules::leds::LEDs();
    new (&modules::finda::finda) modules::finda::FINDA();
    new (&modules::fsensor::fsensor) modules::fsensor::FSensor();
    new (&modules::idler::idler) modules::idler::Idler();
    new (&modules::selector::selector) modules::selector::Selector();
    new (&modules::motion::motion) modules::motion::Motion();

    // no buttons involved ;)
    hal::adc::ReinitADC(0, hal::adc::TADCData({ 0 }), 1);

    // finda OFF
    hal::adc::ReinitADC(1, hal::adc::TADCData({ 0 }), 1);

    // let's assume we have the filament NOT loaded and active slot 0
    modules::globals::globals.SetFilamentLoaded(false);
    modules::globals::globals.SetActiveSlot(0);
}