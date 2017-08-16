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
        this->switchBackTimer.initializeMs(100, TimerDelegate(&PulseFeature::switchOff, this));
        this->registerSubscription("pulse", Device::MessageCallback(&PulseFeature::onMessageReceived, this));

        LOG.log("Initialized");
    }

    void doPulse(const int ms) {
        switchBackTimer.setIntervalMs(ms);
        digitalWrite(gpio, !invert);
        switchBackTimer.start(false);
    }

protected:
    virtual void publishCurrentState() {};

private:
    void onMessageReceived(const String& topic, const String& message) {
        const uint32_t now = RTC.getRtcSeconds();

        if (damper > 0) {
            if (this->lastChange + damper > now) {
                this->publish("error", "cooldown active, try again later", false);
                LOG.log("message ignored because still in damping");
                return;
            }
        }
        const int value = atoi(message.c_str());
        if (value <= 0) {
            this->publish("error", "message parsed as 0", false);
            LOG.log("Message parsed as 0 :", message);
        } else if (value > max_duration) {
            this->publish("error", "duration to long", false);
            LOG.log("Message > max duration :", message);
        } else {
            this->publish("feedback", message, false);
            doPulse(value);
        }
        this->lastChange = now;
    }

    void switchOff() {
        digitalWrite(gpio, invert);
    }

    uint32_t lastChange;
    Timer switchBackTimer;
};

#endif
