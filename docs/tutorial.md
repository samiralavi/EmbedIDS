# Tutorial

## Introduction

EmbedIDS is a lightweight, extensible intrusion detection system designed for embedded devices and IoT systems. It provides real-time monitoring of system metrics with configurable detection algorithms.

### Key Features
- **User-managed memory**: No dynamic allocation, perfect for embedded systems
- **Stateless design**: Context-based API for thread safety and multiple instances
- **Extensible architecture**: Custom metrics and algorithms
- **Multiple detection methods**: Thresholds, trends, statistical analysis, and custom algorithms
- **Minimal footprint**: Optimized for resource-constrained environments
- **Real-time analysis**: Low-latency threat detection

## API Design

### Stateless Architecture

EmbedIDS uses a stateless design where all library state is managed through a user-provided context object (`embedids_context_t`). This design provides several benefits:

- **Thread Safety**: Multiple contexts can be used independently across threads
- **Multiple Instances**: Create separate EmbedIDS instances for different subsystems
- **Clear Ownership**: Users control the lifecycle of the context object
- **Embedded-Friendly**: No global state that could cause issues in embedded systems

```c
// Create and initialize a context
embedids_context_t context;
```c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "embedids.h"

// Simulated sensor data helpers
static float get_cpu_usage(void)    { return 20.0f + (rand() % 60); }
static float get_memory_usage(void) { return 30.0f + (rand() % 50); }
static float get_network_packets(void) { return 100.0f + (rand() % 500); }

int main(void) {
    srand((unsigned int)time(NULL));

    // User memory (history + algorithms per metric)
    static embedids_metric_datapoint_t cpu_history[50];
    static embedids_metric_datapoint_t memory_history[50];
    static embedids_metric_datapoint_t network_history[30];

    static embedids_algorithm_t cpu_algorithms[1];
    static embedids_algorithm_t memory_algorithms[1];
    static embedids_algorithm_t network_algorithms[1];

    // Configure algorithms
    memset(cpu_algorithms, 0, sizeof(cpu_algorithms));
    embedids_algorithm_init(&cpu_algorithms[0], EMBEDIDS_ALGORITHM_THRESHOLD, true);
    cpu_algorithms[0].config.threshold.max_threshold.f32 = 75.0f;
    cpu_algorithms[0].config.threshold.check_max = true;

    memset(memory_algorithms, 0, sizeof(memory_algorithms));
    embedids_algorithm_init(&memory_algorithms[0], EMBEDIDS_ALGORITHM_TREND, true);
    memory_algorithms[0].config.trend.window_size = 5;
    memory_algorithms[0].config.trend.max_slope = 10.0f;

    memset(network_algorithms, 0, sizeof(network_algorithms));
    embedids_algorithm_init(&network_algorithms[0], EMBEDIDS_ALGORITHM_THRESHOLD, true);
    network_algorithms[0].config.threshold.max_threshold.f32 = 500.0f;
    network_algorithms[0].config.threshold.check_max = true;

    // Metric configs
    embedids_metric_config_t cpu_config; memset(&cpu_config, 0, sizeof(cpu_config));
    embedids_metric_config_t memory_config; memset(&memory_config, 0, sizeof(memory_config));
    embedids_metric_config_t network_config; memset(&network_config, 0, sizeof(network_config));

    embedids_metric_init(&cpu_config, "cpu_usage", EMBEDIDS_METRIC_TYPE_PERCENTAGE,
                         cpu_history, (uint32_t)(sizeof(cpu_history)/sizeof(cpu_history[0])),
                         cpu_algorithms, (uint32_t)(sizeof(cpu_algorithms)/sizeof(cpu_algorithms[0])));
    cpu_config.num_algorithms = 1;

    embedids_metric_init(&memory_config, "memory_usage", EMBEDIDS_METRIC_TYPE_PERCENTAGE,
                         memory_history, (uint32_t)(sizeof(memory_history)/sizeof(memory_history[0])),
                         memory_algorithms, (uint32_t)(sizeof(memory_algorithms)/sizeof(memory_algorithms[0])));
    memory_config.num_algorithms = 1;

    embedids_metric_init(&network_config, "network_packets", EMBEDIDS_METRIC_TYPE_FLOAT,
                         network_history, (uint32_t)(sizeof(network_history)/sizeof(network_history[0])),
                         network_algorithms, (uint32_t)(sizeof(network_algorithms)/sizeof(network_algorithms[0])));
    network_config.num_algorithms = 1;

    // System configuration array
    embedids_metric_config_t metrics[3] = { cpu_config, memory_config, network_config };

    embedids_system_config_t system_config; memset(&system_config, 0, sizeof(system_config));
    system_config.metrics = metrics;
    system_config.num_active_metrics = 3;

    embedids_context_t context; memset(&context, 0, sizeof(context));
    if (embedids_init(&context, &system_config) != EMBEDIDS_OK) {
        printf("Init failed\n");
        return 1;
    }

    printf("Multi-metric monitoring started:\n");
    printf("- CPU: Threshold (75%%)\n");
    printf("- Memory: Trend (window=5, max slope=10)\n");
    printf("- Network: Threshold (500 pkt/s)\n\n");

    for (int i = 0; i < 15; i++) {
        float cpu = get_cpu_usage();
        float memory = get_memory_usage();
        float network = get_network_packets();

        uint64_t ts = (uint64_t)time(NULL) * 1000ULL + (uint64_t)i * 100ULL;
        embedids_add_datapoint(&context, "cpu_usage", (embedids_metric_value_t){ .f32 = cpu }, ts);
        embedids_add_datapoint(&context, "memory_usage", (embedids_metric_value_t){ .f32 = memory }, ts);
        embedids_add_datapoint(&context, "network_packets", (embedids_metric_value_t){ .f32 = network }, ts);

        embedids_result_t overall = embedids_analyze_all(&context);
        printf("Iter %2d: CPU=%4.1f%% Mem=%4.1f%% Net=%5.0f pkt/s %s\n", i+1, cpu, memory, network,
               (overall == EMBEDIDS_OK) ? "✅ SECURE" : "🚨 THREAT");
        usleep(200000);
    }

    embedids_cleanup(&context);
    return 0;
}
```
### Multiple Metrics with Different Algorithms

```c
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "embedids.h"

