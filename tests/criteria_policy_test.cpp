// Copyright (c) 2026 Andrew J. Sender.
// All rights reserved.

#include "criteria_policy.hpp"

#include <gtest/gtest.h>

namespace {

using gtest_criteria::ClassifyStability;
using gtest_criteria::CriteriaOptions;
using gtest_criteria::IsPerformanceDeviation;
using gtest_criteria::StabilityDisposition;
using gtest_criteria::ValidateCriteriaOptions;

TEST(CriteriaOptionsTest, RejectsInvalidHistoryConfiguration) {
    CriteriaOptions options;
    options.minimum_history_count = 0;
    EXPECT_THROW(ValidateCriteriaOptions(options), std::invalid_argument);

    options.minimum_history_count = 10;
    options.history_limit = 9;
    EXPECT_THROW(ValidateCriteriaOptions(options), std::invalid_argument);
}

TEST(PerformancePolicyTest, RequiresMinimumHistory) {
    CriteriaOptions options;
    options.minimum_history_count = 5;

    EXPECT_FALSE(IsPerformanceDeviation(200.0, 100.0, 1.0, 4, options));
    EXPECT_TRUE(IsPerformanceDeviation(200.0, 100.0, 1.0, 5, options));
}

TEST(PerformancePolicyTest, AppliesNoiseFloorAndStandardDeviationThreshold) {
    CriteriaOptions options;
    options.minimum_history_count = 1;
    options.performance_min_difference_ms = 10.0;
    options.performance_stddev_threshold = 3.0;

    EXPECT_FALSE(IsPerformanceDeviation(109.0, 100.0, 1.0, 1, options));
    EXPECT_FALSE(IsPerformanceDeviation(102.0, 100.0, 1.0, 1, options));
    EXPECT_TRUE(IsPerformanceDeviation(110.0, 100.0, 1.0, 1, options));
}

TEST(StabilityPolicyTest, ClassifiesDeterministically) {
    CriteriaOptions options;
    options.minimum_history_count = 5;

    EXPECT_EQ(ClassifyStability(0.01, 4, 0.05, options),
              StabilityDisposition::InsufficientHistory);
    EXPECT_EQ(ClassifyStability(0.01, 5, 0.05, options),
              StabilityDisposition::Skip);
    EXPECT_EQ(ClassifyStability(0.05, 5, 0.05, options),
              StabilityDisposition::Fail);
}

TEST(StabilityPolicyTest, RejectsInvalidRatesAndThresholds) {
    CriteriaOptions options;

    EXPECT_THROW(ClassifyStability(-0.1, 5, 0.05, options),
                 std::invalid_argument);
    EXPECT_THROW(ClassifyStability(0.1, 5, 1.0, options),
                 std::invalid_argument);
}

}  // namespace
