#pragma once
#include "types.h"

class LoadAssist;
extern LoadAssist loadAssist;

#define RETRACT false // TODO fix this guess
#define EXTEND  true

enum load_assist_state_t {
    WAITING_TO_RETRACT = 0,
    PULSING_RETRACTION = 1,
    RETRACTING = 2,
    RETRACTED = 3,
    PULSING_EXTENSION = 4,
    EXTENDING = 5,
};

/**
 * @brief This file encapsulates a DC load assist driver where:
 *      -Applying +12 V will extend the linear actuator
 *      -Applying -12 V will retract the linear actuator
 * 
 * This is achieved using the E1 stepper driver on board and double stepping
 * With only one of the channels
 */
class LoadAssist
{
private:
    int m_stepCounter;
    bool m_runOutTrig;
    millis_t m_runOutInitialT;
    millis_t m_moveTime;
    load_assist_state_t m_state;

    /**
     * @brief Manually pulse a step in blocking mode
     */
    void do_step();

public:
    LoadAssist();
    
    /**
     * @brief Intializes the Load Assist driver.
     * Call before setting extend.
     */
    void init();

    /**
     * @brief Tick the state machine of the Load Assist. Must be called 1/stepper ISR.
     */
    void tick_state_machine();

    /**
     * @brief Is the Load Assist pulsing the stepper output
     */
    bool is_pulsing();

    /**
     * @brief Starts a pulse (if needed)
     */
    void start_pulse();

    /**
     * @brief Stop a pulse (if needed)
     */
    void stop_pulse();

    /**
     * @brief Set whether the Load Assist driver should extend or retract
     * 
     * @param extend true = extend, false = retract
     */
    void setExtend(bool extend);
};
