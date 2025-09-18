#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <embedids.h>

/**
 * @brief Simple example demonstrating EmbedIDS usage with modern API
 */

#define MAX_HISTORY_SIZE 10

// Static buffers for metrics (user-managed memory)
static embedids_metric_datapoint_t cpu_history[MAX_HISTORY_SIZE];
static embedids_metric_datapoint_t memory_history[MAX_HISTORY_SIZE];
static embedids_metric_datapoint_t network_history[MAX_HISTORY_SIZE];

int main(void) {
    printf("EmbedIDS Example v%s\n", embedids_get_version());
    printf("====================\n\n");

    // Define metrics with thresholds using new extensible API
    embedids_metric_config_t metrics[3];
    embedids_algorithm_t cpu_algorithms[1];
    embedids_algorithm_t mem_algorithms[1];
    embedids_algorithm_t net_algorithms[1];

    embedids_metric_init(&metrics[0], "cpu_usage", EMBEDIDS_METRIC_TYPE_PERCENTAGE,
                         cpu_history, MAX_HISTORY_SIZE, cpu_algorithms, 1);
    metrics[0].num_algorithms = 1;
    metrics[0].algorithms[0] = (embedids_algorithm_t){
        .type = EMBEDIDS_ALGORITHM_THRESHOLD,
        .enabled = true,
        .config.threshold = {
            .max_threshold = { .f32 = 75.0f },
            .check_max = true,
            .check_min = false
        }
    };

    embedids_metric_init(&metrics[1], "memory_usage", EMBEDIDS_METRIC_TYPE_UINT32,
                         memory_history, MAX_HISTORY_SIZE, mem_algorithms, 1);
    metrics[1].num_algorithms = 1;
    metrics[1].algorithms[0] = (embedids_algorithm_t){
        .type = EMBEDIDS_ALGORITHM_THRESHOLD,
        .enabled = true,
        .config.threshold = {
            .max_threshold = { .u32 = 512000 },
            .check_max = true,
            .check_min = false
        }
    };

    embedids_metric_init(&metrics[2], "network_packets", EMBEDIDS_METRIC_TYPE_RATE,
                         network_history, MAX_HISTORY_SIZE, net_algorithms, 1);
    metrics[2].num_algorithms = 1;
    metrics[2].algorithms[0] = (embedids_algorithm_t){
        .type = EMBEDIDS_ALGORITHM_THRESHOLD,
        .enabled = true,
        .config.threshold = {
            .max_threshold = { .u32 = 800 },
            .check_max = true,
            .check_min = false
        }
    };

    // Initialize the intrusion detection system
    embedids_context_t context;
    memset(&context, 0, sizeof(context));
    
    embedids_system_config_t system_config = {
        .metrics = metrics,
        .num_active_metrics = 3,
        .user_context = NULL
    };
    
    embedids_result_t result = embedids_init(&context, &system_config);
    if (result != EMBEDIDS_OK) {
        printf("Error: Failed to initialize EmbedIDS (code: %d)\n", result);
        return EXIT_FAILURE;
    }

    printf("EmbedIDS initialized successfully!\n");
    printf("Monitoring system with 3 metrics:\n");
    printf("  - CPU Usage: max 75%%\n");
    printf("  - Memory Usage: max 500 MB\n");
    printf("  - Network Packets: max 800/sec\n");
    printf("\nStarting monitoring simulation...\n\n");

    // Simulate monitoring for 10 iterations
    for (int i = 0; i < 10; i++) {
        uint64_t timestamp = (uint64_t)time(NULL) * 1000 + i * 1000;
        
        // Simulate CPU usage
        embedids_metric_value_t cpu_value = { .f32 = (float)(rand() % 100) };
        result = embedids_add_datapoint(&context, "cpu_usage", cpu_value, timestamp);
        if (result != EMBEDIDS_OK) {
            printf("Error adding CPU datapoint: %d\n", result);
        }
        
        // Simulate memory usage
        embedids_metric_value_t memory_value = { .u32 = 200000 + (rand() % 600000) };
        result = embedids_add_datapoint(&context, "memory_usage", memory_value, timestamp);
        if (result != EMBEDIDS_OK) {
            printf("Error adding memory datapoint: %d\n", result);
        }
        
        // Simulate network packets
        embedids_metric_value_t network_value = { .u32 = 100 + (rand() % 900) };
        result = embedids_add_datapoint(&context, "network_packets", network_value, timestamp);
        if (result != EMBEDIDS_OK) {
            printf("Error adding network datapoint: %d\n", result);
        }

        printf("Iteration %d:\n", i + 1);
        printf("  CPU: %.0f%%, Memory: %u KB, Network: %u pkt/s\n",
               cpu_value.f32, memory_value.u32, network_value.u32);

        // Analyze all metrics for potential intrusions
        result = embedids_analyze_all(&context);
        switch (result) {
            case EMBEDIDS_OK:
                printf("  Status: NORMAL - All metrics within acceptable ranges\n");
                break;
            case EMBEDIDS_ERROR_THRESHOLD_EXCEEDED:
                printf("  Status: ALERT - Potential intrusion detected! Threshold exceeded.\n");
                break;
            default:
                printf("  Status: ERROR - Analysis failed (code: %d)\n", result);
                break;
        }

        printf("\n");
        
        // Sleep for 1 second between iterations
        sleep(1);
    }

    printf("Monitoring simulation completed.\n");
    
    // Cleanup
    embedids_cleanup(&context);
    printf("EmbedIDS cleaned up successfully.\n");

    return EXIT_SUCCESS;
}
