# Test Group 8: Action Execution Verification

This group tests proper execution of both action types, timing verification, parameter handling, and mixed action scenarios.

## Test 8.1: Basic SENDKEY Action Execution

**Objective**: Verify basic `TDCL_TAP_KEY_SENDKEY` press/release sequence

**Configuration**:
```cpp
TAP_DANCE_KEY = 3000
OUTPUT_KEY = 3001
Strategy: TAP_PREFERRED
Tap actions: [1: SENDKEY(OUTPUT_KEY)]
Hold actions: []
Hold timeout: 200ms, Tap timeout: 200ms
```

**Input Sequence**:
```cpp
tap_key(TAP_DANCE_KEY, 50);      // t=0-50ms
platform_wait_ms(200);          // t=250ms
```

**Expected Output**:
```cpp
{OUTPUT_KEY, PRESS, 0},     // Immediate press (no hold configured)
{OUTPUT_KEY, RELEASE, 50}   // Release follows input timing
```

---

## Test 8.2: Basic CHANGELAYERTEMPO Action Execution

**Objective**: Verify basic `TDCL_HOLD_KEY_CHANGELAYERTEMPO` layer activation/deactivation

**Configuration**:
```cpp
TAP_DANCE_KEY = 3000
TARGET_LAYER = 1
Strategy: TAP_PREFERRED
Tap actions: [1: SENDKEY(3001)]
Hold actions: [1: CHANGELAYERTEMPO(TARGET_LAYER)]
Hold timeout: 200ms, Tap timeout: 200ms
```

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
platform_wait_ms(250);          // t=250ms (exceed hold timeout)
release_key(TAP_DANCE_KEY);      // t=250ms
```

**Expected Output**:
```cpp
{LAYER_1, ACTIVATE, 200},    // Layer activation at hold timeout
{LAYER_1, DEACTIVATE, 250}   // Layer deactivation on key release
```

---

## Test 8.3: SENDKEY Action Timing Precision

**Objective**: Verify precise timing of SENDKEY press/release events

**Configuration**: Same as Test 8.1

**Input Sequence**:
```cpp
platform_wait_ms(500);          // t=500ms (establish baseline)
press_key(TAP_DANCE_KEY);        // t=500ms
platform_wait_ms(150);          // t=650ms
release_key(TAP_DANCE_KEY);      // t=650ms
```

**Expected Output**:
```cpp
{3001, PRESS, 500},     // Press exactly at trigger press time
{3001, RELEASE, 650}    // Release exactly at trigger release time
```

---

## Test 8.4: CHANGELAYERTEMPO Action Duration Tracking

**Objective**: Verify layer activation duration tracking across different hold durations

**Configuration**: Same as Test 8.2

**Test 8.4a - Short Hold (300ms)**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
platform_wait_ms(300);          // t=300ms
release_key(TAP_DANCE_KEY);      // t=300ms
Expected: ACTIVATE at 200ms, DEACTIVATE at 300ms (100ms active)
```

**Test 8.4b - Long Hold (1000ms)**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
platform_wait_ms(1000);         // t=1000ms
release_key(TAP_DANCE_KEY);      // t=1000ms
Expected: ACTIVATE at 200ms, DEACTIVATE at 1000ms (800ms active)
```

---

## Test 8.5: Multiple SENDKEY Actions - Different Keys

**Objective**: Verify multiple SENDKEY actions with different target keys

**Configuration**:
```cpp
Tap actions: [1: SENDKEY(3001), 2: SENDKEY(3002), 3: SENDKEY(3003)]
Hold actions: []
Other settings same as Test 8.1
```

**Input Sequence**:
```cpp
tap_key(TAP_DANCE_KEY, 30);      // t=0-30ms (1st tap)
tap_key(TAP_DANCE_KEY, 40, 30);  // t=70-110ms (2nd tap)
tap_key(TAP_DANCE_KEY, 50, 30);  // t=160-210ms (3rd tap)
platform_wait_ms(200);          // t=410ms
```

**Expected Output**:
```cpp
{3003, PRESS, 410},     // Third tap action
{3003, RELEASE, 410}
```

---

## Test 8.6: Multiple CHANGELAYERTEMPO Actions - Different Layers

**Objective**: Verify multiple layer actions target different layers correctly

**Configuration**:
```cpp
Tap actions: [1: SENDKEY(3001), 2: SENDKEY(3002)]
Hold actions: [1: CHANGELAYERTEMPO(1), 2: CHANGELAYERTEMPO(2)]
Other settings same as Test 8.1
```

**Test 8.6a - First Tap Hold**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
platform_wait_ms(250);          // t=250ms
release_key(TAP_DANCE_KEY);      // t=250ms
Expected: LAYER_1 activation/deactivation
```

**Test 8.6b - Second Tap Hold**:
```cpp
tap_key(TAP_DANCE_KEY, 30);      // t=0-30ms
press_key(TAP_DANCE_KEY, 50);    // t=80ms (2nd tap)
platform_wait_ms(250);          // t=330ms
release_key(TAP_DANCE_KEY);      // t=330ms
Expected: LAYER_2 activation/deactivation
```

