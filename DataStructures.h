#pragma once

#include <cstdint>
#include <vector>

// Enum for data format (placeholder for actual implementation)
enum class Rx_Data_Format_t {
    COMPLEX_2x16BIT,
    REAL_16BIT,
    COMPLEX_4x16BIT
};

// Target track structure
struct TargetTrack {
    uint32_t target_id;
    float level;
    float radius;
    float azimuth;        // degrees, -90 to +90
    float elevation;      // degrees
    float radial_speed;   // m/s (positive = approaching, negative = receding)
    float azimuth_speed;  // deg/s
    float elevation_speed; // deg/s
};

// Target track data container
struct TargetTrackData {
    uint32_t numTracks;
    std::vector<TargetTrack> targets;
    
    TargetTrackData() : numTracks(0) {}
    
    void clear() {
        numTracks = 0;
        targets.clear();
    }
    
    void addTarget(const TargetTrack& target) {
        targets.push_back(target);
        numTracks = static_cast<uint32_t>(targets.size());
    }
};

// Raw ADC frame structure
struct RawADCFrame {
    std::vector<float> sample_data;
    uint32_t frame_number;
    uint32_t num_chirps;
    uint8_t num_rx_antennas;
    uint32_t num_samples_per_chirp;
    uint8_t rx_mask;
    uint8_t adc_resolution;
    uint8_t interleaved_rx;
    Rx_Data_Format_t data_format;
    
    RawADCFrame() : frame_number(0), num_chirps(0), num_rx_antennas(0),
                    num_samples_per_chirp(0), rx_mask(0), adc_resolution(0),
                    interleaved_rx(0), data_format(Rx_Data_Format_t::COMPLEX_2x16BIT) {}
};

// UDP message types
enum class MessageType : uint8_t {
    TARGET_TRACK_DATA = 1,
    RAW_ADC_FRAME = 2
};

// UDP message header
struct MessageHeader {
    MessageType type;
    uint32_t data_size;
    uint32_t timestamp;
};