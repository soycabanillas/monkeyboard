# Test Group 9: Configuration Combinations

This group tests various configuration setups, different timeout values, strategy variations, and edge configuration combinations.

## Test 9.1: Tap-Only Configuration

**Objective**: Verify behavior when only tap actions are configured (no hold actions)

**Configuration**:
```cpp
TAP_DANCE_KEY = 3000
Strategy: TAP_PREFERRED
Tap actions: [1: SENDKEY(3001), 2: SENDKEY(3002), 3: SENDKEY(3003)]
Hold actions: [] // No hold actions configured
Hold timeout: 200ms, Tap timeout: 200ms
```

**Test 9.1a - Single Tap**:
```cpp
tap_key(TAP_DANCE_KEY, 50);      // t=0-50ms
Expected: Immediate execution - {3001, PRESS, 0}, {3001, RELEASE, 50}
```

**Test 9.1b - Multi-Tap Sequence**:
```cpp
tap_key(TAP_DANCE_KEY, 30);      // t=0-30ms
tap_key(TAP_DANCE_KEY, 40, 30);  // t=70-110ms
tap_key(TAP_DANCE_KEY, 50, 30);  // t=160-210ms
Expected: Immediate execution - {3003, PRESS, 160}, {3003, RELEASE, 210}
```

**Test 9.1c - Hold Attempt (Should Still Tap)**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
platform_wait_ms(300);          // t=300ms (exceed hold timeout)
release_key(TAP_DANCE_KEY);      // t=300ms
Expected: Immediate execution - {3001, PRESS, 0}, {3001, RELEASE, 300}
```

---

## Test 9.2: Hold-Only Configuration

**Objective**: Verify behavior when only hold actions are configured (no tap actions)

**Configuration**:
```cpp
TAP_DANCE_KEY = 3000
Strategy: TAP_PREFERRED
Tap actions: [] // No tap actions configured
Hold actions: [1: CHANGELAYERTEMPO(1), 2: CHANGELAYERTEMPO(2)]
Hold timeout: 200ms, Tap timeout: 200ms
```

**Test 9.2a - Tap Attempt (No Action)**:
```cpp
tap_key(TAP_DANCE_KEY, 50);      // t=0-50ms
platform_wait_ms(200);          // t=250ms
Expected: No output (no tap action configured)
```

**Test 9.2b - Hold Execution**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
platform_wait_ms(250);          // t=250ms
release_key(TAP_DANCE_KEY);      // t=250ms
Expected: {LAYER_1, ACTIVATE, 200}, {LAYER_1, DEACTIVATE, 250}
```

**Test 9.2c - Multi-Tap Hold**:
```cpp
tap_key(TAP_DANCE_KEY, 30);      // t=0-30ms (1st tap)
press_key(TAP_DANCE_KEY, 50);    // t=80ms (2nd tap, hold)
platform_wait_ms(250);          // t=330ms
release_key(TAP_DANCE_KEY);      // t=330ms
Expected: {LAYER_2, ACTIVATE, 280}, {LAYER_2, DEACTIVATE, 330}
```

---

## Test 9.3: Sparse Configuration - Gaps in Tap Counts

**Objective**: Verify behavior with non-sequential tap count configurations

**Configuration**:
```cpp
TAP_DANCE_KEY = 3000
Strategy: TAP_PREFERRED
Tap actions: [1: SENDKEY(3001), 3: SENDKEY(3003)] // No 2nd tap action
Hold actions: [2: CHANGELAYERTEMPO(2)] // Only 2nd tap has hold
Hold timeout: 200ms, Tap timeout: 200ms
```

**Test 9.3a - First Tap (Configured)**:
```cpp
tap_key(TAP_DANCE_KEY, 50);      // t=0-50ms
platform_wait_ms(200);          // t=250ms
Expected: {3001, PRESS, 250}, {3001, RELEASE, 250}
```

**Test 9.3b - Second Tap Hold (Configured)**:
```cpp
tap_key(TAP_DANCE_KEY, 30);      // t=0-30ms
press_key(TAP_DANCE_KEY, 50);    // t=80ms
platform_wait_ms(250);          // t=330ms
release_key(TAP_DANCE_KEY);      // t=330ms
Expected: {LAYER_2, ACTIVATE, 280}, {LAYER_2, DEACTIVATE, 330}
```

**Test 9.3c - Second Tap Tap (No Action)**:
```cpp
tap_key(TAP_DANCE_KEY, 30);      // t=0-30ms
tap_key(TAP_DANCE_KEY, 50, 30);  // t=80-130ms (no tap action for 2nd)
platform_wait_ms(200);          // t=330ms
Expected: No output (no tap action for 2nd tap count)
```

