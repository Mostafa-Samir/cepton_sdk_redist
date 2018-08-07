/*
  Copyright Cepton Technologies Inc. 2017, All rights reserved.

  Cepton Sensor SDK C++ interface.
*/
#pragma once

#include "cepton_sdk.h"

#include <functional>
#include <mutex>
#include <string>
#include <vector>

namespace cepton_sdk {

#include "cepton_def.h"

//------------------------------------------------------------------------------
// Errors
//------------------------------------------------------------------------------
typedef CeptonSensorErrorCode SensorErrorCode;

/// Returns string name of error code.
/**
 * Returns empty string if error code is invalid.
 */
static const char *get_error_code_name(SensorErrorCode error_code) {
  return cepton_get_error_code_name(error_code);
}

/// Returns true if error name is of the form `CEPTON_ERROR_*`, false otherwise.
static bool is_error_code(SensorErrorCode error_code) {
  return (bool)cepton_is_error_code(error_code);
}

/// Returns true if error name is of the form `CEPTON_FAULT_*`, false otherwise.
static bool is_fault_code(SensorErrorCode error_code) {
  return (bool)cepton_is_fault_code(error_code);
}

/// Type checking for error callback data. Not implemented.
/**
 * If specified type is correct, returns pointer to data, otherwise returns
 * nullptr.
 */
template <typename T>
const T *get_error_data(SensorErrorCode error_code, const void *error_data,
                        std::size_t error_data_size) {
  if (error_data_size == 0) {
    return nullptr;
  }

  switch (error_code) {
    default:
      return nullptr;
  }

  return dynamic_cast<const T *>(error_data);
}

/// Error returned by most functions.
/**
 * Implicitly convertible from/to SensorErrorCode.
 * Getter functions do not return an error, because they cannot fail.
 */
class SensorError : public std::runtime_error {
 public:
  SensorError(SensorErrorCode code_, const char *const msg)
      : std::runtime_error(create_message(code_, msg).c_str()), code(code_) {}
  SensorError(SensorErrorCode code) : SensorError(code, "") {}
  SensorError() : SensorError(CEPTON_SUCCESS) {}

  /// Returns `false` if code is `CEPTON_SUCCESS`, true otherwise.
  operator bool() const { return code; }
  operator SensorErrorCode() const { return code; }

  const char *name() const { return get_error_code_name(code); }
  bool is_error() const { return is_error_code(code); }
  bool is_fault() const { return is_fault_code(code); }

 private:
  static std::string create_message(SensorErrorCode code,
                                    const char *const msg) {
    std::array<char, 1024> result;
    if (msg[0] == '\0') {
      std::snprintf(result.data(), result.size(), "%s",
                    get_error_code_name(code));
    } else {
      std::snprintf(result.data(), result.size(), "%s: %s",
                    get_error_code_name(code), msg);
    }
    return result.data();
  }

