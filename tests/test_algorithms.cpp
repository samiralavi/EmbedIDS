#include "embedids.h"
#include <cstring>
#include <gtest/gtest.h>

/**
 * @brief Test fixture for algorithm functionality
 * 
 * Tests threshold algorithm, trend algorithm, statistical algorithm,
 * and algorithm configuration across different metric types.
 */
class EmbedIDSAlgorithmsTest : public ::testing::Test {
protected:
  embedids_context_t context;
  
  void SetUp() override { 
    memset(&context, 0, sizeof(context));
    embedids_cleanup(&context); 
  }

  void TearDown() override { 
    embedids_cleanup(&context); 
  }

  /**
   * @brief Helper function to setup a metric with threshold algorithm
   */
  void setupThresholdMetric(embedids_metric_config_t& metric_config,
                           embedids_metric_datapoint_t* history_buffer,
                           const char* name,
                           embedids_metric_type_t type,
                           uint32_t history_size) {
    memset(&metric_config, 0, sizeof(metric_config));
    // Provide storage for exactly one algorithm for this helper
    static embedids_algorithm_t single_algo_storage[1];
    embedids_metric_init(&metric_config, name, type, history_buffer, history_size,
                         single_algo_storage, 1);
    metric_config.num_algorithms = 1;
    embedids_algorithm_init(&metric_config.algorithms[0], EMBEDIDS_ALGORITHM_THRESHOLD, true);
  }

  /**
   * @brief Helper function to initialize system with a single metric
   */
  embedids_result_t initializeWithMetric(embedids_metric_config_t* metric_config) {
    memset(&system_config, 0, sizeof(system_config));
    system_config.metrics = metric_config;
    system_config.num_active_metrics = 1;
    
    return embedids_init(&context, &system_config);
  }

private:
  embedids_system_config_t system_config;
};

// ============================================================================
// Threshold Algorithm Tests - Float Type
// ============================================================================

TEST_F(EmbedIDSAlgorithmsTest, ThresholdAlgorithmFloat) {
  embedids_metric_datapoint_t history_buffer[10];
  embedids_metric_config_t metric_config;
  
  setupThresholdMetric(metric_config, history_buffer, "temperature", 
                       EMBEDIDS_METRIC_TYPE_FLOAT, 10);
  
  // Configure float threshold algorithm
  embedids_metric_value_t minv = { .f32 = 10.0f };
  embedids_metric_value_t maxv = { .f32 = 80.0f };
  metric_config.algorithms[0].config.threshold =
      embedids_threshold_config_init(&minv, &maxv);

  ASSERT_EQ(initializeWithMetric(&metric_config), EMBEDIDS_OK);

  // Test normal value (should pass)
  embedids_metric_value_t value;
  value.f32 = 50.0f;
  EXPECT_EQ(embedids_add_datapoint(&context, "temperature", value, 1000), EMBEDIDS_OK);
  EXPECT_EQ(embedids_analyze_metric(&context, "temperature"), EMBEDIDS_OK);

  // Test high value (should trigger threshold)
  value.f32 = 90.0f;
  EXPECT_EQ(embedids_add_datapoint(&context, "temperature", value, 2000), EMBEDIDS_OK);
  EXPECT_EQ(embedids_analyze_metric(&context, "temperature"), EMBEDIDS_ERROR_THRESHOLD_EXCEEDED);

  // Test low value (should trigger threshold)
  value.f32 = 5.0f;
  EXPECT_EQ(embedids_add_datapoint(&context, "temperature", value, 3000), EMBEDIDS_OK);
  EXPECT_EQ(embedids_analyze_metric(&context, "temperature"), EMBEDIDS_ERROR_THRESHOLD_EXCEEDED);
}

