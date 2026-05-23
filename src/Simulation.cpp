#include "Simulation.hpp"

#include <algorithm>
#include <cmath>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
namespace {

constexpr float kPi = 3.14159265359f;

float length(sf::Vector2f v)
{
    return std::sqrt(v.x * v.x + v.y * v.y);
}

// Normalise an angle into [0, 360)
float normaliseAngle(float deg)
{
    deg = std::fmod(deg, 360.0f);
    if (deg < 0.0f) deg += 360.0f;
    return deg;
}

// Returns true when 'test' lies in the arc swept from 'start' to 'end'
// going in the positive (clockwise) direction.
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

// Compute the bearing (in SFML degrees, clockwise from right) from 'from' to 'to'
float bearing(sf::Vector2f from, sf::Vector2f to)
{
    const sf::Vector2f delta = to - from;
    float deg = std::atan2(delta.y, delta.x) * 180.0f / kPi;
    return normaliseAngle(deg);
}

} // namespace

// ---------------------------------------------------------------------------
// Simulation
// ---------------------------------------------------------------------------

Simulation::Simulation(const RadarConfig& config)
    : m_config(config)
{
    spawnEnemies();
}

void Simulation::update(float deltaSeconds)
{
    m_totalTime += deltaSeconds;
    m_player.update(deltaSeconds);

    const float prevAngle = m_scanAngleDegrees;

    m_scanAngleDegrees += m_config.scanDegreesPerSecond * deltaSeconds;
    if (m_scanAngleDegrees >= 360.0f) {
        m_scanAngleDegrees = std::fmod(m_scanAngleDegrees, 360.0f);
    }

    for (Enemy& enemy : m_enemies) {
        enemy.update(deltaSeconds);
    }

    checkSweepDetection(prevAngle, m_scanAngleDegrees);
    updateLockTimer(deltaSeconds);

    // Tick missile-fire flash timer
    if (m_missileFlashTimer > 0.0f) {
        m_missileFlashTimer -= deltaSeconds;
        if (m_missileFlashTimer < 0.0f) m_missileFlashTimer = 0.0f;
    }
}

const Player& Simulation::player() const { return m_player; }

const std::vector<Enemy>& Simulation::enemies() const { return m_enemies; }
std::vector<Enemy>&       Simulation::enemies()       { return m_enemies; }

float Simulation::scanAngleDegrees() const { return m_scanAngleDegrees; }

const RadarConfig& Simulation::config() const { return m_config; }

// ---------------------------------------------------------------------------
// Lock-on progress
// ---------------------------------------------------------------------------

float Simulation::lockProgress() const
{
    if (m_lockState == LockState::None)     return 0.0f;
    if (m_lockState == LockState::Locked)   return 1.0f;
    return std::min(m_lockTimer / m_config.lockAcquireTime, 1.0f);
}

// ---------------------------------------------------------------------------
// cycleLockOn — Tab: select next visible enemy, restart acquire timer
// ---------------------------------------------------------------------------
void Simulation::cycleLockOn()
{
    const sf::Vector2f playerPos = m_player.position();

    std::vector<int> visible;
    for (int i = 0; i < static_cast<int>(m_enemies.size()); ++i) {
        if (length(m_enemies[i].position() - playerPos) <= m_config.radarRange) {
            visible.push_back(i);
        }
    }

    if (visible.empty()) {
        clearLockOn();
        return;
    }

    // Cycle to the next index
    if (m_lockedIndex == -1) {
        m_lockedIndex = visible.front();
    } else {
        auto it = std::find(visible.begin(), visible.end(), m_lockedIndex);
        if (it == visible.end()) {
            m_lockedIndex = visible.front();
        } else {
            ++it;
            m_lockedIndex = (it == visible.end()) ? visible.front() : *it;
        }
    }

    // Start / restart the acquisition sequence
    m_lockState     = LockState::Acquiring;
    m_lockTimer     = 0.0f;
    m_missilesFired = false;
}

// ---------------------------------------------------------------------------
// clearLockOn — ESC: full reset
// ---------------------------------------------------------------------------
void Simulation::clearLockOn()
{
    m_lockedIndex   = -1;
    m_lockState     = LockState::None;
    m_lockTimer     = 0.0f;
    m_missilesFired = false;
}

// ---------------------------------------------------------------------------
// fireMissile — SPACE: only fires when LOCKED
// ---------------------------------------------------------------------------
bool Simulation::fireMissile()
{
    if (m_lockState != LockState::Locked) return false;

    m_missilesFired = true;
    m_missileFlashTimer = 0.3f;  // trigger full-screen flash
    // Reset back to NONE so the player can select a new target
    clearLockOn();
    return true;
}

// ---------------------------------------------------------------------------
// setPlayerVelocity — forward movement commands to the player
// ---------------------------------------------------------------------------
void Simulation::setPlayerVelocity(sf::Vector2f velocity)
{
    m_player.setVelocity(velocity);
}

// ---------------------------------------------------------------------------
// Private: advance the lock-acquisition timer each frame
// ---------------------------------------------------------------------------
void Simulation::updateLockTimer(float deltaSeconds)
{
    if (m_lockState != LockState::Acquiring) return;

    // Ensure target is still in range
    if (m_lockedIndex < 0 || m_lockedIndex >= static_cast<int>(m_enemies.size())) {
        clearLockOn();
        return;
    }
    const sf::Vector2f playerPos = m_player.position();
    if (length(m_enemies[m_lockedIndex].position() - playerPos) > m_config.radarRange) {
        clearLockOn();
        return;
    }

    m_lockTimer += deltaSeconds;
    if (m_lockTimer >= m_config.lockAcquireTime) {
        m_lockTimer = m_config.lockAcquireTime;
        m_lockState = LockState::Locked;
    }
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void Simulation::spawnEnemies()
{
    constexpr float worldLimit = 1400.0f;

    m_enemies.emplace_back(sf::Vector2f(-850.0f, -460.0f), sf::Vector2f( 120.0f,   65.0f), worldLimit);
    m_enemies.emplace_back(sf::Vector2f( 260.0f, -720.0f), sf::Vector2f( -75.0f,  135.0f), worldLimit);
    m_enemies.emplace_back(sf::Vector2f(1030.0f,  190.0f), sf::Vector2f( -95.0f,  -45.0f), worldLimit);
    m_enemies.emplace_back(sf::Vector2f(-380.0f,  880.0f), sf::Vector2f(  80.0f, -115.0f), worldLimit);
    m_enemies.emplace_back(sf::Vector2f(1180.0f,-1030.0f), sf::Vector2f(-140.0f,   90.0f), worldLimit);
    m_enemies.emplace_back(sf::Vector2f(-1220.0f, 640.0f), sf::Vector2f( 105.0f,  -70.0f), worldLimit);
}

void Simulation::checkSweepDetection(float prevAngle, float newAngle)
{
    const sf::Vector2f playerPos = m_player.position();

    for (Enemy& enemy : m_enemies) {
        if (length(enemy.position() - playerPos) > m_config.radarRange) continue;

        const float enemyBearing = bearing(playerPos, enemy.position());

        if (inSweep(prevAngle, newAngle, enemyBearing)) {
            enemy.triggerPulse(m_config.pulseDuration);
        }
    }
}