// Function to simulate realistic sensor data
float get_cpu_usage() {
    return 20.0f + (rand() % 60);  // 20-80%
}

float get_memory_usage() {
    return 30.0f + (rand() % 50);  // 30-80%
}

float get_network_packets() {
    return 100.0f + (rand() % 500);  // 100-600 packets/s
}

    embedids_context_t context;
    memset(&context, 0, sizeof(context));
    
    embedids_result_t result = embedids_init(&context, &system_config);
    if (result != EMBEDIDS_OK) {
        printf("Failed to initialize EmbedIDS: %d\n", result);
        return 1;
    }
    
    printf("Multi-metric monitoring started:\n");
    printf("- CPU: Threshold (75%%)\n");
    printf("- Memory: Trend analysis (5 window, 10%% max slope)\n");
    printf("- Network: Threshold (500 pkt/s)\n\n");
    
    // Monitoring loop
    for (int i = 0; i < 15; i++) {
        // Get sensor data
        float cpu = get_cpu_usage();
        float memory = get_memory_usage();
        float network = get_network_packets();
        
        // Add data points
        uint64_t timestamp = (uint64_t)time(NULL) * 1000 + i * 100;
        
        embedids_metric_value_t cpu_val = {.f32 = cpu};
        embedids_metric_value_t memory_val = {.f32 = memory};
        embedids_metric_value_t network_val = {.f32 = network};
        
        embedids_add_datapoint(&context, "cpu_usage", cpu_val, timestamp);
        embedids_add_datapoint(&context, "memory_usage", memory_val, timestamp);
        embedids_add_datapoint(&context, "network_packets", network_val, timestamp);
        
        // Analyze all metrics
        embedids_result_t overall_result = embedids_analyze_all(&context);
        
        printf("Iteration %2d: CPU=%4.1f%%, Memory=%4.1f%%, Network=%5.0f pkt/s ", 
               i+1, cpu, memory, network);
        
        if (overall_result == EMBEDIDS_OK) {
            printf("✅ SECURE\n");
        } else {
            printf("🚨 THREAT DETECTED\n");
        }
        
        usleep(200000);  // 200ms delay
    }
    
    embedids_cleanup(&context);
    return 0;
}
```

## Custom Algorithms

### Creating a Custom Detection Algorithm

Custom algorithms allow you to implement specialized detection logic:

```c
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include "embedids.h"