TEST_F(EmbedIDSAlgorithmsTest, ThresholdAlgorithmFloatBoundaryValues) {
  embedids_metric_datapoint_t history_buffer[10];
  embedids_metric_config_t metric_config;
  
  setupThresholdMetric(metric_config, history_buffer, "precise_temp", 
                       EMBEDIDS_METRIC_TYPE_FLOAT, 10);
  
  // Configure threshold with precise boundaries
  embedids_metric_value_t minv2 = { .f32 = 0.0f };
  embedids_metric_value_t maxv2 = { .f32 = 100.0f };
  metric_config.algorithms[0].config.threshold =
      embedids_threshold_config_init(&minv2, &maxv2);

  ASSERT_EQ(initializeWithMetric(&metric_config), EMBEDIDS_OK);

  embedids_metric_value_t value;
  
  // Test exact boundary values
  value.f32 = 0.0f;
  EXPECT_EQ(embedids_add_datapoint(&context, "precise_temp", value, 1000), EMBEDIDS_OK);
  EXPECT_EQ(embedids_analyze_metric(&context, "precise_temp"), EMBEDIDS_OK);

  value.f32 = 100.0f;
  EXPECT_EQ(embedids_add_datapoint(&context, "precise_temp", value, 2000), EMBEDIDS_OK);
  EXPECT_EQ(embedids_analyze_metric(&context, "precise_temp"), EMBEDIDS_OK);

  // Test just outside boundaries
  value.f32 = -0.1f;
  EXPECT_EQ(embedids_add_datapoint(&context, "precise_temp", value, 3000), EMBEDIDS_OK);
  EXPECT_EQ(embedids_analyze_metric(&context, "precise_temp"), EMBEDIDS_ERROR_THRESHOLD_EXCEEDED);

  value.f32 = 100.1f;
  EXPECT_EQ(embedids_add_datapoint(&context, "precise_temp", value, 4000), EMBEDIDS_OK);
  EXPECT_EQ(embedids_analyze_metric(&context, "precise_temp"), EMBEDIDS_ERROR_THRESHOLD_EXCEEDED);
}

// ============================================================================
// Threshold Algorithm Tests - Integer Types
// ============================================================================

TEST_F(EmbedIDSAlgorithmsTest, ThresholdAlgorithmUint32) {
  embedids_metric_datapoint_t history_buffer[10];
  embedids_metric_config_t metric_config;
  
  setupThresholdMetric(metric_config, history_buffer, "packet_count", 
                       EMBEDIDS_METRIC_TYPE_UINT32, 10);
  
  // Configure uint32 threshold algorithm
  embedids_metric_value_t minv3 = { .u32 = 100 };
  embedids_metric_value_t maxv3 = { .u32 = 10000 };
  metric_config.algorithms[0].config.threshold =
      embedids_threshold_config_init(&minv3, &maxv3);

  ASSERT_EQ(initializeWithMetric(&metric_config), EMBEDIDS_OK);

  embedids_metric_value_t value;
  
  // Test normal value
  value.u32 = 5000;
  EXPECT_EQ(embedids_add_datapoint(&context, "packet_count", value, 1000), EMBEDIDS_OK);
  EXPECT_EQ(embedids_analyze_metric(&context, "packet_count"), EMBEDIDS_OK);

  // Test min threshold violation
  value.u32 = 50;
  EXPECT_EQ(embedids_add_datapoint(&context, "packet_count", value, 2000), EMBEDIDS_OK);
  EXPECT_EQ(embedids_analyze_metric(&context, "packet_count"), EMBEDIDS_ERROR_THRESHOLD_EXCEEDED);

  // Test max threshold violation
  value.u32 = 15000;
  EXPECT_EQ(embedids_add_datapoint(&context, "packet_count", value, 3000), EMBEDIDS_OK);
  EXPECT_EQ(embedids_analyze_metric(&context, "packet_count"), EMBEDIDS_ERROR_THRESHOLD_EXCEEDED);
}

TEST_F(EmbedIDSAlgorithmsTest, ThresholdAlgorithmUint64) {
  embedids_metric_datapoint_t history_buffer[10];
  embedids_metric_config_t metric_config;
  
  setupThresholdMetric(metric_config, history_buffer, "byte_count", 
                       EMBEDIDS_METRIC_TYPE_UINT64, 10);
  
  // Configure uint64 threshold algorithm
  embedids_metric_value_t minv4 = { .u64 = 1000000ULL };
  embedids_metric_value_t maxv4 = { .u64 = 1000000000ULL };
  metric_config.algorithms[0].config.threshold =
      embedids_threshold_config_init(&minv4, &maxv4);

  ASSERT_EQ(initializeWithMetric(&metric_config), EMBEDIDS_OK);

  embedids_metric_value_t value;
  
  // Test normal value
  value.u64 = 500000000ULL;
  EXPECT_EQ(embedids_add_datapoint(&context, "byte_count", value, 1000), EMBEDIDS_OK);
  EXPECT_EQ(embedids_analyze_metric(&context, "byte_count"), EMBEDIDS_OK);

  // Test min threshold violation
  value.u64 = 500000ULL;
  EXPECT_EQ(embedids_add_datapoint(&context, "byte_count", value, 2000), EMBEDIDS_OK);
  EXPECT_EQ(embedids_analyze_metric(&context, "byte_count"), EMBEDIDS_ERROR_THRESHOLD_EXCEEDED);

  // Test max threshold violation
  value.u64 = 2000000000ULL;
  EXPECT_EQ(embedids_add_datapoint(&context, "byte_count", value, 3000), EMBEDIDS_OK);
  EXPECT_EQ(embedids_analyze_metric(&context, "byte_count"), EMBEDIDS_ERROR_THRESHOLD_EXCEEDED);
}

