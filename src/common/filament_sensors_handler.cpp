/**
 * @file filament_sensors_handler.cpp
 * @brief this file contains shared code for both MMU and non MMU versions.
 * there are two other files filament_sensors_handler_no_mmu, filament_sensors_handler_mmu
 * I would normally use inheritance, but last time i did that it was rewritten, so I am using this approach now.
 */

#include "filament_sensors_handler.hpp"
#include "print_processor.hpp"
#include "rtos_api.hpp"
#include <common/freertos_mutex.hpp>
#include "bsod.h"
#include <mutex>
#include <log.h>
#include "fsensor_eeprom.hpp"
#include <option/has_selftest_snake.h>
#include <option/has_mmu2.h>
#include <option/has_human_interactions.h>
#if HAS_SELFTEST_SNAKE()
    #include <ScreenHandler.hpp>
    #include "screen_menu_selftest_snake.hpp"
#endif

LOG_COMPONENT_DEF(FSensor, LOG_SEVERITY_INFO);

using namespace MMU2;

FilamentSensors::FilamentSensors() {

    SetToolIndex(); // other config depends on it, do it first

    if (has_mmu2_enabled()) {
        request_side = filament_sensor::cmd_t::on;
    }

    // Set logical sensors
    // MK4 can be reconfigured (connecting MMU)
    // MINI can not
    // XL reconfigures on tool change
    configure_sensors();
}

freertos::Mutex &FilamentSensors::GetSideMutex() {
    static freertos::Mutex ret;
    return ret;
}
freertos::Mutex &FilamentSensors::GetExtruderMutex() {
    static freertos::Mutex ret;
    return ret;
}

// Store request_printer off
void FilamentSensors::Disable() {
    DisableSideSensor(); // MMU requires enabled filament sensor to work, it makes sense for XL to behave the same
    const std::lock_guard lock(GetExtruderMutex());
    request_printer = filament_sensor::cmd_t::off;
}

// Store request_printer on
void FilamentSensors::Enable() {
    const std::lock_guard lock(GetExtruderMutex());
    request_printer = filament_sensor::cmd_t::on;
}

bool FilamentSensors::is_enabled() const {
    return FSensorEEPROM::Get();
}

FilamentSensors::SensorStateBitset FilamentSensors::get_sensors_states() {
    SensorStateBitset result;

    // If we're processing an enable command, consider the sensors not initialized
    if (filament_sensor::cmd_t request = request_printer.load(); request == filament_sensor::cmd_t::on || request == filament_sensor::cmd_t::processing) {
        result.set(ftrstd::to_underlying(FilamentSensorState::NotInitialized));
    }

    // Check state of all sensors
    for_all_sensors([&](IFSensor &s) {
        result.set(ftrstd::to_underlying(s.get_state()));
    });

    return result;
}

void FilamentSensors::for_all_sensors(const std::function<void(IFSensor &)> &f) {
    HOTEND_LOOP() {
        if (IFSensor *s = GetExtruderFSensor(e)) {
            f(*s);
        }
        if (IFSensor *s = GetSideFSensor(e)) {
            f(*s);
        }
    }
}

// process printer request stored by Enable/Disable
void FilamentSensors::process_printer_request() {
    switch (request_printer) {

    case filament_sensor::cmd_t::on:
        for_all_sensors([&](IFSensor &s) {
            s.Enable();
        });
        request_printer = filament_sensor::cmd_t::processing;
        break;

    case filament_sensor::cmd_t::off:
        for_all_sensors([&](IFSensor &s) {
            s.Disable();
        });
        request_printer = filament_sensor::cmd_t::processing;
        break;

    case filament_sensor::cmd_t::processing:
    case filament_sensor::cmd_t::null:
        break;
    }
}

void FilamentSensors::all_sensors_initialized() {
    // Need lock to avoid loss of command if it is set just after evaluation but before assignment
    const std::lock_guard lock(GetExtruderMutex());
    if (request_printer == filament_sensor::cmd_t::processing) {
        request_printer = filament_sensor::cmd_t::null; // cycle ended clear command, it is atomic so it will not be reordered
    }
}
bool FilamentSensors::run_sensors_cycle() {
    bool any_not_intitialized = false;

    for_all_sensors([&](IFSensor &s) {
        s.Cycle();
        any_not_intitialized |= (s.get_state() == FilamentSensorState::NotInitialized);
    });

    return any_not_intitialized;
}

