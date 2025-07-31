# Test Group 4: Action Overflow Scenarios

This group tests behavior when tap counts exceed configured actions, focusing on tap action overflow and hold action non-overflow behavior.

## Test 4.1: Basic Tap Action Overflow

**Objective**: Verify tap action overflow uses last configured action

**Configuration**:
```cpp
TAP_DANCE_KEY = 3000
Strategy: TAP_PREFERRED
Tap actions: [1: SENDKEY(3001), 2: SENDKEY(3002)]  // Only 2 actions configured
Hold actions: [1: CHANGELAYER(1)]
Hold timeout: 200ms, Tap timeout: 200ms
```

**Input Sequence**:
```cpp
// Perform 4 taps (exceeds configured actions)
tap_key(TAP_DANCE_KEY, 30);      // t=0-30ms (1st tap)
tap_key(TAP_DANCE_KEY, 50, 30);  // t=80-110ms (2nd tap)
tap_key(TAP_DANCE_KEY, 50, 30);  // t=160-190ms (3rd tap - overflow)
tap_key(TAP_DANCE_KEY, 50, 30);  // t=240-270ms (4th tap - overflow)
platform_wait_ms(200);          // t=470ms
```

**Expected Output**:
```cpp
{3002, PRESS, 470},    // Uses last configured action (2nd tap action)
{3002, RELEASE, 470}
```

---

## Test 4.2: Hold Action Non-Overflow

**Objective**: Verify hold actions do NOT overflow - no hold available beyond configured counts

**Configuration**: Same as Test 4.1

**Input Sequence**:
```cpp
tap_key(TAP_DANCE_KEY, 30);      // t=0-30ms (1st tap)
tap_key(TAP_DANCE_KEY, 50, 30);  // t=80-110ms (2nd tap)
press_key(TAP_DANCE_KEY, 50);    // t=160ms (3rd tap - overflow, attempt hold)
platform_wait_ms(250);          // t=410ms (exceed hold timeout)
release_key(TAP_DANCE_KEY);      // t=410ms
platform_wait_ms(200);          // t=610ms
```

**Expected Output**:
```cpp
{3002, PRESS, 610},    // Tap action only (no hold available for 3rd tap)
{3002, RELEASE, 610}
```

---

## Test 4.3: Overflow with Only SENDKEY Actions - Immediate Execution

**Objective**: Verify immediate execution when overflow occurs with only SENDKEY actions

**Configuration**:
```cpp
Tap actions: [1: SENDKEY(3001), 2: SENDKEY(3002)]
Hold actions: [] // No hold actions at all
Other settings same as Test 4.1
```

**Input Sequence**:
```cpp
tap_key(TAP_DANCE_KEY, 30);      // t=0-30ms (1st tap)
tap_key(TAP_DANCE_KEY, 50, 30);  // t=80-110ms (2nd tap)
press_key(TAP_DANCE_KEY, 50);    // t=160ms (3rd tap - overflow, immediate)
release_key(TAP_DANCE_KEY, 100); // t=260ms
```

**Expected Output**:
```cpp
{3002, PRESS, 160},    // Immediate execution on press (overflow + no hold)
{3002, RELEASE, 260}   // Release follows input timing
```

---

## Test 4.4: Overflow with Mixed Actions - Delayed Execution

**Objective**: Verify delayed execution when overflow occurs but hold actions exist at lower counts

**Configuration**:
```cpp
Tap actions: [1: SENDKEY(3001), 2: SENDKEY(3002), 3: SENDKEY(3003)]
Hold actions: [1: CHANGELAYER(1), 2: CHANGELAYER(2)] // Hold available for 1st, 2nd only
Other settings same as Test 4.1
```

**Input Sequence**:
```cpp
tap_key(TAP_DANCE_KEY, 30);      // t=0-30ms (1st tap)
tap_key(TAP_DANCE_KEY, 50, 30);  // t=80-110ms (2nd tap)
tap_key(TAP_DANCE_KEY, 50, 30);  // t=160-190ms (3rd tap - no hold at this count)
tap_key(TAP_DANCE_KEY, 50, 30);  // t=240-270ms (4th tap - overflow)
platform_wait_ms(200);          // t=470ms
```

**Expected Output**:
```cpp
{3003, PRESS, 470},    // Uses 3rd tap action (last configured, delayed execution)
{3003, RELEASE, 470}
```

---

## Test 4.5: Extreme Overflow - High Tap Count

**Objective**: Verify system handles very high tap counts with overflow

**Configuration**: Same as Test 4.1

**Input Sequence**:
```cpp
// Perform 10 rapid taps
for (int i = 0; i < 10; i++) {
    tap_key(TAP_DANCE_KEY, 20);
    platform_wait_ms(30);       // t = i*50 to (i*50+20)
}
platform_wait_ms(200);          // Final timeout at t=700ms
```

**Expected Output**:
```cpp
{3002, PRESS, 700},    // Still uses last configured action (2nd tap)
{3002, RELEASE, 700}
```

---

