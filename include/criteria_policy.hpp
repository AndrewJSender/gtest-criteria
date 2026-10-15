// Copyright (c) 2026 Andrew J. Sender.
// All rights reserved.

#ifndef GTEST_CRITERIA_POLICY_HPP
#define GTEST_CRITERIA_POLICY_HPP

#include <cstddef>

namespace gtest_criteria {

struct CriteriaOptions {
    std::size_t minimum_history_count = 5;
    std::size_t history_limit = 100;
    double performance_stddev_threshold = 3.0;
    double performance_min_difference_ms = 10.0;
};

enum class StabilityDisposition {
    InsufficientHistory,
    Skip,
    Fail,
};

void ValidateCriteriaOptions(const CriteriaOptions& options);

bool IsPerformanceDeviation(double duration_ms, double historical_mean_ms,
                            double historical_stddev_ms,
                            std::size_t history_count,
                            const CriteriaOptions& options);

StabilityDisposition ClassifyStability(double failure_or_skip_rate,
                                       std::size_t history_count,
                                       double stable_threshold,
                                       const CriteriaOptions& options);

}  // namespace gtest_criteria

#endif  // GTEST_CRITERIA_POLICY_HPP
