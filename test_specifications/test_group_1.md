# Test Group 1: Basic State Machine Transitions

This group tests fundamental state transitions without interruptions, focusing on core state machine behavior.

## Test 1.1: Simple Tap (IDLE → WAITING_FOR_HOLD → WAITING_FOR_TAP → IDLE)

**Objective**: Verify basic tap sequence with release before hold timeout

**Configuration**:
```cpp
TAP_DANCE_KEY = 3000
OUTPUT_KEY = 3001
Strategy: TAP_PREFERRED
Tap actions: [1: SENDKEY(OUTPUT_KEY)]
Hold actions: [1: CHANGELAYER(1)]
Hold timeout: 200ms, Tap timeout: 200ms
```

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
release_key(TAP_DANCE_KEY, 100); // t=100ms (before hold timeout)
// Wait for tap timeout to complete sequence
platform_wait_ms(200);          // t=300ms
```

**Expected Output**:
```cpp
{OUTPUT_KEY, PRESS, 300},   // Tap action executed at timeout
{OUTPUT_KEY, RELEASE, 300}
```

---

## Test 1.2: Simple Hold (IDLE → WAITING_FOR_HOLD → HOLDING → IDLE)

**Objective**: Verify basic hold sequence with timeout triggering hold action

**Configuration**: Same as Test 1.1

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
// Hold for longer than hold timeout
platform_wait_ms(250);          // t=250ms (exceeds hold timeout)
release_key(TAP_DANCE_KEY);      // t=250ms
```

**Expected Output**:
```cpp
{LAYER_1, ACTIVATE, 200},    // Hold action at timeout
{LAYER_1, DEACTIVATE, 250}   // Deactivate on release
```

---

## Test 1.3: Hold Timeout Boundary - Just Before

**Objective**: Verify tap behavior when released exactly at hold timeout boundary

**Configuration**: Same as Test 1.1

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
release_key(TAP_DANCE_KEY, 199); // t=199ms (1ms before timeout)
platform_wait_ms(200);          // Wait for tap timeout
```

**Expected Output**:
```cpp
{OUTPUT_KEY, PRESS, 399},
{OUTPUT_KEY, RELEASE, 399}
```

---

## Test 1.4: Hold Timeout Boundary - Exactly At

**Objective**: Verify hold behavior when timeout occurs exactly at boundary

**Configuration**: Same as Test 1.1

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
platform_wait_ms(200);          // t=200ms (exactly at timeout)
release_key(TAP_DANCE_KEY);      // t=200ms
```

**Expected Output**:
```cpp
{LAYER_1, ACTIVATE, 200},
{LAYER_1, DEACTIVATE, 200}
```

---

## Test 1.5: Hold Timeout Boundary - Just After

**Objective**: Verify hold behavior when held past timeout

**Configuration**: Same as Test 1.1

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
release_key(TAP_DANCE_KEY, 201); // t=201ms (1ms after timeout)
```

**Expected Output**:
```cpp
{LAYER_1, ACTIVATE, 200},
{LAYER_1, DEACTIVATE, 201}
```

---

## Test 1.6: Tap Timeout Boundary - Sequence Reset

**Objective**: Verify sequence resets when tap timeout expires

**Configuration**: Same as Test 1.1

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
release_key(TAP_DANCE_KEY, 100); // t=100ms
// Wait exactly for tap timeout
platform_wait_ms(200);          // t=300ms (tap timeout expires)
```

**Expected Output**:
```cpp
{OUTPUT_KEY, PRESS, 300},
{OUTPUT_KEY, RELEASE, 300}
```

---

## Test 1.7: Tap Timeout Boundary - Sequence Continuation

**Objective**: Verify sequence continues when next press occurs before tap timeout

**Configuration**:
```cpp
Tap actions: [1: SENDKEY(3001), 2: SENDKEY(3002)]
Hold actions: [1: CHANGELAYER(1)]
```

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
release_key(TAP_DANCE_KEY, 100); // t=100ms
press_key(TAP_DANCE_KEY, 199);   // t=299ms (1ms before tap timeout)
release_key(TAP_DANCE_KEY, 50);  // t=349ms
platform_wait_ms(200);          // Wait for final tap timeout
```

**Expected Output**:
```cpp
{3002, PRESS, 549},    // Second tap action
{3002, RELEASE, 549}
```

---

## Test 1.8: Long Hold Duration

**Objective**: Verify hold action remains active for extended periods

**Configuration**: Same as Test 1.1

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
platform_wait_ms(1000);         // Hold for 1 second
release_key(TAP_DANCE_KEY);      // t=1000ms
```

