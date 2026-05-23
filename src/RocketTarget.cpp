// =============================================================================
//  RocketTarget.cpp — flight state machine & simplified rocket physics
// =============================================================================

#include "RocketTarget.hpp"

#include <algorithm>
#include <cmath>

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

RocketTarget::RocketTarget(const std::string& callsign,
                           float targetAltitude,
                           float signalMaxRange)
    : m_callsign(callsign)
    , m_targetAltitude(targetAltitude)
    , m_signalMaxRange(signalMaxRange)
{
    m_separationFraction = 0.35f;
}

// ---------------------------------------------------------------------------
// Entity interface
// ---------------------------------------------------------------------------

void RocketTarget::update(float deltaSeconds)
{
    if (m_state == FlightState::SignalLost ||
        m_state == FlightState::MissionComplete) {
        tickPulse(deltaSeconds);
        return;
    }

    switch (m_state) {
        case FlightState::Idle:               updateIdle(deltaSeconds);              break;
        case FlightState::Launch:             updateLaunch(deltaSeconds);            break;
        case FlightState::Ascending:          updateAscending(deltaSeconds);         break;
        case FlightState::BoosterSeparation:  updateBoosterSeparation(deltaSeconds); break;
        case FlightState::Coasting:           updateCoasting(deltaSeconds);          break;
        default: break;
    }

    updateSignal();
    recordTrail();
    tickPulse(deltaSeconds);
}

sf::Vector2f RocketTarget::position() const
{
    // Y is negative so that higher altitude appears "up" on the radar
    return { m_telemetry.downrangeX, -m_telemetry.altitude };
}

// ---------------------------------------------------------------------------
// Commands
// ---------------------------------------------------------------------------

void RocketTarget::launch()
{
    if (m_state != FlightState::Idle) return;
    m_state          = FlightState::Launch;
    m_launchCountdown = 3.0f;
}

void RocketTarget::triggerSeparation()
{
    if (m_state != FlightState::Ascending || m_boosterSeparated) return;
    m_state            = FlightState::BoosterSeparation;
    m_separationTimer  = kSeparationDuration;
    m_boosterSeparated = true;
}

// ---------------------------------------------------------------------------
// Pulse (identical pattern to old Enemy)
// ---------------------------------------------------------------------------

void RocketTarget::triggerPulse(float duration)
{
    m_pulseDuration = duration;
    m_pulseTimer    = duration;
}

void RocketTarget::tickPulse(float deltaSeconds)
{
    if (m_pulseTimer > 0.0f) {
        m_pulseTimer -= deltaSeconds;
        if (m_pulseTimer < 0.0f) m_pulseTimer = 0.0f;
    }
}

// ---------------------------------------------------------------------------
// State: Idle — waiting on the pad
// ---------------------------------------------------------------------------

void RocketTarget::updateIdle(float /*dt*/)
{
    // Nothing happens — telemetry stays at defaults
}

// ---------------------------------------------------------------------------
// State: Launch — 3-second countdown then ignition
// ---------------------------------------------------------------------------

void RocketTarget::updateLaunch(float dt)
{
    m_missionTime += dt;
    m_launchCountdown -= dt;

    if (m_launchCountdown <= 0.0f) {
        m_state  = FlightState::Ascending;
        m_thrust = m_baseThrustFirst;
    }
}

// ---------------------------------------------------------------------------
// State: Ascending — powered flight with fuel consumption
// ---------------------------------------------------------------------------

void RocketTarget::updateAscending(float dt)
{
    m_missionTime += dt;

    // Consume fuel
    const float burnRate = m_boosterSeparated ? m_fuelBurnRate2 : m_fuelBurnRate;
    m_telemetry.fuelPercent -= burnRate * dt;
    if (m_telemetry.fuelPercent <= 0.0f) {
        m_telemetry.fuelPercent = 0.0f;
        m_thrust = 0.0f;
        m_state  = FlightState::Coasting;
        return;
    }

    // Choose thrust
    m_thrust = m_boosterSeparated ? m_baseThrustSecond : m_baseThrustFirst;

    // Drag (simplified: proportional to v²)
    const float drag = m_dragCoeff * m_telemetry.velocity * m_telemetry.velocity;

    // Net acceleration
    m_telemetry.acceleration = m_thrust - m_gravity - drag;
    m_telemetry.velocity    += m_telemetry.acceleration * dt;

    if (m_telemetry.velocity < 0.0f) m_telemetry.velocity = 0.0f;

    m_telemetry.altitude += m_telemetry.velocity * dt;
    m_telemetry.maxAltitude = std::max(m_telemetry.maxAltitude,
                                        m_telemetry.altitude);

    // Lateral drift
    m_telemetry.downrangeX += m_lateralDrift * dt;

    // Auto booster separation
    if (!m_boosterSeparated &&
        m_telemetry.altitude >= m_targetAltitude * m_separationFraction) {
        triggerSeparation();
    }

    // Mission complete?
    if (m_telemetry.altitude >= m_targetAltitude) {
        m_state = FlightState::MissionComplete;
    }
}