---

## Test 8.7: Mixed Action Types in Single Configuration

**Objective**: Verify proper execution when both action types are configured

**Configuration**: Same as Test 8.6

**Test 8.7a - Tap Execution (SENDKEY)**:
```cpp
tap_key(TAP_DANCE_KEY, 50);      // t=0-50ms
platform_wait_ms(200);          // t=250ms
Expected: SENDKEY action execution
```

**Test 8.7b - Hold Execution (CHANGELAYERTEMPO)**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
platform_wait_ms(250);          // t=250ms
release_key(TAP_DANCE_KEY);      // t=250ms
Expected: CHANGELAYERTEMPO action execution
```

---

## Test 8.8: Action Parameter Validation

**Objective**: Verify actions execute with correct parameters

**Configuration**:
```cpp
Tap actions: [1: SENDKEY(0x41)]  // 'A' key
Hold actions: [1: CHANGELAYERTEMPO(3)]  // Layer 3
Other settings same as Test 8.1
```

**Test 8.8a - SENDKEY Parameter**:
```cpp
tap_key(TAP_DANCE_KEY, 50);
Expected Output: {0x41, PRESS, ...}, {0x41, RELEASE, ...}
```

**Test 8.8b - CHANGELAYERTEMPO Parameter**:
```cpp
press_key(TAP_DANCE_KEY);
platform_wait_ms(250);
release_key(TAP_DANCE_KEY);
Expected Output: {LAYER_3, ACTIVATE, ...}, {LAYER_3, DEACTIVATE, ...}
```

---

## Test 8.9: Action Execution with Strategy Integration

**Objective**: Verify action execution works correctly with different hold strategies

**Configuration**:
```cpp
Strategy: BALANCED
Tap actions: [1: SENDKEY(3001)]
Hold actions: [1: CHANGELAYERTEMPO(1)]
INTERRUPTING_KEY = 3010
Other settings same as Test 8.1
```

**Test 8.9a - Strategy Triggers Tap Action**:
```cpp
press_key(TAP_DANCE_KEY);          // t=0ms
press_key(INTERRUPTING_KEY, 50);   // t=50ms
release_key(TAP_DANCE_KEY, 50);    // t=100ms (trigger released first)
release_key(INTERRUPTING_KEY, 50); // t=150ms
Expected: SENDKEY action execution
```

**Test 8.9b - Strategy Triggers Hold Action**:
```cpp
press_key(TAP_DANCE_KEY);          // t=0ms
press_key(INTERRUPTING_KEY, 50);   // t=50ms
release_key(INTERRUPTING_KEY, 50); // t=100ms (complete cycle)
release_key(TAP_DANCE_KEY, 50);    // t=150ms
Expected: CHANGELAYERTEMPO action execution
```

---

## Test 8.10: Action Execution in Overflow Scenarios

**Objective**: Verify action execution during overflow conditions

**Configuration**:
```cpp
Tap actions: [1: SENDKEY(3001), 2: SENDKEY(3002)]
Hold actions: [1: CHANGELAYERTEMPO(1)]
Other settings same as Test 8.1
```

**Test 8.10a - Tap Overflow**:
```cpp
// 4 taps (overflow)
for (int i = 0; i < 4; i++) {
    tap_key(TAP_DANCE_KEY, 20);
    platform_wait_ms(30);
}
platform_wait_ms(200);
Expected: SENDKEY(3002) action (last configured)
```

**Test 8.10b - Hold Overflow (No Action)**:
```cpp
// 3 taps then hold (no hold action for 3rd tap)
tap_key(TAP_DANCE_KEY, 20);
tap_key(TAP_DANCE_KEY, 30, 20);
press_key(TAP_DANCE_KEY, 30);
platform_wait_ms(250);
release_key(TAP_DANCE_KEY);
Expected: SENDKEY(3002) action (no hold available)
```

---

## Test 8.11: Action Execution Timing with Immediate vs Delayed

**Objective**: Verify timing differences between immediate and delayed action execution

**Configuration**:
```cpp
Tap actions: [1: SENDKEY(3001)]
Hold actions: [] // No hold actions for immediate execution
```

**Test 8.11a - Immediate Execution**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
release_key(TAP_DANCE_KEY, 100); // t=100ms
Expected: PRESS at 0ms, RELEASE at 100ms
```

**Configuration**:
```cpp
Hold actions: [1: CHANGELAYERTEMPO(1)] // Add hold action for delayed execution
```

