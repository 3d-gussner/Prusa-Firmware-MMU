#include "movable_base.h"
#include "globals.h"
#include "motion.h"

namespace modules {
namespace motion {

void MovableBase::PlanHome(config::Axis axis) {
    // switch to normal mode on this axis
    state = Homing;
    mm::motion.InitAxis(axis);
    mm::motion.SetMode(axis, mm::Normal);
    mm::motion.StallGuardReset(axis);

    // plan move at least as long as the axis can go from one side to the other
    PlanHomingMove(); // mm::motion.PlanMove(axis, delta, 1000);
}

MovableBase::OperationResult MovableBase::InitMovement(config::Axis axis) {
    if (motion.InitAxis(axis)) {
        PrepareMoveToPlannedSlot();
        state = Moving;
        return OperationResult::Accepted;
    } else {
        state = Failed;
        return OperationResult::Failed;
    }
}

void MovableBase::PerformMove(config::Axis axis) {
    if (!mm::motion.DriverForAxis(axis).GetErrorFlags().Good()) { // @@TODO check occasionally, i.e. not every time?
        // TMC2130 entered some error state, the planned move couldn't have been finished - result of operation is Failed
        tmcErrorFlags = mm::motion.DriverForAxis(axis).GetErrorFlags(); // save the failed state
        state = Failed;
    } else if (mm::motion.QueueEmpty(axis)) {
        // move finished
        state = Ready;
    }
}

void MovableBase::PerformHome(config::Axis axis) {
    if (mm::motion.StallGuard(axis)) {
        // we have reached the end of the axis - homed ok
        mm::motion.SetMode(axis, mg::globals.MotorsStealth() ? mm::Stealth : mm::Normal);
        state = Ready;
    } else if (mm::motion.QueueEmpty(axis)) {
        // we ran out of planned moves but no StallGuard event has occurred - homing failed
        mm::motion.SetMode(axis, mg::globals.MotorsStealth() ? mm::Stealth : mm::Normal);
        state = Failed;
    }
}

} // namespace motion
} // namespace modules
