#pragma once

class LoadAssist;
extern LoadAssist loadAssist;

#define RETRACT false // TODO fix this guess
#define EXTEND  true

enum load_assist_state_t {
    RETRACTED = 0,
    RETRACTING = 1,
    EXTENDED = 2,
    EXTENDING = 3,
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
    int m_step_counter;
    load_assist_state_t m_state;

    void do_step();
public:
    LoadAssist();
    
    /**
     * @brief Intializes the Load Assist driver.
     * Call before setting extend.
     */
    void init();

    /**
     * @brief Is the Load Assist currently mid motion
     */
    bool is_moving();

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
