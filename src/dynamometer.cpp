#include "../include/dynamometer.h"

#include "../include/units.h"

#include <cmath>

Dynamometer::Dynamometer() {
    m_measurementTimeLeft = 0.0;
    m_state = State::Idle;
    m_timeout = 5.0;
    m_maxTorque = units::torque(2000.0, units::ft_lb);
    m_torqueVelocity = 0.0;
}

Dynamometer::~Dynamometer() {
    /* void */
}

void Dynamometer::initialize(Crankshaft *crankshaft) {
    m_device.m_crankshaft = crankshaft;
}

bool Dynamometer::isReady() const {
    switch (m_state) {
        case State::Idle:
        case State::Timeout:
        case State::Complete:
            return true;
        default:
            return false;
    }
}

void Dynamometer::startMeasurement(double window, double speed) {
    m_measurementTimeLeft = window;
    m_timeoutTimeLeft = m_timeout;

    m_lastMeasurement.Speed = speed;
    m_lastMeasurement.Samples = 0;
    m_lastMeasurement.Torque = 0;
    m_lastMeasurement.Window = window;

    m_state = State::Measuring;
}

void Dynamometer::update(double dt) {
    State nextState = m_state;
    switch (m_state) {
        case State::Idle:
        case State::Timeout:
        case State::Complete:
            break;
        case State::Measuring:
            nextState = measure(dt);
            break;
    }

    m_state = nextState;
}

Dynamometer::State Dynamometer::measure(double dt) {
    const double v = std::abs(m_device.m_crankshaft->m_body.v_theta);
    const double delta = v - m_lastMeasurement.Speed;

    m_device.m_torque = std::fmin(
            m_maxTorque,
            std::fmax(
                m_device.m_torque + m_torqueVelocity * dt,
                0.0));

    m_torqueVelocity += (5.0 * delta - 0.5 * m_torqueVelocity) * dt;

    m_maxSpeedDeviation = std::fmax(
        m_maxSpeedDeviation,
        std::abs(m_device.m_crankshaft->m_body.v_theta));

    const int samples = m_lastMeasurement.Samples;
    const double s = (double)samples / (samples + 1.0);

    m_lastMeasurement.Torque =
        s * m_lastMeasurement.Torque + m_device.m_torque / (samples + 1.0);
    m_lastMeasurement.Samples += 1;

    m_measurementTimeLeft -= dt;
    m_timeoutTimeLeft -= dt;

    if (m_measurementTimeLeft <= 0) {
        const double err = m_maxSpeedDeviation / m_lastMeasurement.Speed;
        if (err < 0.01) {
            return State::Complete;
        }
        else {
            m_measurementTimeLeft = m_lastMeasurement.Window;
        }
    }

    if (m_timeoutTimeLeft <= 0) {
        return State::Timeout;
    }
    else {
        return State::Measuring;
    }
}
