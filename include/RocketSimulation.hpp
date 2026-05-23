#pragma once

#include "RocketTarget.hpp"
#include "RadarConfig.hpp"

#include <vector>

// =============================================================================
//  RocketSimulation — replaces the old Simulation class
//  Manages rockets, radar sweep, and rocket selection.
// =============================================================================
class RocketSimulation {
public:
    explicit RocketSimulation(const RadarConfig& config);

    void update(float deltaSeconds);

    // ---- Accessors ----------------------------------------------------------
    const std::vector<RocketTarget>& rockets()          const;
    std::vector<RocketTarget>&       rockets();
    float                            scanAngleDegrees() const;
    const RadarConfig&               config()           const;
    float                            missionElapsed()   const;

    // ---- Rocket selection (replaces lock-on) --------------------------------
    int  selectedRocketIndex() const;
    void cycleSelection();        // Tab
    void launchSelected();        // Space — launch the selected rocket
    void launchAll();             // L — launch every rocket

    // Station is fixed at the world origin
    sf::Vector2f stationPosition() const { return {0.0f, 0.0f}; }

private:
    void spawnRockets();
    void checkSweepDetection(float prevAngle, float newAngle);

    RadarConfig                m_config;
    std::vector<RocketTarget>  m_rockets;
    float                      m_scanAngleDegrees = 0.0f;
    float                      m_missionTime      = 0.0f;
    int                        m_selectedIndex    = 0;
};