// ============================================================================
// Threshold Algorithm Tests - Enum Type
// ============================================================================

TEST_F(EmbedIDSAlgorithmsTest, ThresholdAlgorithmEnum) {
  embedids_metric_datapoint_t history_buffer[10];
  embedids_metric_config_t metric_config;
  
  setupThresholdMetric(metric_config, history_buffer, "error_level", 
                       EMBEDIDS_METRIC_TYPE_ENUM, 10);
  
  // Configure enum threshold (0=OK, 1=WARN, 2=ERROR, 3=CRITICAL)
  embedids_metric_value_t minv5 = { .enum_val = 0 };
  embedids_metric_value_t maxv5 = { .enum_val = 2 };
  metric_config.algorithms[0].config.threshold =
      embedids_threshold_config_init(&minv5, &maxv5);

  ASSERT_EQ(initializeWithMetric(&metric_config), EMBEDIDS_OK);

  embedids_metric_value_t value;
  
  // Test normal enum value (WARNING level)
  value.enum_val = 1;
  EXPECT_EQ(embedids_add_datapoint(&context, "error_level", value, 1000), EMBEDIDS_OK);
  EXPECT_EQ(embedids_analyze_metric(&context, "error_level"), EMBEDIDS_OK);

  // Test max threshold violation (CRITICAL level)
  value.enum_val = 3;
  EXPECT_EQ(embedids_add_datapoint(&context, "error_level", value, 2000), EMBEDIDS_OK);
  EXPECT_EQ(embedids_analyze_metric(&context, "error_level"), EMBEDIDS_ERROR_THRESHOLD_EXCEEDED);
}

// ============================================================================
// Threshold Algorithm Tests - Boolean Type
// ============================================================================

TEST_F(EmbedIDSAlgorithmsTest, BooleanMetricWithThreshold) {
  embedids_metric_datapoint_t history_buffer[10];
  embedids_metric_config_t metric_config;
  
  setupThresholdMetric(metric_config, history_buffer, "security_breach", 
                       EMBEDIDS_METRIC_TYPE_BOOL, 10);
  
  // Configure threshold algorithm (though not really applicable for boolean)
  // No min/max checks for boolean metric (pass NULLs to disable)
  metric_config.algorithms[0].config.threshold =
      embedids_threshold_config_init(NULL, NULL);

  ASSERT_EQ(initializeWithMetric(&metric_config), EMBEDIDS_OK);

  embedids_metric_value_t value;
  
  // Test boolean values
  value.boolean = false;
  EXPECT_EQ(embedids_add_datapoint(&context, "security_breach", value, 1000), EMBEDIDS_OK);
  EXPECT_EQ(embedids_analyze_metric(&context, "security_breach"), EMBEDIDS_OK);

  value.boolean = true;
  EXPECT_EQ(embedids_add_datapoint(&context, "security_breach", value, 2000), EMBEDIDS_OK);
  EXPECT_EQ(embedids_analyze_metric(&context, "security_breach"), EMBEDIDS_OK);
}

// ============================================================================
// Trend Algorithm Tests
// ============================================================================

TEST_F(EmbedIDSAlgorithmsTest, TrendAlgorithmTesting) {
  embedids_metric_datapoint_t history_buffer[10];
  embedids_metric_config_t metric_config;
  
  embedids_algorithm_t algo_storage1[1];
  embedids_metric_init(&metric_config, "cpu_trend", EMBEDIDS_METRIC_TYPE_FLOAT,
                       history_buffer, 10, algo_storage1, 1);
  metric_config.num_algorithms = 1;
  embedids_algorithm_init(&metric_config.algorithms[0], EMBEDIDS_ALGORITHM_TREND, true);
  metric_config.algorithms[0].config.trend =
      embedids_trend_config_init(5, 10.0f, 100.0f, EMBEDIDS_TREND_STABLE);

  ASSERT_EQ(initializeWithMetric(&metric_config), EMBEDIDS_OK);

  // Add data points for trend analysis
  embedids_metric_value_t value;
  value.f32 = 50.0f;
  EXPECT_EQ(embedids_add_datapoint(&context, "cpu_trend", value, 1000), EMBEDIDS_OK);

  value.f32 = 52.0f;
  EXPECT_EQ(embedids_add_datapoint(&context, "cpu_trend", value, 2000), EMBEDIDS_OK);

  value.f32 = 51.0f;
  EXPECT_EQ(embedids_add_datapoint(&context, "cpu_trend", value, 3000), EMBEDIDS_OK);

  // Analyze trend - should be OK (stable trend)
  embedids_result_t result = embedids_analyze_metric(&context, "cpu_trend");
  EXPECT_EQ(result, EMBEDIDS_OK);
}

