#pragma once

#include <vector>
#include <string>
#include <cstdint>

class ADCLogger {
public:
    ADCLogger();

    void clear();
    void writeSample(int value);
    std::vector<int> readSamples() const;
    uint16_t toJSON(char* buf, uint16_t sz) const;

    void setSampleInterval(uint32_t valueSec);
    uint32_t getSampleInterval() const;

    void resetTimeToNext();
    void decTimeToNext();
    uint32_t timeToNext() const;

    void setUnits(std::string str);
    std::string getUnits() const;
    size_t getSampleCount() const;

private:
    size_t bufferSize;
    uint32_t sampleInterval;
    uint32_t sampleNext;
    std::string units;
    std::vector<int> buffer;
    size_t head;
    size_t count;
};
