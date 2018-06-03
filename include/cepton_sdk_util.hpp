/*
  Copyright Cepton Technologies Inc. 2017, All rights reserved.

  Cepton Sensor SDK utilities.
*/
#pragma once

#include "cepton_sdk.hpp"

#include <cmath>
#include <cstdio>

#include <array>
#include <chrono>

namespace cepton_sdk {

//------------------------------------------------------------------------------
// Common
//------------------------------------------------------------------------------
template <typename T>
inline T square(T x) {
  return x * x;
}

/// Returns current unix timestamp [microseconds].
/**
 * This is the timestamp format used by all sdk functions.
 */
static uint64_t get_timestamp_usec() {
  auto t_epoch = std::chrono::system_clock::now().time_since_epoch();
  return std::chrono::duration_cast<std::chrono::microseconds>(t_epoch).count();
}

//------------------------------------------------------------------------------
// Points
//------------------------------------------------------------------------------
/// Convert image point to 3d point.
inline static void convert_image_point_to_point(float image_x, float image_z,
                                         float distance, float &x, float &y,
                                         float &z) {
  float hypotenuse_small = std::sqrt(square(image_x) + square(image_z) + 1.0f);
  float ratio = distance / hypotenuse_small;
  x = -image_x * ratio;
  y = ratio;
  z = -image_z * ratio;
}

/// 3d point class.
struct SensorPoint {
  uint64_t timestamp;
  float x;
  float y;
  float z;
  float intensity;
  uint8_t return_number;
  uint8_t valid;
  uint8_t saturated;
};

/// Convenience method to convert `CeptonSensorImagePoint` to
/// `cepton_sdk::SensorPoint`.
inline static void convert_sensor_image_point_to_point(
    const CeptonSensorImagePoint &image_point, SensorPoint &point) {
  point.timestamp = image_point.timestamp;
  point.intensity = image_point.intensity;
  point.return_number = image_point.return_number;
  point.valid = image_point.valid;
  point.saturated = image_point.saturated;

  convert_image_point_to_point(image_point.image_x, image_point.image_z,
                               image_point.distance, point.x, point.y, point.z);
}

// -----------------------------------------------------------------------------
// Transform
// -----------------------------------------------------------------------------
/// 3d translation and rotation.
/**
 * For more functionality, use Eigen's Geometry module.
 */
class CompiledTransform {
 public:
  /// Create from translation and rotation.
  /**
   * @param translation Cartesian (x, y, z)
   * @param rotation Quaternion (x, y, z, w)
  */
  static CompiledTransform create(const float *const translation,
                                  const float *const rotation) {
    CompiledTransform compiled_transform;
    std::copy(translation, translation + 3,
              compiled_transform.translation.begin());

    // Convert quaternion to rotation matrix
    float x = rotation[0];
    float y = rotation[1];
    float z = rotation[2];
    float w = rotation[3];
    float xx = x * x;
    float xy = x * y;
    float xz = x * z;
    float xw = x * w;
    float yy = y * y;
    float yz = y * z;
    float yw = y * w;
    float zz = z * z;
    float zw = z * w;

    compiled_transform.rotation_m00 = 1 - 2 * (yy + zz);
    compiled_transform.rotation_m01 = 2 * (xy - zw);
    compiled_transform.rotation_m02 = 2 * (xz + yw);

    compiled_transform.rotation_m10 = 2 * (xy + zw);
    compiled_transform.rotation_m11 = 1 - 2 * (xx + zz);
    compiled_transform.rotation_m12 = 2 * (yz - xw);

    compiled_transform.rotation_m20 = 2 * (xz - yw);
    compiled_transform.rotation_m21 = 2 * (yz + xw);
    compiled_transform.rotation_m22 = 1 - 2 * (xx + yy);

    return compiled_transform;
  }

  /// Apply transformation to 3d position.
  void apply(float &x, float &y, float &z) {
    float x_tmp = x * rotation_m00 + y * rotation_m01 + z * rotation_m02;
    float y_tmp = x * rotation_m10 + y * rotation_m11 + z * rotation_m12;
    float z_tmp = x * rotation_m20 + y * rotation_m21 + z * rotation_m22;

    x_tmp += translation[0];
    y_tmp += translation[1];
    z_tmp += translation[2];

    x = x_tmp;
    y = y_tmp;
    z = z_tmp;
  }

 public:
  std::array<float, 3> translation;

  // Rotation matrix
  float rotation_m00 = 1.0f;
  float rotation_m01 = 0.0f;
  float rotation_m02 = 0.0f;
  float rotation_m10 = 0.0f;
  float rotation_m11 = 1.0f;
  float rotation_m12 = 0.0f;
  float rotation_m20 = 0.0f;
  float rotation_m21 = 0.0f;
  float rotation_m22 = 1.0f;
};
}  // cepton_sdk