filament_sensor::Events FilamentSensors::evaluate_logical_sensors_events() {
    auto arr_logical = logical_sensors.get_array();

    // Get events for each logical sensor.
    filament_sensor::Events ret;
    for (size_t logi = 0; logi < arr_logical.size(); ++logi) {
        if (arr_logical[logi]) { // check if current sensor is linked
            ret.get(logi) = arr_logical[logi]->GenerateEvent();

            // Since sensor can repeat inside arr_logical, update every consecutive logical sensor and set it to null so GenerateEvent is not called twice
            for (size_t next_logi = logi + 1; next_logi < arr_logical.size(); ++next_logi) {
                if (arr_logical[logi] == arr_logical[next_logi]) {
                    ret.get(next_logi) = ret.get(logi);
                    arr_logical[next_logi] = nullptr;
                }
            }
        }
    }

    return ret;
}

void FilamentSensors::Cycle() {
    process_side_request();
    process_printer_request();

    // run cycle to evaluate state of all sensors (even those not active)
    bool any_not_intitialized = run_sensors_cycle();

    if (!any_not_intitialized) {
        all_sensors_initialized();
    }

    // Evaluate currently used sensors of all sensors
    filament_sensor::Events events = FilamentSensors::evaluate_logical_sensors_events();

    set_corresponding_variables();

    if (isEvLocked()) {
        return;
    }

    enum class Action {
        none,
        filament_change,
        autoload,
    };
    Action action = Action::none;

    if (PrintProcessor::IsPrinting()) {
        if (evaluateM600(events.primary_runout)) {
            action = Action::filament_change;
        }

        // With an MMU, don't check for runout on the secondary sensor
        if (!has_mmu && evaluateM600(events.secondary_runout)) {
            action = Action::filament_change;
        }

    } else {
        if (evaluateAutoload(events.autoload)) {
            action = Action::autoload;
        }
    }

    switch (action) {

    case Action::none:
        break;

    case Action::filament_change: {
        // gcode is injected outside critical section, so critical section is as short as possible
        // also injection of GCode inside critical section might not work
        m600_sent = true;

        if constexpr (option::has_human_interactions) {
            PrintProcessor::InjectGcode("M600 A"); // change filament
        } else {
            PrintProcessor::InjectGcode("M25 U"); // pause and unload filament
        }

        log_info(FSensor, "Injected runout");
        break;
    }

    case Action::autoload: {
        autoload_sent = true;
        PrintProcessor::InjectGcode("M1701 Z40"); // autoload with return option and minimal Z value of 40mm
        log_info(FSensor, "Injected autoload");
        break;
    }
    }
}

/**
 * @brief method to store corresponding values inside critical section
 */
void FilamentSensors::set_corresponding_variables() {
    // need locks, to ensure all variables have corresponding values
    // don't actually take the locks yet
    std::unique_lock lock_mmu(GetSideMutex(), std::defer_lock);
    std::unique_lock lock_printer(GetExtruderMutex(), std::defer_lock);

    // lock both unique_locks without deadlock
    buddy::lock(lock_mmu, lock_printer);

    reconfigure_sensors_if_needed();

    // copy states of sensors while we are in critical section
    state_of_primary_runout_sensor = logical_sensors.primary_runout ? logical_sensors.primary_runout->get_state() : FilamentSensorState::Disabled;
    state_of_secondary_runout_sensor = logical_sensors.secondary_runout ? logical_sensors.secondary_runout->get_state() : FilamentSensorState::Disabled;
    state_of_autoload_sensor = logical_sensors.autoload ? logical_sensors.autoload->get_state() : FilamentSensorState::Disabled;

    state_of_current_extruder = logical_sensors.current_extruder ? logical_sensors.current_extruder->get_state() : FilamentSensorState::Disabled;
    state_of_current_side = logical_sensors.current_side ? logical_sensors.current_side->get_state() : FilamentSensorState::Disabled;
}

FilamentSensors::BothSensors FilamentSensors::GetBothSensors() {
    // need locks, to ensure all variables have corresponding values
    // don't actually take the locks yet
    std::unique_lock lock_mmu(GetSideMutex(), std::defer_lock);
    std::unique_lock lock_printer(GetExtruderMutex(), std::defer_lock);

    // lock both unique_locks without deadlock
    buddy::lock(lock_mmu, lock_printer);

    BothSensors ret = { state_of_current_extruder, state_of_current_side };

    return ret;
}

// this method is currently called outside FilamentSensors::Cycle critical section, so the critical section is shorter
// trying to trigger runout at exact moment when print ended could break something
// also if another M600 happens during clear of M600_sent flag, it could be discarded, this is not a problem, because it could happen only due a bug
// if it happens move it inside FilamentSensors::Cycle critical section
bool FilamentSensors::evaluateM600(std::optional<IFSensor::Event> ev) const {
    return ev && ev == IFSensor::Event::filament_removed && !m600_sent;
}

