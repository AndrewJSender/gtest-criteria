// Copyright (c) 2026 Andrew J. Sender.
// All rights reserved.

#include "criteria_policy.hpp"

#include <cmath>
#include <stdexcept>

namespace gtest_criteria {

void ValidateCriteriaOptions(const CriteriaOptions& options) {
    if (options.minimum_history_count == 0) {
        throw std::invalid_argument("minimum_history_count must be greater than zero");
    }
    if (options.history_limit < options.minimum_history_count) {
        throw std::invalid_argument(
            "history_limit must be at least minimum_history_count");
    }
    if (options.performance_stddev_threshold <= 0.0) {
        throw std::invalid_argument(
            "performance_stddev_threshold must be greater than zero");
    }
    if (options.performance_min_difference_ms < 0.0) {
        throw std::invalid_argument(
            "performance_min_difference_ms must not be negative");
    }
}

bool IsPerformanceDeviation(double duration_ms, double historical_mean_ms,
                            double historical_stddev_ms,
                            std::size_t history_count,
                            const CriteriaOptions& options) {
    ValidateCriteriaOptions(options);
    if (history_count < options.minimum_history_count) {
        return false;
    }
    const double difference = std::abs(duration_ms - historical_mean_ms);
    if (difference < options.performance_min_difference_ms) {
        return false;
    }
    return difference >
        options.performance_stddev_threshold * historical_stddev_ms;
}

StabilityDisposition ClassifyStability(double failure_or_skip_rate,
                                       std::size_t history_count,
                                       double stable_threshold,
                                       const CriteriaOptions& options) {
    ValidateCriteriaOptions(options);
    if (stable_threshold <= 0.0 || stable_threshold >= 1.0) {
        throw std::invalid_argument("stable_threshold must be between zero and one");
    }
    if (failure_or_skip_rate < 0.0 || failure_or_skip_rate > 1.0) {
        throw std::invalid_argument(
            "failure_or_skip_rate must be between zero and one");
    }
    if (history_count < options.minimum_history_count) {
        return StabilityDisposition::InsufficientHistory;
    }
    return failure_or_skip_rate < stable_threshold
        ? StabilityDisposition::Skip
        : StabilityDisposition::Fail;
}

}  // namespace gtest_criteria