**Test 9.3d - Third Tap (Configured)**:
```cpp
tap_key(TAP_DANCE_KEY, 30);      // t=0-30ms
tap_key(TAP_DANCE_KEY, 40, 30);  // t=70-110ms
tap_key(TAP_DANCE_KEY, 50, 30);  // t=160-210ms
platform_wait_ms(200);          // t=410ms
Expected: {3003, PRESS, 410}, {3003, RELEASE, 410}
```

---

## Test 9.4: Custom Timeout Configuration

**Objective**: Verify behavior with non-default timeout values

**Configuration**:
```cpp
TAP_DANCE_KEY = 3000
Strategy: TAP_PREFERRED
Tap actions: [1: SENDKEY(3001)]
Hold actions: [1: CHANGELAYERTEMPO(1)]
Hold timeout: 100ms, Tap timeout: 300ms // Custom timeouts
```

**Test 9.4a - Short Hold Timeout**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
platform_wait_ms(150);          // t=150ms (exceed 100ms timeout)
release_key(TAP_DANCE_KEY);      // t=150ms
Expected: {LAYER_1, ACTIVATE, 100}, {LAYER_1, DEACTIVATE, 150}
```

**Test 9.4b - Long Tap Timeout**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
release_key(TAP_DANCE_KEY, 50);  // t=50ms
platform_wait_ms(300);          // t=350ms (300ms tap timeout)
Expected: {3001, PRESS, 350}, {3001, RELEASE, 350}
```

**Test 9.4c - Sequence Continuation with Long Tap Timeout**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
release_key(TAP_DANCE_KEY, 50);  // t=50ms
press_key(TAP_DANCE_KEY, 299);   // t=349ms (1ms before tap timeout)
release_key(TAP_DANCE_KEY, 50);  // t=399ms
platform_wait_ms(300);          // t=699ms
Expected: {3001, PRESS, 699}, {3001, RELEASE, 699} (single tap, sequence continued)
```

---

## Test 9.5: Asymmetric Timeout Configuration

**Objective**: Verify behavior when hold timeout > tap timeout

**Configuration**:
```cpp
Tap actions: [1: SENDKEY(3001), 2: SENDKEY(3002)]
Hold actions: [1: CHANGELAYERTEMPO(1)]
Hold timeout: 300ms, Tap timeout: 150ms // Hold > Tap
Other settings same as Test 9.4
```

**Test 9.5a - Tap Timeout Before Hold Timeout**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
release_key(TAP_DANCE_KEY, 100); // t=100ms
// Tap timeout (150ms) occurs before hold timeout (300ms)
platform_wait_ms(150);          // t=250ms
Expected: {3001, PRESS, 250}, {3001, RELEASE, 250}
```

**Test 9.5b - Hold Timeout After Release**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
platform_wait_ms(350);          // t=350ms (exceed hold timeout)
release_key(TAP_DANCE_KEY);      // t=350ms
Expected: {LAYER_1, ACTIVATE, 300}, {LAYER_1, DEACTIVATE, 350}
```

---

## Test 9.6: Strategy Variation per Configuration

**Objective**: Verify different strategies work with various configurations

**Base Configuration**:
```cpp
Tap actions: [1: SENDKEY(3001)]
Hold actions: [1: CHANGELAYERTEMPO(1)]
INTERRUPTING_KEY = 3010
Hold timeout: 200ms, Tap timeout: 200ms
```

**Test 9.6a - TAP_PREFERRED with Configuration**:
```cpp
Strategy: TAP_PREFERRED
Input: Trigger press, interrupt press+release, trigger release (before timeout)
Expected: Tap action (interruption ignored)
```

**Test 9.6b - BALANCED with Configuration**:
```cpp
Strategy: BALANCED
Input: Trigger press, interrupt press+release, trigger release
Expected: Hold action (complete cycle)
```

**Test 9.6c - HOLD_PREFERRED with Configuration**:
```cpp
Strategy: HOLD_PREFERRED
Input: Trigger press, interrupt press, trigger release
Expected: Hold action (immediate on interrupt)
```

---

## Test 9.7: Maximum Configuration Complexity

**Objective**: Verify system handles maximum practical configuration complexity

**Configuration**:
```cpp
TAP_DANCE_KEY = 3000
Strategy: BALANCED
Tap actions: [1: SENDKEY(3001), 2: SENDKEY(3002), 3: SENDKEY(3003), 
              4: SENDKEY(3004), 5: SENDKEY(3005)]
Hold actions: [1: CHANGELAYERTEMPO(1), 2: CHANGELAYERTEMPO(2), 
               3: CHANGELAYERTEMPO(3), 4: CHANGELAYERTEMPO(4), 5: CHANGELAYERTEMPO(5)]
