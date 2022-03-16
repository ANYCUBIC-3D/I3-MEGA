#include "LoadAssist.h"
#include "Marlin.h"

#define STEP_PULSE_CYCLES_LOAD_ASSIST ((3/*uS*/) * CYCLES_PER_MICROSECOND)
#define NUM_STEPS_LINEAR_ACTUATOR_RETRACT ((32))

// Singleton
LoadAssist loadAssist;

LoadAssist::LoadAssist() {
    return; // Nothing to do here... for now
}

bool LoadAssist::is_moving() {
    return (m_state == EXTENDING) || (m_state == RETRACTING);
}

void LoadAssist::start_pulse() {
    if (m_state == EXTENDING || m_state == RETRACTING) {
        if (m_step_counter < NUM_STEPS_LINEAR_ACTUATOR_RETRACT) {
            WRITE(LOADING_STEP_PIN, HIGH); // Only start a pulse if we need to
        }
    }
}

void LoadAssist::stop_pulse() {
    if (m_state == EXTENDING || m_state == RETRACTING) {
        if (m_step_counter < NUM_STEPS_LINEAR_ACTUATOR_RETRACT) {
            WRITE(LOADING_STEP_PIN, LOW); // Only stop a pulse if we need to
            m_step_counter++;
            if (m_step_counter >= NUM_STEPS_LINEAR_ACTUATOR_RETRACT) {
                if (m_state == EXTENDING) {
                    m_state = EXTENDED;
                } else if (m_state == RETRACTING) {
                    m_state = RETRACTED;
                }
            }
        }
    }
}

void LoadAssist::do_step() {
    WRITE(LOADING_STEP_PIN, HIGH); // Active high
    
    // Wait for minimum step pulse cycles
    uint32_t pulse_start = TCNT0;
    while ((uint32_t) (TCNT0 - pulse_start) < STEP_PULSE_CYCLES_LOAD_ASSIST);

    WRITE(LOADING_STEP_PIN, LOW);

    // Wait for minimum step pulse cycles
    pulse_start = TCNT0;
    while ((uint32_t) (TCNT0 - pulse_start) < STEP_PULSE_CYCLES_LOAD_ASSIST);
}

void LoadAssist::init() {
    // Default to retracting for loading
    SET_OUTPUT(LOADING_ENABLE_PIN);
    WRITE(LOADING_ENABLE_PIN, HIGH); // Start disabled low
    delay(20);
    WRITE(LOADING_ENABLE_PIN, LOW); // Active low

    // Default to retracting for loading
    SET_OUTPUT(LOADING_DIR_PIN);
    WRITE(LOADING_DIR_PIN, LOW);

    // pulse step to start first move
    SET_OUTPUT(LOADING_STEP_PIN);
    m_state = RETRACTED;
}

void LoadAssist::setExtend(bool extend) {
    if ((extend == RETRACT && m_state == RETRACTED) ||
        (extend == EXTEND && m_state == EXTENDED) ||
        (m_state == EXTENDING) ||
        (m_state == RETRACTING))
    {
        return; // Can only start a new move if previous is complete
    }

    m_step_counter = 0; // reset step counter
    
    if (extend == EXTEND) {
        m_state = EXTENDING;
    } else {
        m_state = RETRACTING;
    }
}