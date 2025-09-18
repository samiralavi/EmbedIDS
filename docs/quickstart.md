# Quick Start

Get up and running with EmbedIDS in 5 minutes!

## Installation

```bash
git clone https://github.com/samiralavi/EmbedIDS.git
cd EmbedIDS
mkdir build && cd build
cmake ..
make
```

## Hello World Example

Create `my_monitor.c`:

```c
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "embedids.h"

int main() {
    // 1. Allocate user memory
    static embedids_metric_datapoint_t cpu_history[50];
    static embedids_algorithm_t cpu_algorithms[1];

    // 2. Prepare algorithm (threshold > 80%) using new value-returning config helpers
    embedids_algorithm_init(&cpu_algorithms[0], EMBEDIDS_ALGORITHM_THRESHOLD, true);
    embedids_metric_value_t maxv = { .f32 = 80.0f };
    cpu_algorithms[0].config.threshold = embedids_threshold_config_init(NULL, &maxv);

    // 3. Initialize metric configuration with helper
    embedids_metric_config_t cpu_metric_config;
    memset(&cpu_metric_config, 0, sizeof(cpu_metric_config));
    embedids_result_t r = embedids_metric_init(
        &cpu_metric_config,
        "cpu_usage",
        EMBEDIDS_METRIC_TYPE_PERCENTAGE,
        cpu_history,
        (uint32_t)(sizeof(cpu_history)/sizeof(cpu_history[0])),
        cpu_algorithms,
        (uint32_t)(sizeof(cpu_algorithms)/sizeof(cpu_algorithms[0]))
    );
    if (r != EMBEDIDS_OK) {
        printf("Metric init failed: %d\n", r);
        return 1;
    }
    cpu_metric_config.num_algorithms = 1; // we populated first slot

    // 4. System configuration (now constructed via helper)
    embedids_system_config_t system_config =
        embedids_system_config_init(&cpu_metric_config, 1, NULL);

    // 5. Initialize context
    embedids_context_t context;
    memset(&context, 0, sizeof(context));
    if (embedids_init(&context, &system_config) != EMBEDIDS_OK) {
        printf("Failed to initialize EmbedIDS\n");
        return 1;
    }

    printf("🔒 CPU Monitor Started (threshold: 80%%)\n\n");

    // 6. Monitoring loop
    for (int i = 0; i < 10; i++) {
        float cpu = 30.0f + (i * 8.0f); // Simulated CPU usage
        embedids_metric_value_t value = {.f32 = cpu};
        embedids_add_datapoint(&context, "cpu_usage", value, (uint64_t)time(NULL) * 1000ULL);

        embedids_result_t ar = embedids_analyze_metric(&context, "cpu_usage");
        if (ar == EMBEDIDS_OK) {
            printf("✅ CPU: %.1f%% - Normal\n", cpu);
        } else if (ar == EMBEDIDS_ERROR_THRESHOLD_EXCEEDED) {
            printf("🚨 CPU: %.1f%% - ALERT (threshold)\n", cpu);
        } else {
            printf("⚠️ CPU: %.1f%% - Analysis error %d\n", cpu, ar);
        }
        sleep(1);
    }

    embedids_cleanup(&context);
    return 0;
}
```

## Compile and Run

```bash
# Adjust -I/-L paths based on your build output location
gcc -I../include -L. -lembedids -o my_monitor my_monitor.c
./my_monitor
```

## Expected Output

```
🔒 CPU Monitor Started (threshold: 80%)

✅ CPU: 30.0% - Normal
✅ CPU: 38.0% - Normal
✅ CPU: 46.0% - Normal
✅ CPU: 54.0% - Normal
✅ CPU: 62.0% - Normal
✅ CPU: 70.0% - Normal
✅ CPU: 78.0% - Normal
🚨 CPU: 86.0% - ALERT!
🚨 CPU: 94.0% - ALERT!
🚨 CPU: 102.0% - ALERT!
```

## What Just Happened?

1. **Created a metric**: CPU usage with 50-point history
2. **Added detection**: Threshold algorithm at 80% (using `embedids_threshold_config_init`)
3. **Monitored in real-time**: Added data points and analyzed
4. **Got alerts**: When CPU exceeded threshold

## Next Steps

- **Multiple metrics**: Monitor CPU, memory, network together
- **Custom algorithms**: Implement your own detection logic
- **Advanced features**: Trend analysis, pattern detection
- **Real sensors**: Replace simulated data with actual sensor readings

See the [full tutorial](tutorial.md) for comprehensive examples and advanced usage!

## Quick Reference

### Core Functions
```c
embedids_init(&context, &config)             // Initialize system
embedids_add_datapoint(&context, name, val, time) // Add sensor data
embedids_analyze_metric(&context, name)      // Check one metric
embedids_analyze_all(&context)               // Check all metrics
embedids_cleanup(&context)                   // Shutdown system
embedids_threshold_config_init(&min,&max)    // Create threshold config (NULL to skip one side)
embedids_trend_config_init(window,slope,var,expected) // Create trend config
embedids_system_config_init(metrics,count,user_ctx) // Create system config
```

### Algorithm Types
- `EMBEDIDS_ALGORITHM_THRESHOLD` - Simple min/max limits
- `EMBEDIDS_ALGORITHM_TREND` - Slope analysis over time
- `EMBEDIDS_ALGORITHM_CUSTOM` - Your own detection logic

### Return Codes
- `EMBEDIDS_OK` - All normal
- `EMBEDIDS_ERROR_THRESHOLD_EXCEEDED` - Threat detected
- `EMBEDIDS_ERROR_NOT_INITIALIZED` - Call init first
- `EMBEDIDS_ERROR_METRIC_NOT_FOUND` - Check metric name