## Test 4.6: Overflow Hold Attempt with Strategy

**Objective**: Verify overflow with hold attempt using different strategies

**Configuration**:
```cpp
Strategy: HOLD_PREFERRED
Tap actions: [1: SENDKEY(3001), 2: SENDKEY(3002)]
Hold actions: [1: CHANGELAYER(1)] // Only 1st tap has hold
INTERRUPTING_KEY = 3010
Other settings same as Test 4.1
```

**Input Sequence**:
```cpp
tap_key(TAP_DANCE_KEY, 30);        // t=0-30ms (1st tap)
tap_key(TAP_DANCE_KEY, 50, 30);    // t=80-110ms (2nd tap)
press_key(TAP_DANCE_KEY, 50);      // t=160ms (3rd tap - overflow)
press_key(INTERRUPTING_KEY, 50);   // t=210ms (interrupt - would trigger hold if available)
release_key(INTERRUPTING_KEY, 50); // t=260ms
release_key(TAP_DANCE_KEY, 50);    // t=310ms
platform_wait_ms(200);            // t=510ms
```

**Expected Output**:
```cpp
{INTERRUPTING_KEY, PRESS, 210},
{3002, PRESS, 310},              // Tap action (no hold available for 3rd tap)
{3002, RELEASE, 310},
{INTERRUPTING_KEY, RELEASE, 260}
```

---

## Test 4.7: Overflow Mixed with Non-Overflow Hold

**Objective**: Verify overflow tap behavior mixed with valid hold actions at lower counts

**Configuration**: Same as Test 4.4

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms (1st tap - hold available)
platform_wait_ms(250);          // t=250ms (hold timeout exceeded)
release_key(TAP_DANCE_KEY);      // t=250ms
platform_wait_ms(50);           // t=300ms

// New sequence with overflow
tap_key(TAP_DANCE_KEY, 30);      // t=330-360ms (1st tap)
tap_key(TAP_DANCE_KEY, 50, 30);  // t=410-460ms (2nd tap)
tap_key(TAP_DANCE_KEY, 50, 30);  // t=510-540ms (3rd tap)
tap_key(TAP_DANCE_KEY, 50, 30);  // t=590-620ms (4th tap - overflow)
platform_wait_ms(200);          // t=820ms
```

**Expected Output**:
```cpp
{LAYER_1, ACTIVATE, 200},        // First sequence - hold action
{LAYER_1, DEACTIVATE, 250},
{3003, PRESS, 820},              // Second sequence - overflow uses 3rd action
{3003, RELEASE, 820}
```

---

## Test 4.8: Overflow Boundary - Exactly at Last Configured Action

**Objective**: Verify behavior exactly at the boundary of configured actions

**Configuration**:
```cpp
Tap actions: [1: SENDKEY(3001), 2: SENDKEY(3002), 3: SENDKEY(3003)]
Hold actions: [1: CHANGELAYER(1), 2: CHANGELAYER(2)]
Other settings same as Test 4.1
```

**Input Sequence**:
```cpp
// Exactly 3 taps (matches last configured action)
tap_key(TAP_DANCE_KEY, 30);      // t=0-30ms
tap_key(TAP_DANCE_KEY, 50, 30);  // t=80-110ms
tap_key(TAP_DANCE_KEY, 50, 30);  // t=160-190ms (exactly at boundary)
platform_wait_ms(200);          // t=390ms
```

**Expected Output**:
```cpp
{3003, PRESS, 390},    // Uses exact configured action (not overflow)
{3003, RELEASE, 390}
```

---

## Test 4.9: Overflow Boundary - One Beyond Last Configured

**Objective**: Verify overflow behavior starts exactly one beyond last configured action

**Configuration**: Same as Test 4.8

**Input Sequence**:
```cpp
// 4 taps (one beyond last configured)
tap_key(TAP_DANCE_KEY, 30);      // t=0-30ms
tap_key(TAP_DANCE_KEY, 50, 30);  // t=80-110ms
tap_key(TAP_DANCE_KEY, 50, 30);  // t=160-190ms
tap_key(TAP_DANCE_KEY, 50, 30);  // t=240-270ms (first overflow)
platform_wait_ms(200);          // t=470ms
```

**Expected Output**:
```cpp
{3003, PRESS, 470},    // Uses last configured action (overflow behavior)
{3003, RELEASE, 470}
```

---

## Test 4.10: Overflow with Hold Available at Overflow Count

**Objective**: Verify hold action at overflow count when configured

**Configuration**:
```cpp
Tap actions: [1: SENDKEY(3001), 2: SENDKEY(3002)]
Hold actions: [1: CHANGELAYER(1), 2: CHANGELAYER(2), 3: CHANGELAYER(3)] // Hold at 3rd
Other settings same as Test 4.1
```

**Input Sequence**:
```cpp
tap_key(TAP_DANCE_KEY, 30);      // t=0-30ms
tap_key(TAP_DANCE_KEY, 50, 30);  // t=80-110ms
press_key(TAP_DANCE_KEY, 50);    // t=160ms (3rd tap - hold available)
platform_wait_ms(250);          // t=410ms
release_key(TAP_DANCE_KEY);      // t=410ms
```

**Expected Output**:
```cpp
{LAYER_3, ACTIVATE, 360},        // Hold action available at 3rd tap
{LAYER_3, DEACTIVATE, 410}
```

---

## Test 4.11: Immediate Execution Decision Table - Overflow Scenarios

**Objective**: Verify immediate vs delayed execution decision in various overflow scenarios

**Test 4.11a - Immediate Execution (SENDKEY only, no hold)**:
```cpp
Configuration:
Tap actions: [1: SENDKEY(3001)]
Hold actions: [] // No hold actions