Hold timeout: 200ms, Tap timeout: 200ms
INTERRUPTING_KEY = 3010
```

**Test 9.7a - Fifth Tap Action**:
```cpp
// 5 rapid taps
for (int i = 0; i < 5; i++) {
    tap_key(TAP_DANCE_KEY, 20);
    platform_wait_ms(30);
}
platform_wait_ms(200);
Expected: {3005, PRESS, ...}, {3005, RELEASE, ...}
```

**Test 9.7b - Fifth Hold Action**:
```cpp
// 4 taps + 5th hold
for (int i = 0; i < 4; i++) {
    tap_key(TAP_DANCE_KEY, 20);
    platform_wait_ms(30);
}
press_key(TAP_DANCE_KEY, 30);
platform_wait_ms(250);
release_key(TAP_DANCE_KEY);
Expected: {LAYER_5, ACTIVATE, ...}, {LAYER_5, DEACTIVATE, ...}
```

---

## Test 9.8: Minimal Configuration

**Objective**: Verify system handles minimal valid configurations

**Configuration**:
```cpp
TAP_DANCE_KEY = 3000
Strategy: TAP_PREFERRED
Tap actions: [1: SENDKEY(3001)] // Single tap action only
Hold actions: [] // No hold actions
Hold timeout: 200ms, Tap timeout: 200ms
```

**Test 9.8a - Single Valid Action**:
```cpp
tap_key(TAP_DANCE_KEY, 50);
Expected: {3001, PRESS, 0}, {3001, RELEASE, 50}
```

**Test 9.8b - Overflow from Minimal**:
```cpp
tap_key(TAP_DANCE_KEY, 30);
tap_key(TAP_DANCE_KEY, 40, 30);  // 2nd tap - overflow
Expected: {3001, PRESS, 0}, {3001, RELEASE, 30} (immediate, first tap)
         {3001, PRESS, 70}, {3001, RELEASE, 110} (immediate, overflow)
```

---

## Test 9.9: Mixed Action Types Configuration

**Objective**: Verify configurations with different action types at different tap counts

**Configuration**:
```cpp
TAP_DANCE_KEY = 3000
Strategy: TAP_PREFERRED
Tap actions: [1: SENDKEY(3001), 2: SENDKEY(3002)]
Hold actions: [1: CHANGELAYERTEMPO(1), 3: CHANGELAYERTEMPO(3)] // Skip 2nd
Hold timeout: 200ms, Tap timeout: 200ms
```

**Test 9.9a - First Count (Both Available)**:
```cpp
// Tap
tap_key(TAP_DANCE_KEY, 50);
platform_wait_ms(200);
Expected: {3001, PRESS, 250}, {3001, RELEASE, 250}

// Hold
press_key(TAP_DANCE_KEY);
platform_wait_ms(250);
release_key(TAP_DANCE_KEY);
Expected: {LAYER_1, ACTIVATE, 200}, {LAYER_1, DEACTIVATE, 250}
```

**Test 9.9b - Second Count (Tap Only)**:
```cpp
tap_key(TAP_DANCE_KEY, 30);
tap_key(TAP_DANCE_KEY, 50, 30);
platform_wait_ms(200);
Expected: {3002, PRESS, 280}, {3002, RELEASE, 280}
```

**Test 9.9c - Third Count (Hold Only)**:
```cpp
tap_key(TAP_DANCE_KEY, 30);
tap_key(TAP_DANCE_KEY, 40, 30);
press_key(TAP_DANCE_KEY, 30);
platform_wait_ms(250);
release_key(TAP_DANCE_KEY);
Expected: {LAYER_3, ACTIVATE, ...}, {LAYER_3, DEACTIVATE, ...}
```

---

## Test 9.10: Zero-Based vs One-Based Configuration

**Objective**: Verify proper handling of tap count indexing

**Configuration**:
```cpp
TAP_DANCE_KEY = 3000
Strategy: TAP_PREFERRED
// Explicitly test that tap count 1 = first tap, count 2 = second tap, etc.
Tap actions: [1: SENDKEY(0x31), 2: SENDKEY(0x32), 3: SENDKEY(0x33)] // '1', '2', '3'
Hold actions: []
Hold timeout: 200ms, Tap timeout: 200ms
```

**Test 9.10a - First Tap (Count 1)**:
```cpp
tap_key(TAP_DANCE_KEY, 50);
Expected: {0x31, PRESS, 0}, {0x31, RELEASE, 50} // '1' key
```

**Test 9.10b - Second Tap (Count 2)**:
```cpp
tap_key(TAP_DANCE_KEY, 30);
tap_key(TAP_DANCE_KEY, 50, 30);
Expected: {0x32, PRESS, 0}, {0x32, RELEASE, 80} // '2' key
```

**Test 9.10c - Third Tap (Count 3)**:
```cpp
tap_key(TAP_DANCE_KEY, 30);
tap_key(TAP_DANCE_KEY, 40, 30);
tap_key(TAP_DANCE_KEY, 50, 30);
Expected: {0x33, PRESS, 0}, {0x33, RELEASE, 150} // '3' key
```

---

## Test 9.11: Configuration with Large Timeout Values

**Objective**: Verify system handles large timeout values correctly

**Configuration**:
```cpp
TAP_DANCE_KEY = 3000
Strategy: TAP_PREFERRED
Tap actions: [1: SENDKEY(3001)]
Hold actions: [1: CHANGELAYERTEMPO(1)]
Hold timeout: 1000ms, Tap timeout: 2000ms // Large timeouts
```

**Test 9.11a - Long Hold Timeout**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
platform_wait_ms(1100);         // t=1100ms (exceed 1000ms timeout)
release_key(TAP_DANCE_KEY);      // t=1100ms
Expected: {LAYER_1, ACTIVATE, 1000}, {LAYER_1, DEACTIVATE, 1100}
```

