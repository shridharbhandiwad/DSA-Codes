# Radar Visualization Application

A comprehensive Qt C++ GUI application for real-time radar data visualization, featuring UDP data reception, semi-circular PPI plots, and FFT spectrum analysis.

## Features

### 🎯 Target Track Visualization
- **Semi-circular PPI Plot**: Range vs azimuth display covering -90° to +90°
- **Color-coded Targets**: Visual distinction between approaching (red), receding (green), and stationary (blue) targets
- **Interactive Interface**: Click to select targets, hover for detailed information
- **Velocity Vectors**: Visual representation of target movement with directional arrows
- **Range Rings & Grid**: Configurable display options for better spatial reference

### 📊 Frequency Domain Analysis
- **Real-time FFT Processing**: Live spectrum analysis of ADC sample data
- **Interactive Plot**: Click and hover for frequency/magnitude information
- **Windowing**: Hamming window applied for improved spectral quality
- **Auto-scaling**: Dynamic magnitude range adjustment
- **Customizable Display**: Toggle log scale and grid options

### 🌐 Network Communication
- **UDP Reception**: Listens on port 5000 for incoming radar data
- **Dual Data Types**: Handles both target track data and raw ADC frames
- **Simulation Mode**: Built-in data generator for testing and demonstration
- **Error Handling**: Robust network error detection and reporting

### 📋 Data Management
- **Target Table**: Comprehensive listing of all detected targets with full attributes
- **Color-coded Rows**: Speed-based color coding matching the PPI display
- **Real-time Updates**: Live data refresh every 50ms
- **Statistics Tracking**: Frame count, target count, and data rate monitoring

## Data Structures

### Target Track Data
```cpp
struct TargetTrack {
    uint32_t target_id;      // Unique target identifier
    float level;             // Signal level (dB)
    float radius;            // Range from radar (meters)
    float azimuth;           // Azimuth angle (-90° to +90°)
    float elevation;         // Elevation angle (degrees)
    float radial_speed;      // Radial velocity (m/s, +approaching, -receding)
    float azimuth_speed;     // Azimuth rate (deg/s)
    float elevation_speed;   // Elevation rate (deg/s)
};
```

### Raw ADC Frame Data
```cpp
struct RawADCFrame {
    std::vector<float> sample_data;  // I/Q interleaved samples
    uint32_t frame_number;           // Frame sequence number
    uint32_t num_chirps;             // Number of chirps in frame
    uint8_t num_rx_antennas;         // Number of receive antennas
    uint32_t num_samples_per_chirp;  // Samples per chirp
    uint8_t rx_mask;                 // Active receive antenna mask
    uint8_t adc_resolution;          // ADC resolution in bits
    uint8_t interleaved_rx;          // Rx interleaving flag
    Rx_Data_Format_t data_format;    // Data format specification
};
```

## Building the Application

### Prerequisites
- **Qt 5.12+** or **Qt 6.0+**
- **CMake 3.16+**
- **C++17 compatible compiler** (GCC 7+, Clang 5+, MSVC 2017+)

### Windows Build with Qt and CMake

1. **Install Qt**: Download from [qt.io](https://www.qt.io/download)
2. **Clone/Download** the source code
3. **Open Command Prompt** in the project directory
4. **Configure and build**:

```bash
# Create build directory
mkdir build
cd build

# Configure with CMake (adjust Qt path as needed)
cmake -DCMAKE_PREFIX_PATH="C:/Qt/6.5.0/msvc2019_64" -G "Visual Studio 16 2019" ..

# Build the project
cmake --build . --config Release

# Run the application
./Release/RadarVisualizationApp.exe
```

### Linux Build

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install qt6-base-dev qt6-charts-dev cmake build-essential

# Or for Qt5
sudo apt-get install qt5-default qtcharts5-dev cmake build-essential

# Build
mkdir build && cd build
cmake ..
make -j$(nproc)

# Run
./RadarVisualizationApp
```

### macOS Build

```bash
# Install Qt via Homebrew
brew install qt6 cmake

# Build
mkdir build && cd build
cmake -DCMAKE_PREFIX_PATH="/opt/homebrew/opt/qt6" ..
make -j$(sysctl -n hw.ncpu)

# Run
./RadarVisualizationApp
```

## Usage

### Getting Started
1. **Launch the application**
2. **Default Mode**: Starts in simulation mode with automatic data generation
3. **View Real-time Data**: Observe targets in the PPI plot and frequency spectrum in the FFT display
4. **Interact**: Click on targets for selection, hover for detailed tooltips

### Controls

#### System Control
- **Start/Stop UDP**: Toggle UDP data reception on port 5000
- **Start/Stop Simulation**: Toggle simulation mode for testing
- **Clear Data**: Reset all displays and clear accumulated data

#### Display Options
- **Max Range**: Adjust PPI plot maximum range (100m - 50km)
- **Show Grid**: Toggle grid display on both plots
- **Show Range Rings**: Toggle range ring display on PPI plot
- **Log Scale**: Toggle logarithmic scale on FFT plot

### UDP Data Format

Send data to `localhost:5000` with the following message structure:

```cpp
struct MessageHeader {
    MessageType type;        // TARGET_TRACK_DATA = 1, RAW_ADC_FRAME = 2
    uint32_t data_size;      // Size of payload in bytes
    uint32_t timestamp;      // Message timestamp
};
```

Followed by the appropriate data payload (TargetTrackData or RawADCFrame).

## Architecture

The application follows modern Qt and C++ best practices:

- **RAII**: Smart pointers for automatic memory management
- **Signal/Slot**: Qt's signal-slot mechanism for loose coupling
- **MVC Pattern**: Separation of data, presentation, and control logic
- **Modern C++**: C++17 features including structured bindings and auto type deduction
- **Thread Safety**: Proper signal handling for cross-thread communication

### Key Components

- **MainWindow**: Central coordinator managing UI and data flow
- **PPIPlot**: Custom widget for polar radar display
- **FFTPlot**: Custom widget for frequency domain visualization
- **UDPReceiver**: Network communication and data parsing
- **DataStructures**: Type-safe data containers

## Performance

- **Update Rate**: 50ms (20 Hz) for smooth real-time visualization
- **FFT Processing**: Efficient radix-2 implementation with windowing
- **Memory Management**: Automatic cleanup with smart pointers
- **Rendering**: Hardware-accelerated Qt graphics with antialiasing

## Troubleshooting

### Common Issues

1. **Qt Not Found**: Ensure Qt is installed and CMAKE_PREFIX_PATH is set correctly
2. **Port 5000 In Use**: Check if another application is using the UDP port
3. **No Data Displayed**: Verify simulation mode is enabled or UDP data is being sent
4. **Build Errors**: Ensure C++17 support and all dependencies are installed

### Debug Mode

Build in debug mode for detailed logging:
```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
```

## License

This project is provided as-is for educational and demonstration purposes.

## Contributing

The codebase is designed to be easily extensible:
- Add new plot types by inheriting from QWidget
- Extend data structures for additional radar parameters
- Implement new data sources by extending UDPReceiver
- Customize visualization through the plotting widgets

---

*Built with Qt and modern C++ for reliable, real-time radar data visualization.*