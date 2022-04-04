#include "LoadAssist.h"
#include "Marlin.h"

#define STEP_PULSE_CYCLES_LOAD_ASSIST ((3/*uS*/) * CYCLES_PER_MICROSECOND)
#define NUM_STEPS_LINEAR_ACTUATOR_RETRACT ((32))
#define FILASTICK_RUNOUT_TIME ((32))
#define LOAD_ASSIST_MOVE_TIME_MS ((2000))
#define FilamentTestPin 19

// Singleton
LoadAssist loadAssist;

LoadAssist::LoadAssist() {
    m_runOutTrig = false;
}

bool LoadAssist::is_pulsing() {
    return (m_state == PULSING_EXTENSION) || (m_state == PULSING_RETRACTION);
}

void LoadAssist::start_pulse() {
    if (m_state == PULSING_EXTENSION || m_state == PULSING_RETRACTION) {
        if (m_stepCounter < NUM_STEPS_LINEAR_ACTUATOR_RETRACT) {
            WRITE(LOADING_STEP_PIN, HIGH); // Only start a pulse if we need to
        }
    }
}

void LoadAssist::stop_pulse() {
    if (m_state == PULSING_EXTENSION || m_state == PULSING_RETRACTION) {
        if (m_stepCounter < NUM_STEPS_LINEAR_ACTUATOR_RETRACT) {
            WRITE(LOADING_STEP_PIN, LOW); // Only stop a pulse if we need to
            m_stepCounter++;
            if (m_stepCounter >= NUM_STEPS_LINEAR_ACTUATOR_RETRACT) {
                if (m_state == PULSING_EXTENSION) {
                    m_state = EXTENDING;
                } else if (m_state == PULSING_RETRACTION) {
                    m_state = RETRACTING;
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

    // Configure FilaStick runout pin
    pinMode(FilamentTestPin,INPUT);
    WRITE(FilamentTestPin,HIGH);

    // pulse step to start first move
    SET_OUTPUT(LOADING_STEP_PIN);
    for (size_t i = 0; i < NUM_STEPS_LINEAR_ACTUATOR_RETRACT; i++) {
        do_step();
    }

    m_state = WAITING_TO_RETRACT;
}

void LoadAssist::tick_state_machine() {
    // Check if a new filastick needs to be loaded
    if (m_state == WAITING_TO_RETRACT) {
        bool filastickDetected = !READ(FilamentTestPin);
        if (!m_runOutTrig && !filastickDetected) {
            m_runOutInitialT = millis();
            m_runOutTrig = true;
        }
        if (m_runOutTrig && !filastickDetected) {
            if (millis() - m_runOutInitialT >= FILASTICK_RUNOUT_TIME) {
                SERIAL_ECHOLN("Retracting...");
                loadAssist.setExtend(RETRACT);
                m_moveTime = millis();
                m_runOutTrig = false;
            }
        }
    }

    // If we are retracting the load assist, check if it's okay to turn around
    if (m_state == RETRACTING) {
        if (millis() - m_moveTime > LOAD_ASSIST_MOVE_TIME_MS) {
            loadAssist.setExtend(EXTEND);
            m_moveTime = millis();
            SERIAL_ECHOLN("Extending...");
        }
    }

    // If we are extending the load assist, check if the move is finished
    if (m_state == EXTENDING) {
        if (millis() - m_moveTime > LOAD_ASSIST_MOVE_TIME_MS) {
            m_state = WAITING_TO_RETRACT;
        }
    }

    // Do load assist steps before other steps since they are not part of the planning blocks
    if (loadAssist.is_pulsing()) {
        // If a minimum pulse time was specified get the CPU clock
        #if STEP_PULSE_CYCLES > CYCLES_EATEN_BY_CODE
            static uint32_t pulse_start;
            pulse_start = TCNT0;
        #endif

        loadAssist.start_pulse();

        // For a minimum pulse time wait before stopping pulses
        #if STEP_PULSE_CYCLES > CYCLES_EATEN_BY_CODE
            while ((uint32_t)(TCNT0 - pulse_start) < STEP_PULSE_CYCLES - CYCLES_EATEN_BY_CODE) { /* nada */ }
        #endif

        loadAssist.stop_pulse();
    }
}

void LoadAssist::setExtend(bool extend) {
    if ((extend == RETRACT && m_state == RETRACTED) ||
        (extend == EXTEND && m_state == WAITING_TO_RETRACT) ||
        (m_state == PULSING_EXTENSION) ||
        (m_state == PULSING_RETRACTION))
    {
        return; // Can only start a new move if previous is complete
    }

    m_stepCounter = 0; // reset step counter
    
    if (extend == EXTEND) {
        m_state = PULSING_EXTENSION;
    } else {
        m_state = PULSING_RETRACTION;
    }
}