**Test 9.11b - Long Tap Timeout**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
release_key(TAP_DANCE_KEY, 500); // t=500ms
platform_wait_ms(2000);         // t=2500ms (2000ms tap timeout)
Expected: {3001, PRESS, 2500}, {3001, RELEASE, 2500}
```

---

## Test 9.12: Configuration Edge Cases

**Objective**: Verify handling of edge case configurations

**Test 9.12a - Zero Timeout Values**:
```cpp
Configuration: Hold timeout: 0ms, Tap timeout: 0ms
Expected Behavior: Implementation dependent (immediate timeouts)
```

**Test 9.12b - Identical Timeout Values**:
```cpp
Configuration: Hold timeout: 200ms, Tap timeout: 200ms
Input: Press, release at 100ms, wait 200ms total
Expected: Clear precedence rules (tap timeout from release, hold from press)
```

**Test 9.12c - Very Large Tap Count Configuration**:
```cpp
Configuration: Tap actions for counts 1-100
Expected: System handles without performance degradation
```

---

## Test 9.13: Configuration Consistency Verification

**Objective**: Verify consistent behavior across different configurations

**Base Test Pattern**:
```cpp
Input: Single tap with 50ms hold
```

**Test 9.13a - Tap-Only Config**:
```cpp
Config: Tap[1: SENDKEY], Hold[]
Expected: Immediate execution
```

**Test 9.13b - Hold-Only Config**:
```cpp
Config: Tap[], Hold[1: LAYER]
Expected: No action (no tap configured)
```

**Test 9.13c - Mixed Config**:
```cpp
Config: Tap[1: SENDKEY], Hold[1: LAYER]
Expected: Delayed tap execution
```

---

## Test 9.14: Multi-Key Configuration Comparison

**Objective**: Verify independent behavior of multiple configured keys

**Configuration**:
```cpp
TAP_DANCE_KEY_1 = 3000  // Tap-only configuration
TAP_DANCE_KEY_2 = 3100  // Hold-only configuration
TAP_DANCE_KEY_3 = 3200  // Mixed configuration
```

**Input Sequence**:
```cpp
// Simultaneous activation of all three keys
press_key(TAP_DANCE_KEY_1);      // t=0ms
press_key(TAP_DANCE_KEY_2, 10);  // t=10ms
press_key(TAP_DANCE_KEY_3, 10);  // t=20ms
platform_wait_ms(250);          // t=270ms
release_key(TAP_DANCE_KEY_1);    // t=270ms
release_key(TAP_DANCE_KEY_2);    // t=270ms
release_key(TAP_DANCE_KEY_3);    // t=270ms
```

**Expected Output**:
```cpp
// Key 1 (tap-only) - immediate
{KEY1_OUTPUT, PRESS, 0}, {KEY1_OUTPUT, RELEASE, 270},
// Key 2 (hold-only) - layer activation
{LAYER_2, ACTIVATE, 210}, {LAYER_2, DEACTIVATE, 270},
// Key 3 (mixed) - layer activation  
{LAYER_3, ACTIVATE, 220}, {LAYER_3, DEACTIVATE, 270}
```

---

## Test 9.15: Configuration Performance Impact

**Objective**: Verify performance consistency across different configuration complexities

**Simple Configuration**:
```cpp
Tap actions: [1: SENDKEY(3001)]
Hold actions: []
```

**Complex Configuration**:
```cpp
Tap actions: [1-10: various SENDKEY actions]
Hold actions: [1-10: various LAYER actions]
```

**Performance Test**:
```cpp
// Identical input pattern for both configurations
for (int i = 0; i < 100; i++) {
    tap_key(TAP_DANCE_KEY, 10);
    platform_wait_ms(20);
}
Expected: Consistent timing behavior regardless of configuration complexity
```