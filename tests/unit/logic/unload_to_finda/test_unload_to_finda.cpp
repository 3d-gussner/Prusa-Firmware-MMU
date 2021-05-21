#include "catch2/catch.hpp"

#include "../../../../src/modules/buttons.h"
#include "../../../../src/modules/finda.h"
#include "../../../../src/modules/fsensor.h"
#include "../../../../src/modules/globals.h"
#include "../../../../src/modules/idler.h"
#include "../../../../src/modules/leds.h"
#include "../../../../src/modules/motion.h"
#include "../../../../src/modules/permanent_storage.h"
#include "../../../../src/modules/selector.h"

#include "../../../../src/logic/unload_to_finda.h"

#include "../../modules/stubs/stub_adc.h"

#include "../stubs/main_loop_stub.h"
#include "../stubs/stub_motion.h"

using Catch::Matchers::Equals;

namespace mm = modules::motion;
namespace mf = modules::finda;
namespace mi = modules::idler;
namespace ml = modules::leds;
namespace mb = modules::buttons;
namespace mg = modules::globals;
namespace ms = modules::selector;

namespace ha = hal::adc;

template <typename COND>
bool WhileConditionFF(logic::UnloadToFinda &ff, COND cond, uint32_t maxLoops = 5000) {
    while (cond() && --maxLoops) {
        main_loop();
        ff.Step();
    }
    return maxLoops > 0;
}

TEST_CASE("unload_to_finda::regular_unload", "[unload_to_finda]") {
    using namespace logic;

    ForceReinitAllAutomata();

    // we need finda ON
    hal::adc::ReinitADC(1, hal::adc::TADCData({ 1023 }), 1);

    UnloadToFinda ff;

    // wait for FINDA to debounce
    REQUIRE(WhileCondition(
        [&]() { return !mf::finda.Pressed(); },
        5000));

    // restart the automaton - just 1 attempt
    ff.Reset(1);

    REQUIRE(ff.State() == UnloadToFinda::EngagingIdler);

    // it should have instructed the selector and idler to move to slot 1
    // check if the idler and selector have the right command
    CHECK(mm::axes[mm::Idler].targetPos == mi::Idler::SlotPosition(0));
    CHECK(mm::axes[mm::Selector].targetPos == ms::Selector::SlotPosition(0));
    CHECK(mm::axes[mm::Idler].enabled == true);

    // engaging idler
    REQUIRE(WhileConditionFF(
        ff,
        [&]() { return !mi::idler.Engaged(); },
        5000));

    // now pulling the filament until finda triggers
    REQUIRE(ff.State() == UnloadToFinda::WaitingForFINDA);
    hal::adc::ReinitADC(1, hal::adc::TADCData({ 1023, 900, 800, 500, 0 }), 10);
    REQUIRE(WhileConditionFF(
        ff,
        [&]() { return mf::finda.Pressed(); },
        50000));

    REQUIRE(ff.State() == UnloadToFinda::OK);
}

TEST_CASE("unload_to_finda::no_sense_FINDA_upon_start", "[unload_to_finda]") {
    using namespace logic;

    ForceReinitAllAutomata(); // that implies FINDA OFF which should really not happen for an unload call

    UnloadToFinda ff;

    // restart the automaton - just 1 attempt
    ff.Reset(1);

    // the state machine should accept the unpressed FINDA as no-fillament-loaded
    // thus should immediately end in the OK state
    REQUIRE(ff.State() == UnloadToFinda::OK);
}

TEST_CASE("unload_to_finda::unload_without_FINDA_trigger", "[unload_to_finda]") {
    using namespace logic;

    ForceReinitAllAutomata();

    // we need finda ON
    hal::adc::ReinitADC(1, hal::adc::TADCData({ 1023 }), 1);

    UnloadToFinda ff;

    // wait for FINDA to debounce
    REQUIRE(WhileCondition(
        [&]() { return !mf::finda.Pressed(); },
        5000));

    // restart the automaton - just 1 attempt
    ff.Reset(1);

    REQUIRE(ff.State() == UnloadToFinda::EngagingIdler);

    // it should have instructed the selector and idler to move to slot 1
    // check if the idler and selector have the right command
    CHECK(mm::axes[mm::Idler].targetPos == mi::Idler::SlotPosition(0));
    CHECK(mm::axes[mm::Selector].targetPos == ms::Selector::SlotPosition(0));
    CHECK(mm::axes[mm::Idler].enabled == true);

    // engaging idler
    REQUIRE(WhileConditionFF(
        ff,
        [&]() { return !mi::idler.Engaged(); },
        5000));

    // now pulling the filament until finda triggers
    REQUIRE(ff.State() == UnloadToFinda::WaitingForFINDA);

    // no changes to FINDA during unload - we'll pretend it never triggers
    REQUIRE(!WhileConditionFF(
        ff,
        [&]() { return mf::finda.Pressed(); },
        50000));

    REQUIRE(ff.State() == UnloadToFinda::Failed);
}