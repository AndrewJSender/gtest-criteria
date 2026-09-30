-- Copyright (c) 2026 Andrew J. Sender.
-- All rights reserved.

-- select_logged_value
SELECT part.message
FROM test_result_part AS part
JOIN test ON part.test_id = test.id
JOIN suite ON test.suite_id = suite.id
WHERE suite.name = ? AND test.name = ? AND part.type = 'success' AND part.message LIKE ?
ORDER BY part.id ASC LIMIT 1;

-- select_test_durations
SELECT AVG(test.end_timestamp - test.start_timestamp),
AVG((test.end_timestamp - test.start_timestamp) *
(test.end_timestamp - test.start_timestamp)), COUNT(*)
FROM test JOIN suite ON test.suite_id = suite.id
WHERE suite.name = ? AND test.name = ? AND test.result = 'passed'
AND test.start_timestamp IS NOT NULL AND test.end_timestamp IS NOT NULL
LIMIT 100;
