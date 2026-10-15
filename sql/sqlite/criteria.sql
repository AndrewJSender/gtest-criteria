-- Copyright (c) 2026 Andrew J. Sender.
-- All rights reserved.

-- select_logged_value_list
SELECT part.message
FROM test_result_part AS part
JOIN test ON part.test_id = test.id
JOIN suite ON test.suite_id = suite.id
WHERE suite.name = ? AND test.name = ? AND part.type = 'success' AND part.message LIKE ?
ORDER BY part.id ASC;

-- select_test_durations
SELECT AVG(duration), AVG(duration * duration), COUNT(*)
FROM (
  SELECT test.end_timestamp - test.start_timestamp AS duration
  FROM test JOIN suite ON test.suite_id = suite.id
  WHERE suite.name = ? AND test.name = ? AND test.result = 'passed'
  AND test.start_timestamp IS NOT NULL AND test.end_timestamp IS NOT NULL
  ORDER BY test.id DESC LIMIT ?
) AS recent_durations;

-- select_test_failure_or_skip_rate
SELECT
  COUNT(*) FILTER (WHERE result = 'failed' OR result = 'skipped') * 1.0 /
    NULLIF(COUNT(*), 0),
  COUNT(*)
FROM (
  SELECT test.result
  FROM test
  JOIN suite ON test.suite_id = suite.id
  WHERE suite.name = ? AND test.name = ?
    AND test.result IN ('failed', 'skipped', 'passed')
  ORDER BY test.id DESC LIMIT ?
) AS recent_results;