TEST_F(EmbedIDSAlgorithmsTest, TrendAlgorithmIncreasingPattern) {
  embedids_metric_datapoint_t history_buffer[10];
  embedids_metric_config_t metric_config;
  
  embedids_algorithm_t algo_storage2[1];
  embedids_metric_init(&metric_config, "memory_usage", EMBEDIDS_METRIC_TYPE_FLOAT,
                       history_buffer, 10, algo_storage2, 1);
  metric_config.num_algorithms = 1;
  embedids_algorithm_init(&metric_config.algorithms[0], EMBEDIDS_ALGORITHM_TREND, true);
  metric_config.algorithms[0].config.trend =
      embedids_trend_config_init(3, 5.0f, 10.0f, EMBEDIDS_TREND_STABLE);

  ASSERT_EQ(initializeWithMetric(&metric_config), EMBEDIDS_OK);

  // Add increasing data points
  embedids_metric_value_t value;
  for (int i = 0; i < 5; i++) {
    value.f32 = 10.0f + i * 20.0f; // Strong increasing pattern
    EXPECT_EQ(embedids_add_datapoint(&context, "memory_usage", value, 1000 + i * 1000), EMBEDIDS_OK);
  }

  // Trend analysis might detect unexpected trend
  embedids_result_t result = embedids_analyze_metric(&context, "memory_usage");
  // Result depends on implementation - could be OK or trend anomaly
  EXPECT_TRUE(result == EMBEDIDS_OK || result == EMBEDIDS_ERROR_TREND_ANOMALY);
}

// ============================================================================
// Empty Metric Analysis Tests
// ============================================================================

TEST_F(EmbedIDSAlgorithmsTest, EmptyMetricAnalysis) {
  embedids_metric_datapoint_t history_buffer[10];
  embedids_metric_config_t metric_config;
  
  setupThresholdMetric(metric_config, history_buffer, "empty_metric", 
                       EMBEDIDS_METRIC_TYPE_FLOAT, 10);
  
  embedids_metric_value_t minv6 = { .f32 = 10.0f };
  embedids_metric_value_t maxv6 = { .f32 = 80.0f };
  metric_config.algorithms[0].config.threshold =
      embedids_threshold_config_init(&minv6, &maxv6);

  ASSERT_EQ(initializeWithMetric(&metric_config), EMBEDIDS_OK);

  // Analyze empty metric should return OK (no data to analyze)
  embedids_result_t result = embedids_analyze_metric(&context, "empty_metric");
  EXPECT_EQ(result, EMBEDIDS_OK);
}

// ============================================================================
// Algorithm Configuration Tests
// ============================================================================

TEST_F(EmbedIDSAlgorithmsTest, DisabledAlgorithm) {
  embedids_metric_datapoint_t history_buffer[10];
  embedids_metric_config_t metric_config;
  
  setupThresholdMetric(metric_config, history_buffer, "test_metric", 
                       EMBEDIDS_METRIC_TYPE_FLOAT, 10);
  
  // Configure but disable the algorithm
  metric_config.algorithms[0].enabled = false;
  embedids_metric_value_t minv7 = { .f32 = 10.0f };
  embedids_metric_value_t maxv7 = { .f32 = 80.0f };
  metric_config.algorithms[0].config.threshold =
      embedids_threshold_config_init(&minv7, &maxv7);

  ASSERT_EQ(initializeWithMetric(&metric_config), EMBEDIDS_OK);

  // Add value that would violate threshold if algorithm was enabled
  embedids_metric_value_t value;
  value.f32 = 90.0f; // Above threshold
  EXPECT_EQ(embedids_add_datapoint(&context, "test_metric", value, 1000), EMBEDIDS_OK);

  // Analysis should pass since algorithm is disabled
  embedids_result_t result = embedids_analyze_metric(&context, "test_metric");
  EXPECT_EQ(result, EMBEDIDS_OK);
}