 public:
  SensorErrorCode code;
};

/// Returns and clears the last sdk error.
/**
 * Called automatically by all C++ methods, so only useful when calling C
 * methods directly.
 */
static SensorError get_error() {
  const char *error_msg;
  const auto error_code = cepton_sdk_get_error(&error_msg);
  return SensorError(error_code, error_msg);
}

//------------------------------------------------------------------------------
// Types
//------------------------------------------------------------------------------
/// Sensor identifier.
typedef CeptonSensorHandle SensorHandle;

/// Indicates that handle was generated by capture replay.
static const SensorHandle SENSOR_HANDLE_FLAG_MOCK =
    CEPTON_SENSOR_HANDLE_FLAG_MOCK;

typedef CeptonSensorModel SensorModel;

typedef CeptonSensorInformation SensorInformation;

typedef CeptonSensorImagePoint SensorImagePoint;

//------------------------------------------------------------------------------
// SDK Setup
//------------------------------------------------------------------------------

typedef CeptonSDKControl Control;
typedef CeptonSDKFrameMode FrameMode;
typedef CeptonSDKFrameOptions FrameOptions;
typedef CeptonSDKOptions Options;

/// Create default frame options.
static FrameOptions create_frame_options() {
  return cepton_sdk_create_frame_options();
}

/// Create default options.
static Options create_options() { return cepton_sdk_create_options(); }

/// Callback for receiving sdk and sensor errors.
/**
 * Currently, `error_data` is not used.
 */
typedef void (*FpSensorErrorCallback)(SensorHandle handle,
                                      SensorErrorCode error_code,
                                      const char *error_msg,
                                      const void *error_data,
                                      size_t error_data_size, void *user_data);

static bool is_initialized() { return (bool)cepton_sdk_is_initialized(); }

/// Initializes settings and networking.
/**
 * Must be called before any other sdk function listed below.
 */
static SensorError initialize(int version,
                              const Options &options = create_options(),
                              const FpSensorErrorCallback &cb = nullptr,
                              void *const user_data = nullptr) {
  cepton_sdk_initialize(version, &options, cb, user_data);
  return get_error();
}

/// Resets everything and deallocates memory.
/**
 * Called automatically on program exit.
 */
static SensorError deinitialize() {
  cepton_sdk_deinitialize();
  return get_error();
}

static SensorError set_control_flags(Control mask, Control flags) {
  cepton_sdk_set_control_flags(mask, flags);
  return get_error();
}

static Control get_control_flags() { return cepton_sdk_get_control_flags(); }

static bool has_control_flag(Control flag) {
  return (bool)cepton_sdk_has_control_flag(flag);
}

/// Sets network listen port.
/**
 * Default: 8808.
 */
static SensorError set_port(uint16_t port) {
  cepton_sdk_set_port(port);
  return get_error();
}

static uint16_t get_port() { return cepton_sdk_get_port(); }

static SensorError set_frame_options(const CeptonSDKFrameOptions &options) {
  cepton_sdk_set_frame_options(&options);
  return get_error();
}

static FrameMode get_frame_mode() { return cepton_sdk_get_frame_mode(); }

static float get_frame_length() { return cepton_sdk_get_frame_length(); }

//------------------------------------------------------------------------------
// Points
//------------------------------------------------------------------------------
/// Callback for receiving image points.
/**
 * Set the frame length to control the callback rate.
 */
typedef void (*FpSensorImageDataCallback)(SensorHandle handle,
                                          std::size_t n_points,
                                          const SensorImagePoint *c_points,
                                          void *user_data);

/// Sets image frames callback.
/**
 * Returns points at frequency specified by `cepton_sdk::FrameOptions::mode`.
 * Each frame contains all possible points (use
 * `cepton_sdk::SensorImagePoint::valid` to filter points). Points are ordered
 * by measurement, segment, and return:
 *
 * ```
 * measurement_count = n_points / (segment_count * return_count)
 * idx = ((i_measurement) * segment_count + i_segment) * return_count + i_return
 * ```
 *
 * Returns error if callback already registered.
 */
static SensorError listen_image_frames(FpSensorImageDataCallback cb,
                                       void *const user_data) {
  cepton_sdk_listen_image_frames(cb, user_data);
  return get_error();
}

static SensorError unlisten_image_frames() {
  cepton_sdk_unlisten_image_frames();
  return get_error();
}

//------------------------------------------------------------------------------
// Sensors
//------------------------------------------------------------------------------
/**
 * Get number of sensors attached.
 * Use to check for new sensors. Sensors are not deleted until deinitialization.
 */
static std::size_t get_n_sensors() { return cepton_sdk_get_n_sensors(); }

/**
 * Returns error if sensor not found.
 */
static SensorError get_sensor_handle_by_serial_number(uint64_t serial_number,
                                                      SensorHandle &handle) {
  cepton_sdk_get_sensor_handle_by_serial_number(serial_number, &handle);
  return get_error();
}

/**
 * Valid indices are in range [0, n_sensors).
 * Returns error if index invalid.
 */
static SensorError get_sensor_information_by_index(std::size_t idx,
                                                   SensorInformation &info) {
  cepton_sdk_get_sensor_information_by_index(idx, &info);
  return get_error();
}

/**
 * Returns error if sensor not found.
 */
static SensorError get_sensor_information(SensorHandle handle,
                                          SensorInformation &info) {
  cepton_sdk_get_sensor_information(handle, &info);
  return get_error();
}

//------------------------------------------------------------------------------
// Networking
//------------------------------------------------------------------------------
/// Callback for receiving network packets.
/**
 * \param handle Unique sensor identifier (e.g. IP address).
 * Returns error if callback already set.
 */
typedef void (*FpNetworkReceiveCallback)(SensorHandle handle, int64_t timestamp,
                                         uint8_t const *buffer,
                                         size_t buffer_size, void *user_data);

/// Sets network packets callback.
/**
 * Only 1 callback can be registered.
 */
static SensorError listen_network_packets(FpNetworkReceiveCallback cb,
                                          void *const user_data) {
  cepton_sdk_listen_network_packet(cb, user_data);
  return get_error();
}

static SensorError unlisten_network_packets() {
  cepton_sdk_unlisten_network_packet();
  return get_error();
}

/// Clears sensors.
/**
 * Use when loading/unloading capture file.
 */
static SensorError clear() {
  cepton_sdk_clear();
  return get_error();
}

/// Manually passes packets to sdk.
/**
 * Blocks while processing, and calls listener callbacks synchronously before
 * returning.
 */
static SensorError mock_network_receive(SensorHandle handle, int64_t timestamp,
                                        const uint8_t *const buffer,
                                        std::size_t buffer_size) {
  cepton_sdk_mock_network_receive(handle, timestamp, buffer, buffer_size);
  return get_error();
}
//------------------------------------------------------------------------------
// Capture Replay
//------------------------------------------------------------------------------
namespace capture_replay {

static bool is_open() { return (bool)cepton_sdk_capture_replay_is_open(); }

/// Opens capture file.
/**
 * Must be called before any other replay functions listed below.
 */
static SensorError open(const std::string &path) {
  cepton_sdk_capture_replay_open(path.c_str());
  return get_error();
}

static SensorError close() {
  cepton_sdk_capture_replay_close();
  return get_error();
}

/// Returns capture start timestamp (unix time [microseconds]).
static uint64_t get_start_time() {
  return cepton_sdk_capture_replay_get_start_time();
}

/// Returns capture file position [seconds].
static float get_position() { return cepton_sdk_capture_replay_get_position(); }

/// Returns capture file time (unix time [microseconds])
static uint64_t get_time() {
  return get_start_time() + uint64_t(1e6f * get_position());
}

/// Returns capture file length [seconds].
static float get_length() { return cepton_sdk_capture_replay_get_length(); }

/// Returns true if at end of capture file.
/**
 * This is only relevant when using `resume_blocking` methods.
 */
static bool is_end() { return (bool)cepton_sdk_capture_replay_is_end(); }

/// Seek to capture file position [seconds].
/**
 * Position must be in range [0.0, capture length).
 * Returns error if position is invalid.
 */
static SensorError seek(float position) {
  cepton_sdk_capture_replay_seek(position);
  return get_error();
}

/// Seek to relative capture file position [seconds].
/**
 * Returns error if position is invalid.
 */
static SensorError seek_relative(float position) {
  position += get_position();
  cepton_sdk_capture_replay_seek(position);
  return get_error();
}

/// If enabled, replay will automatically rewind at end.
static SensorError set_enable_loop(bool value) {
  cepton_sdk_capture_replay_set_enable_loop((int)value);
  return get_error();
}

static bool get_enable_loop() {
  return (bool)cepton_sdk_capture_replay_get_enable_loop();
}

/// Replay speed multiplier for asynchronous replay.
static SensorError set_speed(float speed) {
  cepton_sdk_capture_replay_set_speed(speed);
  return get_error();
}

static float get_speed() { return cepton_sdk_capture_replay_get_speed(); }

/// Replay next packet in current thread without sleeping.
/**
 * Pauses replay thread if it is running.
 */
static SensorError resume_blocking_once() {
  cepton_sdk_capture_replay_resume_blocking_once();
  return get_error();
}

/// Replay multiple packets synchronously.
/**
 * No sleep between packets. Resume duration must be non-negative.
 * Pauses replay thread if it is running.
 */
static SensorError resume_blocking(float duration) {
  cepton_sdk_capture_replay_resume_blocking(duration);
  return get_error();
}

/// Returns true if replay thread is running.
static bool is_running() { return cepton_sdk_capture_replay_is_running(); }

// Resumes asynchronous replay thread.
/**
 * Packets are replayed in realtime. Replay thread sleeps in between packets.
 */
static SensorError resume() {
  cepton_sdk_capture_replay_resume();
  return get_error();
}

/// Pauses asynchronous replay thread.
static SensorError pause() {
  cepton_sdk_capture_replay_pause();
  return get_error();
}

#include "cepton_undef.h"

}  // namespace capture_replay
}  // namespace cepton_sdk
