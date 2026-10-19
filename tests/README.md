# Verification Test Cases

These test cases summarize the scenarios recorded in the project report. They are manual target-level tests; no automated CI test or fresh QNX run is included here.

| Case | Selected blocks | Expected status | Expected mask |
|---|---|---|---:|
| Intact image | None | VALID | 0 |
| Four middle blocks | 3, 6, 9, 12 | CORRUPTED | 4680 |
| First and last blocks | 0, 5, 10, 15 | CORRUPTED | 33825 |

## Manual test procedure

1. Build the client and server with the QNX toolchain.
2. Copy both executables to the QNX target and start the server before launching the GUI.
3. Configure the ground station connection using environment variables.
4. Run the first case without selecting any blocks.
5. Select the listed blocks for the next cases and deploy again.
6. Confirm the status, decimal mask and red/green result cells.
7. Keep a screenshot of each result with the project documentation.

## Mask calculation

The mask sets bit i for each selected test block i. For blocks 3, 6, 9 and 12:

~~~text
(1 << 3) + (1 << 6) + (1 << 9) + (1 << 12) = 4680
~~~

For blocks 0, 5, 10 and 15:

~~~text
(1 << 0) + (1 << 5) + (1 << 10) + (1 << 15) = 33825
~~~

The current implementation detects the test marker only at the first byte of each 4 KiB block. This test plan does not imply arbitrary corruption offsets are localized correctly.