**Expected Output**:
```cpp
{LAYER_1, ACTIVATE, 200},
{LAYER_1, DEACTIVATE, 1000}
```

---

## Test 1.9: No Hold Action Configured - Immediate Execution

**Objective**: Verify immediate execution when no hold action available

**Configuration**:
```cpp
Tap actions: [1: SENDKEY(OUTPUT_KEY)]
Hold actions: [] // No hold actions
```

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
release_key(TAP_DANCE_KEY, 100); // t=100ms
```

**Expected Output**:
```cpp
{OUTPUT_KEY, PRESS, 0},     // Immediate execution on press
{OUTPUT_KEY, RELEASE, 100}  // Release follows input
```

---

## Test 1.10: Only Hold Action Configured

**Objective**: Verify behavior when only hold action is configured

**Configuration**:
```cpp
Tap actions: [] // No tap actions
Hold actions: [1: CHANGELAYER(1)]
```

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
release_key(TAP_DANCE_KEY, 100); // t=100ms (before timeout)
platform_wait_ms(200);          // Wait for potential processing
```

**Expected Output**:
```cpp
// No output - no tap action configured and hold timeout not reached
```

---

## Test 1.11: Only Hold Action - Timeout Reached

**Objective**: Verify hold action executes when only hold configured and timeout reached

**Configuration**: Same as Test 1.10

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
platform_wait_ms(250);          // Exceed hold timeout
release_key(TAP_DANCE_KEY);      // t=250ms
```

**Expected Output**:
```cpp
{LAYER_1, ACTIVATE, 200},
{LAYER_1, DEACTIVATE, 250}
```

---

## Test 1.12: State Persistence - Multiple Sequences

**Objective**: Verify state machine properly resets between independent sequences

**Configuration**: Same as Test 1.1

**Input Sequence**:
```cpp
// First sequence - tap
press_key(TAP_DANCE_KEY);        // t=0ms
release_key(TAP_DANCE_KEY, 100); // t=100ms
platform_wait_ms(200);          // t=300ms - first sequence completes

// Second sequence - hold
press_key(TAP_DANCE_KEY, 100);   // t=400ms
platform_wait_ms(250);          // t=650ms
release_key(TAP_DANCE_KEY);      // t=650ms
```

**Expected Output**:
```cpp
{OUTPUT_KEY, PRESS, 300},       // First sequence tap
{OUTPUT_KEY, RELEASE, 300},
{LAYER_1, ACTIVATE, 600},       // Second sequence hold
{LAYER_1, DEACTIVATE, 650}
```

---

## Test 1.13: Very Short Tap - Minimum Duration

**Objective**: Verify system handles very short tap durations

**Configuration**: Same as Test 1.1

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
release_key(TAP_DANCE_KEY, 1);   // t=1ms (very short tap)
platform_wait_ms(200);          // Wait for tap timeout
```

**Expected Output**:
```cpp
{OUTPUT_KEY, PRESS, 201},
{OUTPUT_KEY, RELEASE, 201}
```

---

## Test 1.14: State Machine Reset Verification

**Objective**: Verify internal state properly resets after each sequence

**Configuration**: Same as Test 1.1

**Input Sequence**:
```cpp
// First sequence - incomplete (should reset)
press_key(TAP_DANCE_KEY);        // t=0ms
release_key(TAP_DANCE_KEY, 50);  // t=50ms
platform_wait_ms(200);          // t=250ms - sequence completes

// Immediate second sequence - should start fresh
press_key(TAP_DANCE_KEY);        // t=250ms
release_key(TAP_DANCE_KEY, 50);  // t=300ms
platform_wait_ms(200);          // t=500ms - second sequence completes
```

**Expected Output**:
```cpp
{OUTPUT_KEY, PRESS, 250},       // First sequence
{OUTPUT_KEY, RELEASE, 250},
{OUTPUT_KEY, PRESS, 500},       // Second sequence (fresh start)
{OUTPUT_KEY, RELEASE, 500}
```

---

## Test 1.15: Zero-Length Actions

**Objective**: Verify system handles simultaneous press/release actions

**Configuration**: Same as Test 1.1

**Input Sequence**:
```cpp
tap_key(TAP_DANCE_KEY, 0);       // t=0ms, immediate press+release
platform_wait_ms(200);          // Wait for tap timeout
```

**Expected Output**:
```cpp
{OUTPUT_KEY, PRESS, 200},
{OUTPUT_KEY, RELEASE, 200}
```