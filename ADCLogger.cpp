#include "ADCLogger.h"
#include <sstream>
#include <iomanip>

ADCLogger::ADCLogger() {
    bufferSize = 100; // TBD make this configurable ?
    sampleInterval = 60;
    units = "Min";
    sampleNext = sampleInterval * 60; // seconds before next sample
    buffer.resize(bufferSize);
      head = 0;
      count = 0;
    }

void ADCLogger::clear() {
    head = 0;
    count = 0;
    std::fill(buffer.begin(), buffer.end(), 0);
}

void ADCLogger::writeSample(int value) {
    buffer[head] = value;
    head = (head + 1) % bufferSize;
    if (count < bufferSize) {
        ++count;
    }
}

std::vector<int> ADCLogger::readSamples() const {
    std::vector<int> samples;
    samples.reserve(count);
    size_t start = (head + bufferSize - count) % bufferSize;
    for (size_t i = 0; i < count; ++i) {
        samples.push_back(buffer[(start + i) % bufferSize]);
    }
    return samples;
}

uint16_t ADCLogger::toJSON(char* buf, uint16_t sz) const {

    auto samples = readSamples();
    /*
    std::ostringstream json;
    json << "{";
    json << "\"DataInt\":" << sampleInterval << ",";
    json << "\"TimeToNext\":" << sampleNext << ",";
    json << "\"Values\":[";
    for (size_t i = 0; i < samples.size(); ++i) {
        json << samples[i];
        if (i < samples.size() - 1)
            json << ",";
    }
    json << "]";
    json << "}";

    return json.str();
    */
   int len = snprintf(buf, sz, "{\"DataInt\":%d,\"TimeToNext\":%d,\"Values\":[", sampleInterval, sampleNext);

   for (size_t i = 0; i < samples.size() && len < sz - 8; ++i) {
       int written = snprintf(buf + len, sz - len, "%d%s", samples[i], (i < samples.size() - 1) ? "," : "");
       len += written;
   }

   len += snprintf(buf + len, sz - len, "]}");
   return len;

}

void ADCLogger::setSampleInterval(uint32_t value) {
    sampleInterval = value;
    resetTimeToNext();
}

uint32_t ADCLogger::getSampleInterval() const {
    return sampleInterval;
}

void ADCLogger::resetTimeToNext() {
    sampleNext = sampleInterval * 60; // seconds before next sample;
}
void ADCLogger::decTimeToNext() {
    sampleNext -= 1;
}

uint32_t ADCLogger::timeToNext() const {
    return sampleNext;
}

void ADCLogger::setUnits(std::string str) {

}

std::string ADCLogger::getUnits() const {
    return units;
}

size_t ADCLogger::getSampleCount() const {
    return count;
}
