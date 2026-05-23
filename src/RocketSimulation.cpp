// =============================================================================
//  RocketSimulation.cpp
// =============================================================================

#include "RocketSimulation.hpp"

#include <algorithm>
#include <cmath>

// ---------------------------------------------------------------------------
// Helpers (carried over from old Simulation.cpp)
// ---------------------------------------------------------------------------
namespace {

constexpr float kPi = 3.14159265359f;

float length(sf::Vector2f v)
{
    return std::sqrt(v.x * v.x + v.y * v.y);
}

float normaliseAngle(float deg)
{
    deg = std::fmod(deg, 360.0f);
    if (deg < 0.0f) deg += 360.0f;
    return deg;
}

bool inSweep(float start, float end, float test)
{
    start = normaliseAngle(start);
    end   = normaliseAngle(end);
    test  = normaliseAngle(test);

    if (start <= end) {
        return test >= start && test <= end;
    } else {
        return test >= start || test <= end;
    }
}

float bearing(sf::Vector2f from, sf::Vector2f to)
{
    const sf::Vector2f delta = to - from;
    float deg = std::atan2(delta.y, delta.x) * 180.0f / kPi;
    return normaliseAngle(deg);
}

} // namespace

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

RocketSimulation::RocketSimulation(const RadarConfig& config)
    : m_config(config)
{
    spawnRockets();
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void RocketSimulation::update(float deltaSeconds)
{
    m_missionTime += deltaSeconds;

    const float prevAngle = m_scanAngleDegrees;

    m_scanAngleDegrees += m_config.scanDegreesPerSecond * deltaSeconds;
    if (m_scanAngleDegrees >= 360.0f) {
        m_scanAngleDegrees = std::fmod(m_scanAngleDegrees, 360.0f);
    }

    for (RocketTarget& rocket : m_rockets) {
        rocket.update(deltaSeconds);
    }

    checkSweepDetection(prevAngle, m_scanAngleDegrees);
}

// ---------------------------------------------------------------------------
// Accessors
// ---------------------------------------------------------------------------

const std::vector<RocketTarget>& RocketSimulation::rockets()          const { return m_rockets; }
std::vector<RocketTarget>&       RocketSimulation::rockets()                { return m_rockets; }
float                            RocketSimulation::scanAngleDegrees() const { return m_scanAngleDegrees; }
const RadarConfig&               RocketSimulation::config()           const { return m_config; }
float                            RocketSimulation::missionElapsed()   const { return m_missionTime; }
int                              RocketSimulation::selectedRocketIndex() const { return m_selectedIndex; }

// ---------------------------------------------------------------------------
// Rocket selection
// ---------------------------------------------------------------------------

void RocketSimulation::cycleSelection()
{
    if (m_rockets.empty()) return;
    m_selectedIndex = (m_selectedIndex + 1) % static_cast<int>(m_rockets.size());
}

void RocketSimulation::launchSelected()
{
    if (m_selectedIndex >= 0 &&
        m_selectedIndex < static_cast<int>(m_rockets.size())) {
        m_rockets[m_selectedIndex].launch();
    }
}

void RocketSimulation::launchAll()
{
    for (RocketTarget& rocket : m_rockets) {
        rocket.launch();
    }
}

// ---------------------------------------------------------------------------
// Spawn rockets on the pad
// ---------------------------------------------------------------------------

void RocketSimulation::spawnRockets()
{
    m_rockets.emplace_back("ALPHA",  m_config.defaultTargetAltitude,
                           m_config.signalMaxRange);
    m_rockets.emplace_back("BRAVO",  m_config.defaultTargetAltitude * 0.75f,
                           m_config.signalMaxRange);
    m_rockets.emplace_back("CHARLIE", m_config.defaultTargetAltitude * 1.2f,
                           m_config.signalMaxRange);
}

// ---------------------------------------------------------------------------
// Sweep detection — trigger pulse when the sweep line crosses a rocket
// ---------------------------------------------------------------------------

void RocketSimulation::checkSweepDetection(float prevAngle, float newAngle)
{
    const sf::Vector2f station = stationPosition();

    for (RocketTarget& rocket : m_rockets) {
        if (rocket.flightState() == FlightState::Idle) continue;

        const float dist = length(rocket.position() - station);
        if (dist > m_config.radarRange) continue;

        const float rocketBearing = bearing(station, rocket.position());

        if (inSweep(prevAngle, newAngle, rocketBearing)) {
            rocket.triggerPulse(m_config.pulseDuration);
        }
    }
}