// this method is currently called outside FilamentSensors::Cycle critical section, so the critical section is shorter
// trying to trigger autoload at exact moment when print starts could break something
// also if another autoload happens during clear of Autoload_sent flag, it could be discarded, this is not a problem, because it could happen only due a bug
// if it happens move it inside FilamentSensors::Cycle critical section
bool FilamentSensors::evaluateAutoload(std::optional<IFSensor::Event> ev) const {
    return //
        ev && ev == IFSensor::Event::filament_inserted
        && !has_mmu
        && !autoload_sent
        && !isAutoloadLocked()
        && PrintProcessor::IsAutoloadEnabled() //
#if HAS_SELFTEST_SNAKE()
        && !Screens::Access()->IsScreenOnStack<ScreenMenuSTSWizard>()
        && !Screens::Access()->IsScreenOnStack<ScreenMenuSTSCalibrations>()
#endif /*PRINTER_IS_PRUSA_XL*/
            ;
}

uint32_t FilamentSensors::DecEvLock() {
    CriticalSection C; // TODO use counting semaphore to avoid critical section
    if (event_lock > 0) {
        --event_lock;
    } else {
        bsod("Filament sensor event lock out of range");
    }
    return event_lock;
}

uint32_t FilamentSensors::DecAutoloadLock() {
    CriticalSection C; // TODO use counting semaphore to avoid critical section
    if (autoload_lock > 0) {
        --autoload_lock;
    } else {
        bsod("Autoload event lock out of range");
    }
    return autoload_lock;
}

uint32_t FilamentSensors::IncAutoloadLock() {
    return ++autoload_lock;
}
uint32_t FilamentSensors::IncEvLock() {
    return ++event_lock;
}

bool FilamentSensors::MMUReadyToPrint() {
    // don't actually take the locks yet
    std::unique_lock lock_mmu(GetSideMutex(), std::defer_lock);
    std::unique_lock lock_printer(GetExtruderMutex(), std::defer_lock);

    // lock both unique_locks without deadlock
    buddy::lock(lock_mmu, lock_printer);

    // filament has to be unloaded from primary tool for MMU print
    return state_of_primary_runout_sensor == FilamentSensorState::NoFilament;
}

bool FilamentSensors::ToolHasFilament(uint8_t tool_nr) {
    // don't actually take the locks yet
    std::unique_lock lock_side(GetSideMutex(), std::defer_lock);
    std::unique_lock lock_printer(GetExtruderMutex(), std::defer_lock);

    // lock both unique_locks without deadlock
    buddy::lock(lock_side, lock_printer);

    FilamentSensorState extruder_state = GetExtruderFSensor(tool_nr) ? GetExtruderFSensor(tool_nr)->get_state() : FilamentSensorState::Disabled;
    FilamentSensorState side_state = GetSideFSensor(tool_nr) ? GetSideFSensor(tool_nr)->get_state() : FilamentSensorState::Disabled;

    return (extruder_state == FilamentSensorState::HasFilament || extruder_state == FilamentSensorState::Disabled) && (side_state == FilamentSensorState::HasFilament || side_state == FilamentSensorState::Disabled);
}

IFSensor *get_active_printer_sensor() {
    return GetExtruderFSensor(FSensors_instance().tool_index);
}

IFSensor *get_active_side_sensor() {
    return GetSideFSensor(FSensors_instance().tool_index);
}

/*****************************************************************************/
// section with locks
// Do not nest calls of methods with same mutex !!!
// Do not call from filament sensor thread

/**
 * @brief encode printer sensor state to MMU enum
 * TODO distinguish between at fsensor and in nozzle
 * currently only AT_FSENSOR returned
 * @return MMU2::FilamentState
 */
FilamentState FilamentSensors::WhereIsFilament() {
    const std::lock_guard lock(GetExtruderMutex());
    switch (state_of_secondary_runout_sensor) {
    case FilamentSensorState::HasFilament:
        return FilamentState::AT_FSENSOR;
    case FilamentSensorState::NoFilament:
        return FilamentState::NOT_PRESENT;
    case FilamentSensorState::NotInitialized:
    case FilamentSensorState::NotCalibrated:
    case FilamentSensorState::NotConnected:
    case FilamentSensorState::Disabled:
        break;
    }
    return FilamentState::UNAVAILABLE;
}

// this method should not be accessed if we don't have MMU
// if it is it will do unnecessary lock and return false
// I prefer to have single method for both variants
bool FilamentSensors::HasMMU() {
    const std::lock_guard lock(GetSideMutex());
    return has_mmu;
}

// end of section with locks
/*****************************************************************************/

// Meyer's singleton
FilamentSensors &FSensors_instance() {
    static FilamentSensors ret;
    return ret;
}