// ---------------------------------------------------------------------------
// State: BoosterSeparation — brief staging event
// ---------------------------------------------------------------------------

void RocketTarget::updateBoosterSeparation(float dt)
{
    m_missionTime += dt;
    m_separationTimer -= dt;

    // Reduced thrust during separation
    m_thrust = m_baseThrustSecond * 0.3f;

    const float drag = m_dragCoeff * m_telemetry.velocity * m_telemetry.velocity;
    m_telemetry.acceleration = m_thrust - m_gravity - drag;
    m_telemetry.velocity    += m_telemetry.acceleration * dt;

    if (m_telemetry.velocity < 0.0f) m_telemetry.velocity = 0.0f;

    m_telemetry.altitude += m_telemetry.velocity * dt;
    m_telemetry.maxAltitude = std::max(m_telemetry.maxAltitude,
                                        m_telemetry.altitude);
    m_telemetry.downrangeX += m_lateralDrift * dt;

    // Fuel still burning slowly during separation
    m_telemetry.fuelPercent -= m_fuelBurnRate2 * 0.5f * dt;
    if (m_telemetry.fuelPercent < 0.0f) m_telemetry.fuelPercent = 0.0f;

    if (m_separationTimer <= 0.0f) {
        m_state = FlightState::Ascending;   // return to powered flight
    }
}

// ---------------------------------------------------------------------------
// State: Coasting — no thrust, decelerating under gravity
// ---------------------------------------------------------------------------

void RocketTarget::updateCoasting(float dt)
{
    m_missionTime += dt;
    m_thrust = 0.0f;

    const float drag = m_dragCoeff * m_telemetry.velocity * std::abs(m_telemetry.velocity);
    m_telemetry.acceleration = -m_gravity - drag;
    m_telemetry.velocity    += m_telemetry.acceleration * dt;

    m_telemetry.altitude += m_telemetry.velocity * dt;

    if (m_telemetry.altitude > m_telemetry.maxAltitude) {
        m_telemetry.maxAltitude = m_telemetry.altitude;
    }

    m_telemetry.downrangeX += m_lateralDrift * 0.3f * dt;

    // Mission complete when rocket comes back down
    if (m_telemetry.altitude <= 0.0f) {
        m_telemetry.altitude  = 0.0f;
        m_telemetry.velocity  = 0.0f;
        m_telemetry.acceleration = 0.0f;
        m_state = FlightState::MissionComplete;
    }
}

// ---------------------------------------------------------------------------
// Signal strength — fades with distance from station (origin)
// ---------------------------------------------------------------------------

void RocketTarget::updateSignal()
{
    const float dist = std::sqrt(m_telemetry.altitude * m_telemetry.altitude +
                                  m_telemetry.downrangeX * m_telemetry.downrangeX);

    m_telemetry.signalStrength = 1.0f - std::clamp(dist / m_signalMaxRange,
                                                     0.0f, 1.0f);

    // Boost signal slightly — never fully 0 until well past max range
    m_telemetry.signalStrength = std::pow(m_telemetry.signalStrength, 0.6f);

    if (m_telemetry.signalStrength < 0.05f &&
        m_state != FlightState::Idle &&
        m_state != FlightState::MissionComplete) {
        m_state = FlightState::SignalLost;
        m_telemetry.signalStrength = 0.0f;
    }
}

// ---------------------------------------------------------------------------
// Trail recording
// ---------------------------------------------------------------------------

void RocketTarget::recordTrail()
{
    if (m_state == FlightState::Idle) return;

    m_trailTimer += 0.016f; // approximate dt — called each frame
    if (m_trailTimer >= m_trailInterval) {
        m_trailTimer = 0.0f;
        m_trail.push_back(position());
        if (m_trail.size() > kMaxTrailPoints) {
            m_trail.erase(m_trail.begin());
        }
    }
}