// Custom algorithm context
typedef struct {
    float baseline;
    uint32_t violation_count;
    uint32_t max_violations;
} anomaly_detector_context_t;

// Custom algorithm implementation
embedids_result_t anomaly_detector(const embedids_metric_t* metric,
                                 const void* config,
                                 void* context) {
    (void)config;  // Unused in this example
    
    anomaly_detector_context_t* ctx = (anomaly_detector_context_t*)context;
    
    if (metric->current_size < 3) {
        return EMBEDIDS_OK;  // Need at least 3 data points
    }
    
    // Get recent values
    uint32_t idx1 = (metric->write_index - 1) % metric->max_history_size;
    uint32_t idx2 = (metric->write_index - 2) % metric->max_history_size;
    uint32_t idx3 = (metric->write_index - 3) % metric->max_history_size;
    
    float val1 = metric->history[idx1].value.f32;
    float val2 = metric->history[idx2].value.f32;
    float val3 = metric->history[idx3].value.f32;
    
    // Calculate moving average
    float avg = (val1 + val2 + val3) / 3.0f;
    
    // Check deviation from baseline
    float deviation = fabs(avg - ctx->baseline);
    float threshold = ctx->baseline * 0.5f;  // 50% deviation threshold
    
    if (deviation > threshold) {
        ctx->violation_count++;
        printf("  🔍 Anomaly detector: deviation %.2f from baseline %.2f (count: %d)\n",
               deviation, ctx->baseline, ctx->violation_count);
        
        if (ctx->violation_count >= ctx->max_violations) {
            printf("  🚨 ANOMALY ALERT: Sustained deviation detected!\n");
            ctx->violation_count = 0;  // Reset counter
            return EMBEDIDS_ERROR_THRESHOLD_EXCEEDED;
        }
    } else {
        ctx->violation_count = 0;  // Reset on normal reading
    }
    
    return EMBEDIDS_OK;
}

int main(void) {
    static embedids_metric_datapoint_t sensor_history[40];
    static embedids_algorithm_t algorithms[2];

    anomaly_detector_context_t detector_ctx = { .baseline = 50.0f, .violation_count = 0, .max_violations = 3 };

    // Custom algorithm slot
    memset(&algorithms[0], 0, sizeof(algorithms[0]));
    embedids_algorithm_init(&algorithms[0], EMBEDIDS_ALGORITHM_CUSTOM, true);
    algorithms[0].config.custom.function = anomaly_detector;
    algorithms[0].config.custom.config = NULL;
    algorithms[0].config.custom.context = &detector_ctx;

    // Threshold backup
    memset(&algorithms[1], 0, sizeof(algorithms[1]));
    embedids_algorithm_init(&algorithms[1], EMBEDIDS_ALGORITHM_THRESHOLD, true);
    algorithms[1].config.threshold.max_threshold.f32 = 100.0f;
    algorithms[1].config.threshold.min_threshold.f32 = 0.0f;
    algorithms[1].config.threshold.check_max = true;
    algorithms[1].config.threshold.check_min = true;

    embedids_metric_config_t metric_config; memset(&metric_config, 0, sizeof(metric_config));
    embedids_metric_init(&metric_config, "sensor_data", EMBEDIDS_METRIC_TYPE_FLOAT,
                         sensor_history, (uint32_t)(sizeof(sensor_history)/sizeof(sensor_history[0])),
                         algorithms, (uint32_t)(sizeof(algorithms)/sizeof(algorithms[0])));
    metric_config.num_algorithms = 2;

    embedids_system_config_t system_config; memset(&system_config, 0, sizeof(system_config));
    system_config.metrics = &metric_config;
    system_config.num_active_metrics = 1;

    embedids_context_t context; memset(&context, 0, sizeof(context));
    if (embedids_init(&context, &system_config) != EMBEDIDS_OK) {
        printf("Init failed\n");
        return 1;
    }

    printf("Custom algorithm demo started (baseline %.1f)\n", detector_ctx.baseline);
    printf("Algorithms: Custom anomaly + Threshold (0-100)\n\n");

    float sensor_values[] = { 48.0,52.0,49.0,51.0,47.0, 85.0,88.0,90.0, 45.0,50.0,48.0, 15.0,12.0,18.0, 52.0,49.0,51.0 };
    int num_values = (int)(sizeof(sensor_values)/sizeof(sensor_values[0]));
    for (int i = 0; i < num_values; i++) {
        float value = sensor_values[i];
        embedids_metric_value_t mv = { .f32 = value };
        uint64_t ts = (uint64_t)time(NULL) * 1000ULL + (uint64_t)i * 100ULL;
        embedids_add_datapoint(&context, "sensor_data", mv, ts);
        embedids_result_t r = embedids_analyze_metric(&context, "sensor_data");
        printf("Sample %2d: Value=%.1f %s\n", i+1, value, (r == EMBEDIDS_OK) ? "✅ NORMAL" : "🚨 ANOMALY");
        usleep(300000);
    }

    embedids_cleanup(&context);
    return 0;
}
```

## Best Practices

### 1. Memory Management

```c
// ✅ Good: Static allocation for embedded systems
static embedids_metric_datapoint_t history[100];

