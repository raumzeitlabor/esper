#ifndef PULSEFEATURE_H
#define PULSEFEATURE_H

#include "Feature.h"

template<const char* const name, uint16_t gpio, bool invert = false, uint16_t damper = 0, uint16_t max_duration = 0>
class PulseFeature : public Feature<name> {

protected:
    using Feature<name>::LOG;

public:
    PulseFeature(Device* device) :
            Feature<name>(device),
            lastChange(RTC.getRtcSeconds()) {
        pinMode(gpio, OUTPUT);
        digitalWrite(gpio, invert);
        this->timeoutTimer.initializeMs(100, TimerDelegate(&PulseFeature::onTimeout, this));
        this->registerSubscription("pulse", Device::MessageCallback(&PulseFeature::onMessageReceived, this));
    }

    void doPulse(const uint16_t& duration) {
        digitalWrite(gpio, !invert);
        timeoutTimer.setIntervalMs(min(max_duration, duration));
        timeoutTimer.start(false);
    }

protected:
    virtual void publishCurrentState() {};

private:
    void onMessageReceived(const String& topic, const String& message) {
        const uint32_t now = RTC.getRtcSeconds();

        if (damper > 0) {
            if (this->lastChange + damper > now) {
                LOG.log("message ignored because still in damping");
                return;
            }
        }
        const int value = message.toInt();
        if (value <= 0) {
            LOG.log("Message parsed as 0 :", message);
        } else if (value > max_duration) {
            LOG.log("Message > max duration :", message);
        } else {
            this->publish("triggered", String(value), false);
            doPulse(value);
        }
        this->lastChange = now;
    }

    void onTimeout() {
        digitalWrite(gpio, invert);
    }

    uint32_t lastChange;
    Timer timeoutTimer;
};

#endif