Input: 3 taps (overflow)
Expected: Immediate execution on each press
```

**Test 4.11b - Delayed Execution (Hold available at overflow count)**:
```cpp
Configuration:
Tap actions: [1: SENDKEY(3001)]
Hold actions: [1: CHANGELAYER(1), 2: CHANGELAYER(2), 3: CHANGELAYER(3)]

Input: 3 taps with hold attempt
Expected: Delayed execution, hold action possible
```

---

## Test 4.12: Overflow Reset Verification

**Objective**: Verify overflow sequences properly reset tap counts

**Configuration**: Same as Test 4.1

**Input Sequence**:
```cpp
// First overflow sequence (5 taps)
for (int i = 0; i < 5; i++) {
    tap_key(TAP_DANCE_KEY, 20);
    platform_wait_ms(30);
}
platform_wait_ms(200);          // First sequence completes

// Second sequence (2 taps - should not be affected by previous overflow)
tap_key(TAP_DANCE_KEY, 30);      // Should be 1st tap
tap_key(TAP_DANCE_KEY, 50, 30);  // Should be 2nd tap
platform_wait_ms(200);
```

**Expected Output**:
```cpp
{3002, PRESS, 350},    // First sequence - overflow (5th tap uses 2nd action)
{3002, RELEASE, 350},
{3002, PRESS, 630},    // Second sequence - normal 2nd tap
{3002, RELEASE, 630}
```

---

## Test 4.13: Overflow with Different Action Types

**Objective**: Verify overflow works correctly with different action types in sequence

**Configuration**:
```cpp
Tap actions: [1: SENDKEY(3001), 2: CHANGELAYERTEMPO(1), 3: SENDKEY(3003)]
Hold actions: [1: CHANGELAYER(2)]
Other settings same as Test 4.1
```

**Input Sequence**:
```cpp
// 4 taps - overflow should use 3rd action (SENDKEY)
tap_key(TAP_DANCE_KEY, 30);      // t=0-30ms
tap_key(TAP_DANCE_KEY, 50, 30);  // t=80-110ms
tap_key(TAP_DANCE_KEY, 50, 30);  // t=160-190ms
tap_key(TAP_DANCE_KEY, 50, 30);  // t=240-270ms (overflow)
platform_wait_ms(200);          // t=470ms
```

**Expected Output**:
```cpp
{3003, PRESS, 470},    // Uses 3rd action (SENDKEY) for overflow
{3003, RELEASE, 470}
```

---

## Test 4.14: Continuous Overflow - Multiple Sequences

**Objective**: Verify consistent overflow behavior across multiple sequences

**Configuration**: Same as Test 4.1

**Input Sequence**:
```cpp
// First overflow sequence
tap_key(TAP_DANCE_KEY, 20);
tap_key(TAP_DANCE_KEY, 30, 20);
tap_key(TAP_DANCE_KEY, 30, 20);  // 3rd tap - overflow
platform_wait_ms(200);          // t=220ms

platform_wait_ms(100);          // Gap between sequences

// Second overflow sequence
tap_key(TAP_DANCE_KEY, 20);      // t=340-360ms
tap_key(TAP_DANCE_KEY, 30, 20);  // t=380-410ms
tap_key(TAP_DANCE_KEY, 30, 20);  // t=440-470ms
tap_key(TAP_DANCE_KEY, 30, 20);  // t=500-530ms (4th tap - overflow)
platform_wait_ms(200);          // t=730ms
```

**Expected Output**:
```cpp
{3002, PRESS, 220},    // First overflow - 3rd tap uses 2nd action
{3002, RELEASE, 220},
{3002, PRESS, 730},    // Second overflow - 4th tap uses 2nd action
{3002, RELEASE, 730}
```

---

## Test 4.15: Overflow Edge Case - Zero Configured Actions

**Objective**: Verify behavior when no tap actions are configured but taps attempted

**Configuration**:
```cpp
Tap actions: [] // No tap actions configured
Hold actions: [1: CHANGELAYER(1)]
Other settings same as Test 4.1
```

**Input Sequence**:
```cpp
tap_key(TAP_DANCE_KEY, 50);      // t=0-50ms (no tap action available)
platform_wait_ms(200);          // t=250ms
```

**Expected Output**:
```cpp
// No output - no tap actions configured
```