// ❌ Avoid: Dynamic allocation in embedded systems
// embedids_metric_datapoint_t* history = malloc(sizeof(embedids_metric_datapoint_t) * 100);
```

### 2. Error Handling

```c
// ✅ Always check return codes
embedids_result_t result = embedids_add_datapoint(&context, "metric", value, timestamp);
if (result != EMBEDIDS_OK) {
    handle_error(result);
}

// ✅ Handle specific error types
switch (result) {
    case EMBEDIDS_ERROR_NOT_INITIALIZED:
        reinitialize_system();
        break;
    case EMBEDIDS_ERROR_METRIC_NOT_FOUND:
        log_error("Unknown metric");
        break;
    case EMBEDIDS_ERROR_BUFFER_FULL:
        // This is expected behavior with circular buffers
        break;
    default:
        handle_generic_error(result);
}
```

### 3. Metric Configuration

```c
// ✅ Use appropriate history sizes
embedids_metric_t fast_metric = {
    .max_history_size = 20,  // For high-frequency data
    // ...
};

embedids_metric_t slow_metric = {
    .max_history_size = 100, // For trend analysis
    // ...
};

// ✅ Choose appropriate thresholds
threshold_config.max_threshold.f32 = 85.0f;  // 85% CPU threshold
threshold_config.min_threshold.f32 = 5.0f;   // Minimum activity level
```

### 4. Performance Optimization

```c
// ✅ Batch operations when possible
for (int i = 0; i < num_sensors; i++) {
    embedids_add_datapoint(&context, sensor_names[i], values[i], timestamp);
}
// Analyze all at once
embedids_analyze_all(&context);

// ✅ Use appropriate data types
embedids_metric_value_t percentage_val = {.f32 = 75.5f};  // For percentages
embedids_metric_value_t count_val = {.u32 = 1024};        // For counts
```

## Troubleshooting

### Common Issues and Solutions

#### 1. Initialization Fails
```c
embedids_result_t result = embedids_init(&context, &config);
if (result == EMBEDIDS_ERROR_INVALID_PARAM) {
    // Check: config pointer is not NULL
    // Check: metric configurations are valid
    // Check: algorithm configurations are correct
}
```

#### 2. Metric Not Found
```c
result = embedids_add_datapoint(&context, "cpu_usage", value, timestamp);
if (result == EMBEDIDS_ERROR_METRIC_NOT_FOUND) {
    // Verify metric name matches exactly (case-sensitive)
    // Ensure metric was added to system configuration
    // Check metric is enabled
}
```

#### 3. Memory Issues
```c
// Ensure history arrays are large enough
static embedids_metric_datapoint_t history[SIZE];
if (SIZE < expected_data_points) {
    // Increase SIZE or reduce data retention period
}
```

#### 4. Algorithm Not Triggering
```c
// Check algorithm configuration
threshold_algo.enabled = true;  // Must be enabled
threshold_algo.config.threshold.check_max = true;  // Must enable checking

