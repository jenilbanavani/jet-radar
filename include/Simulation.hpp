#pragma once

#include "Enemy.hpp"
#include "Player.hpp"
#include "RadarConfig.hpp"

#include <cstddef>
#include <vector>

// State machine for the lock-on sequence
enum class LockState {
    None,       // no target selected
    Acquiring,  // target selected, timer counting down
    Locked      // lock confirmed — missile ready
};

class Simulation {
public:
    explicit Simulation(const RadarConfig& config);

    void update(float deltaSeconds);

    const Player&             player()           const;
    const std::vector<Enemy>& enemies()          const;
    std::vector<Enemy>&       enemies();            // non-const for pulse trigger
    float                     scanAngleDegrees() const;
    const RadarConfig&        config()           const;
    float                     totalTime()        const { return m_totalTime; }

    // Lock-on / target selection
    int       lockedEnemyIndex() const { return m_lockedIndex; }
    LockState lockState()        const { return m_lockState; }
    float     lockProgress()     const; // 0.0 -> 1.0 (acquiring fraction)
    bool      missileReady()     const { return m_lockState == LockState::Locked; }

    void cycleLockOn();                    // Tab — next visible enemy
    void clearLockOn();                    // ESC — reset everything
    bool fireMissile();                    // SPACE — fire if LOCKED, returns true on success

    void setPlayerVelocity(sf::Vector2f velocity);
    float missileFlashTimer() const { return m_missileFlashTimer; }

private:
    void spawnEnemies();
    void checkSweepDetection(float prevAngle, float newAngle);
    void updateLockTimer(float deltaSeconds);

    RadarConfig          m_config;
    Player               m_player;
    std::vector<Enemy>   m_enemies;
    float                m_scanAngleDegrees = 0.0f;
    float                m_totalTime        = 0.0f;

    int       m_lockedIndex   = -1;               // -1 = no target
    LockState m_lockState     = LockState::None;
    float     m_lockTimer     = 0.0f;             // time spent acquiring
    bool      m_missilesFired = false;            // for post-fire HUD state
    float     m_missileFlashTimer = 0.0f;         // full-screen flash countdown
};

