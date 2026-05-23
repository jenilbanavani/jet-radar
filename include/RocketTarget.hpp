#pragma once

#include "Entity.hpp"

#include <SFML/System/Vector2.hpp>

#include <string>
#include <vector>

// ============================================================================
//  Flight state machine for rocket tracking
// ============================================================================
enum class FlightState {
    Idle,               // On the pad, pre-launch
    Launch,             // Ignition sequence (~3 s countdown)
    Ascending,          // Powered flight — main engine burn
    BoosterSeparation,  // Staging event — brief visual transition
    Coasting,           // Unpowered ascent after burnout
    SignalLost,         // Out of range or anomaly
    MissionComplete     // Reached target altitude or returned
};

// ============================================================================
//  Live telemetry snapshot
// ============================================================================
struct TelemetryData {
    float altitude       = 0.0f;   // metres
    float velocity       = 0.0f;   // m/s   (vertical component)
    float acceleration   = 0.0f;   // m/s²
    float fuelPercent    = 100.0f;
    float signalStrength = 1.0f;   // 0.0 – 1.0
    float maxAltitude    = 0.0f;   // peak altitude reached so far
    float downrangeX     = 0.0f;   // lateral drift in metres
};

// ============================================================================
//  RocketTarget — replaces the old Enemy entity
// ============================================================================
class RocketTarget final : public Entity {
public:
    RocketTarget(const std::string& callsign, float targetAltitude,
                 float signalMaxRange);

    void update(float deltaSeconds) override;

    /// Returns world position as (downrangeX, -altitude) so that
    /// higher altitude → more negative Y → "upward" on the radar.
    sf::Vector2f position() const override;

    // ---- Telemetry accessors ------------------------------------------------
    const TelemetryData& telemetry()    const { return m_telemetry; }
    FlightState          flightState()  const { return m_state; }
    const std::string&   callsign()     const { return m_callsign; }
    float                missionTime()  const { return m_missionTime; }
    float                targetAltitude() const { return m_targetAltitude; }

    // ---- Commands -----------------------------------------------------------
    void launch();              // Idle → Launch
    void triggerSeparation();   // Force booster separation

    // ---- Radar pulse (same pattern as old Enemy) ----------------------------
    float pulseTimer()    const { return m_pulseTimer; }
    void  triggerPulse(float duration);
    void  tickPulse(float deltaSeconds);

    // ---- Trajectory trail ---------------------------------------------------
    const std::vector<sf::Vector2f>& trail() const { return m_trail; }

private:
    void updateIdle(float dt);
    void updateLaunch(float dt);
    void updateAscending(float dt);
    void updateBoosterSeparation(float dt);
    void updateCoasting(float dt);
    void updateSignal();
    void recordTrail();

    std::string   m_callsign;
    FlightState   m_state           = FlightState::Idle;
    TelemetryData m_telemetry;

    float m_missionTime      = 0.0f;
    float m_targetAltitude   = 12000.0f;
    float m_signalMaxRange   = 14000.0f;

    // Physics
    float m_thrust           = 0.0f;     // current thrust acceleration  m/s²
    float m_baseThrustFirst  = 38.0f;    // first-stage thrust  m/s²
    float m_baseThrustSecond = 22.0f;    // second-stage thrust m/s²
    float m_gravity          = 9.81f;
    float m_dragCoeff        = 0.00004f;
    float m_lateralDrift     = 2.5f;     // gentle sideways drift m/s

    // Fuel
    float m_fuelBurnRate     = 6.0f;     // % per second (first stage)
    float m_fuelBurnRate2    = 3.5f;     // % per second (second stage)

    // Staging
    float m_separationFraction = 0.35f;  // separate at 35% of target alt
    float m_separationTimer    = 0.0f;
    bool  m_boosterSeparated   = false;
    static constexpr float kSeparationDuration = 1.8f;

    // Launch countdown
    float m_launchCountdown  = 3.0f;

    // Pulse
    float m_pulseTimer       = 0.0f;
    float m_pulseDuration    = 1.4f;

    // Trail
    std::vector<sf::Vector2f> m_trail;
    float m_trailInterval    = 0.15f;    // seconds between trail points
    float m_trailTimer       = 0.0f;
    static constexpr std::size_t kMaxTrailPoints = 200;
};