// Verify threshold values
printf("Threshold: %.2f, Current: %.2f\n", 
       threshold_config.max_threshold.f32, current_value);
```

### Debug Information

```c
// Check system status
if (embedids_is_initialized()) {
    printf("EmbedIDS is running\n");
} else {
    printf("EmbedIDS not initialized\n");
}

// Get version info
printf("EmbedIDS version: %s\n", embedids_get_version());

// Monitor metric states
for (int i = 0; i < num_metrics; i++) {
    printf("Metric %s: %d/%d data points\n", 
           metric_names[i], 
           metrics[i].current_size, 
           metrics[i].max_history_size);
}
```

### Performance Monitoring

```c
#include <time.h>

// Measure analysis performance
clock_t start = clock();
embedids_result_t result = embedids_analyze_all(&context);
clock_t end = clock();

double cpu_time = ((double)(end - start)) / CLOCKS_PER_SEC;
printf("Analysis took %.3f ms\n", cpu_time * 1000);
```

## Conclusion

### Migration from Older API Versions

Earlier versions required: 
- Fixed compile-time limits (EMBEDIDS_MAX_METRICS, EMBEDIDS_MAX_METRIC_NAME_LEN, EMBEDIDS_MAX_ALGORITHMS_PER_METRIC)
- Direct struct field population (copying names into fixed char arrays)
- Implicit fixed-size algorithm arrays inside each metric config

These have been removed in favor of a more flexible, user-supplied memory model:
1. Provide your own history buffers (as before)
2. Provide an algorithms array sized for the algorithms you intend to attach
3. Use embedids_metric_init(...) to correctly initialize each metric configuration
4. Set metric_config.num_algorithms after populating algorithm entries

Benefits:
- No rebuild needed for different sizing
- Clear ownership of memory
- Smaller binary if many features unused
- Easier to extend / test

New helper added:
- embedids_algorithm_init(&algo, TYPE, enabled) simplifies preparing algorithm slots before setting detailed configuration fields (threshold values, trend parameters, or custom function/context).

Minimal migration example (old → new):
```c
// OLD (simplified)
embedids_metric_config_t old_cfg; memset(&old_cfg,0,sizeof(old_cfg));
// Previously: copy into fixed buffer and rely on internal fixed algorithm array
// strncpy(old_cfg.metric.name, "temp", EMBEDIDS_MAX_METRIC_NAME_LEN-1);
old_cfg.metric.name = "temp"; // (if pointer field was not available before, this is conceptual)
old_cfg.metric.history = hist; old_cfg.metric.max_history_size = HIST_SZ; old_cfg.metric.enabled = true;
old_cfg.algorithms[0] = algo; old_cfg.num_algorithms = 1;

// NEW
static embedids_algorithm_t algos[1]; algos[0] = algo; // configure first
embedids_metric_config_t new_cfg; memset(&new_cfg,0,sizeof(new_cfg));
embedids_metric_init(&new_cfg, "temp", EMBEDIDS_METRIC_TYPE_FLOAT, hist, HIST_SZ, algos, 1);
new_cfg.num_algorithms = 1; // after filling algos
```

If you previously relied on the maximum metric count macro, simply size your metrics array explicitly and set system_config.num_active_metrics accordingly.

If you find any leftover references to removed macros in your code, they can be safely deleted—there is no equivalent replacement required.

EmbedIDS provides a flexible, efficient intrusion detection framework for embedded systems. Key takeaways:

1. **Start simple** with basic threshold monitoring
2. **Use user-managed memory** for predictable resource usage
3. **Combine multiple algorithms** for comprehensive detection
4. **Implement custom algorithms** for specialized requirements
5. **Follow embedded best practices** for reliable operation

For more examples, see the `examples/` directory in the EmbedIDS repository.

---

**Next Steps:**
- Try the provided examples
- Implement custom algorithms for your use case
- Integrate with your existing monitoring infrastructure
- Explore advanced configuration options