**Test 8.11b - Delayed Execution**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
release_key(TAP_DANCE_KEY, 100); // t=100ms
platform_wait_ms(200);          // t=300ms
Expected: PRESS at 300ms, RELEASE at 300ms
```

---

## Test 8.12: Concurrent Action Execution Prevention

**Objective**: Verify system prevents concurrent layer activations (per specification)

**Configuration**:
```cpp
TAP_DANCE_KEY_1 = 3000
TAP_DANCE_KEY_2 = 3100
Both configured with: Hold actions: [1: CHANGELAYERTEMPO(1)]
```

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY_1);      // t=0ms
platform_wait_ms(100);          // t=100ms
press_key(TAP_DANCE_KEY_2);      // t=100ms (second trigger)
platform_wait_ms(150);          // t=250ms (both exceed timeout)
release_key(TAP_DANCE_KEY_1, 50); // t=300ms
release_key(TAP_DANCE_KEY_2);    // t=300ms
```

**Expected Output**:
```cpp
{LAYER_1, ACTIVATE, 200},        // First key activates layer
{LAYER_1, DEACTIVATE, 300},      // First key deactivates
// Second key should not activate (per "no multiple" rule)
```

---

## Test 8.13: Action Execution Error Scenarios

**Objective**: Verify graceful handling of potential action execution issues

**Configuration**:
```cpp
Tap actions: [1: SENDKEY(0)]     // Potentially invalid key code
Hold actions: [1: CHANGELAYERTEMPO(255)] // Potentially invalid layer
Other settings same as Test 8.1
```

**Test 8.13a - Invalid SENDKEY Parameter**:
```cpp
tap_key(TAP_DANCE_KEY, 50);
Expected: System handles gracefully (implementation dependent)
```

**Test 8.13b - Invalid CHANGELAYERTEMPO Parameter**:
```cpp
press_key(TAP_DANCE_KEY);
platform_wait_ms(250);
release_key(TAP_DANCE_KEY);
Expected: System handles gracefully (implementation dependent)
```

---

## Test 8.14: Action Execution Sequence Verification

**Objective**: Verify proper sequencing of actions in complex scenarios

**Configuration**:
```cpp
Tap actions: [1: SENDKEY(3001), 2: SENDKEY(3002)]
Hold actions: [1: CHANGELAYERTEMPO(1)]
Other settings same as Test 8.1
```

**Input Sequence**:
```cpp
// First sequence - hold
press_key(TAP_DANCE_KEY);        // t=0ms
platform_wait_ms(250);          // t=250ms
release_key(TAP_DANCE_KEY, 50);  // t=300ms

// Second sequence - tap
tap_key(TAP_DANCE_KEY, 50, 50);  // t=400-450ms
platform_wait_ms(200);          // t=650ms

// Third sequence - multi-tap
tap_key(TAP_DANCE_KEY, 30);      // t=680-710ms
tap_key(TAP_DANCE_KEY, 50, 30);  // t=760-810ms
platform_wait_ms(200);          // t=1010ms
```

**Expected Output**:
```cpp
{LAYER_1, ACTIVATE, 200},        // First sequence - hold
{LAYER_1, DEACTIVATE, 300},
{3001, PRESS, 650},              // Second sequence - single tap
{3001, RELEASE, 650},
{3002, PRESS, 1010},             // Third sequence - double tap
{3002, RELEASE, 1010}
```

---

## Test 8.15: Action Execution Performance - Rapid Sequences

**Objective**: Verify action execution performance with rapid sequences

**Configuration**: Same as Test 8.1 (immediate execution)

**Input Sequence**:
```cpp
// 10 rapid tap sequences
for (int i = 0; i < 10; i++) {
    tap_key(TAP_DANCE_KEY, 5);   // Very fast taps
    platform_wait_ms(10);       // Minimal gap
}
```

**Expected Output**:
```cpp
// 10 pairs of press/release events at precise timing
{3001, PRESS, 0}, {3001, RELEASE, 5},
{3001, PRESS, 15}, {3001, RELEASE, 20},
{3001, PRESS, 30}, {3001, RELEASE, 35},
// ... (continues for all 10 sequences)
{3001, PRESS, 135}, {3001, RELEASE, 140}
```

---

## Test 8.16: Action State Cleanup Verification

**Objective**: Verify proper cleanup of action states between sequences

**Configuration**:
```cpp
Tap actions: [1: SENDKEY(3001)]
Hold actions: [1: CHANGELAYERTEMPO(1)]
Other settings same as Test 8.1
```

**Input Sequence**:
```cpp
// First sequence - hold with early termination
press_key(TAP_DANCE_KEY);        // t=0ms
platform_wait_ms(250);          // t=250ms (layer activated)
release_key(TAP_DANCE_KEY);      // t=250ms (layer deactivated)

// Immediate second sequence - should start clean
press_key(TAP_DANCE_KEY);        // t=250ms
release_key(TAP_DANCE_KEY, 50);  // t=300ms (tap, not hold)
platform_wait_ms(200);          // t=500ms
```

**Expected Output**:
```cpp
{LAYER_1, ACTIVATE, 200},        // First sequence - hold
{LAYER_1, DEACTIVATE, 250},
{3001, PRESS, 500},              // Second sequence - tap (clean state)
{3001, RELEASE, 500}
```