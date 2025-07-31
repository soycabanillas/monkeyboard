# Test Group 10: Edge Cases and Special Conditions

This final group tests boundary conditions, stress scenarios, recovery from unusual states, and extreme input patterns.

## Test 10.1: Rapid Fire Stress Test

**Objective**: Verify system stability under extremely rapid input sequences

**Configuration**:
```cpp
TAP_DANCE_KEY = 3000
Strategy: TAP_PREFERRED
Tap actions: [1: SENDKEY(3001), 2: SENDKEY(3002)]
Hold actions: [1: CHANGELAYER(1)]
Hold timeout: 200ms, Tap timeout: 200ms
```

**Input Sequence**:
```cpp
// 50 rapid taps in 500ms (10ms per tap cycle)
for (int i = 0; i < 50; i++) {
    tap_key(TAP_DANCE_KEY, 1);    // 1ms hold
    platform_wait_ms(9);         // 9ms gap
}
platform_wait_ms(200);           // Final timeout
```

**Expected Output**:
```cpp
{3002, PRESS, 700},   // Uses second action (overflow from 50 taps)
{3002, RELEASE, 700}
```

---

## Test 10.2: Zero-Duration Input Patterns

**Objective**: Verify handling of instantaneous press/release patterns

**Configuration**: Same as Test 10.1

**Test 10.2a - Zero Duration Single Tap**:
```cpp
tap_key(TAP_DANCE_KEY, 0);       // t=0ms (instantaneous)
platform_wait_ms(200);          // t=200ms
Expected: {3001, PRESS, 0}, {3001, RELEASE, 200} (immediate execution)
```

**Test 10.2b - Zero Duration Multi-Tap**:
```cpp
tap_key(TAP_DANCE_KEY, 0);       // t=0ms
tap_key(TAP_DANCE_KEY, 0, 0);    // t=0ms (simultaneous)
tap_key(TAP_DANCE_KEY, 0, 0);    // t=0ms (simultaneous)
platform_wait_ms(200);          // t=200ms
Expected: {3002, PRESS, 200}, {3002, RELEASE, 200} (3 simultaneous taps)
```

**Test 10.2c - Zero Duration with Hold Attempt**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
release_key(TAP_DANCE_KEY, 0);   // t=0ms (zero duration)
platform_wait_ms(200);          // t=200ms
Expected: {3001, PRESS, 200}, {3001, RELEASE, 200} (tap, not hold)
```

---

## Test 10.3: Extreme Timeout Boundary Testing

**Objective**: Verify precise behavior at extreme timing boundaries

**Configuration**: Same as Test 10.1

**Test 10.3a - Microsecond Precision Before Timeout**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
// Release at exactly 199.999ms (if system supports sub-ms)
release_key(TAP_DANCE_KEY, 199); // t=199ms (closest to boundary)
platform_wait_ms(200);          // t=399ms
Expected: Tap action (released before timeout)
```

**Test 10.3b - Simultaneous Timeout Events**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
release_key(TAP_DANCE_KEY, 100); // t=100ms
// Next press at exactly tap timeout moment
press_key(TAP_DANCE_KEY, 200);   // t=300ms (exactly at timeout)
release_key(TAP_DANCE_KEY, 50);  // t=350ms
platform_wait_ms(200);          // t=550ms
Expected: Two separate single-tap sequences
```

---

## Test 10.4: Maximum Tap Count Stress Test

**Objective**: Verify system handles extremely high tap counts

**Configuration**: Same as Test 10.1

**Input Sequence**:
```cpp
// 1000 taps in sequence
for (int i = 0; i < 1000; i++) {
    tap_key(TAP_DANCE_KEY, 5);
    platform_wait_ms(10);        // Stay within tap timeout
}
platform_wait_ms(200);           // Final sequence completion
```

**Expected Output**:
```cpp
{3002, PRESS, 15200},   // Uses second action (overflow from 1000 taps)
{3002, RELEASE, 15200}
```

---

## Test 10.5: Interleaved Multi-Key Stress Test

**Objective**: Verify system handles multiple concurrent tap dance sequences

**Configuration**:
```cpp
TAP_DANCE_KEY_1 = 3000
TAP_DANCE_KEY_2 = 3100
TAP_DANCE_KEY_3 = 3200
All configured with: Tap[1,2,3: SENDKEY], Hold[1: LAYER]
```

**Input Sequence**:
```cpp
// Interleaved rapid sequences on all three keys
for (int i = 0; i < 20; i++) {
    tap_key(TAP_DANCE_KEY_1, 3);
    tap_key(TAP_DANCE_KEY_2, 3, 3);
    tap_key(TAP_DANCE_KEY_3, 3, 3);
    platform_wait_ms(15);        // 24ms per cycle
}
platform_wait_ms(200);           // All sequences complete
```

**Expected Output**:
```cpp
// All three keys should produce their respective overflow actions
{KEY1_OUTPUT, PRESS, 680}, {KEY1_OUTPUT, RELEASE, 680},
{KEY2_OUTPUT, PRESS, 680}, {KEY2_OUTPUT, RELEASE, 680},
{KEY3_OUTPUT, PRESS, 680}, {KEY3_OUTPUT, RELEASE, 680}
```

---

## Test 10.6: State Recovery After Interruption Storm

**Objective**: Verify system recovers properly after complex interruption patterns

**Configuration**:
```cpp
Strategy: BALANCED
Tap actions: [1: SENDKEY(3001)]
Hold actions: [1: CHANGELAYER(1)]
INTERRUPTING_KEYS = [3010, 3011, 3012, 3013, 3014] // 5 interrupting keys
Other settings same as Test 10.1
```

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);            // t=0ms
// Rapid fire interruptions
for (int i = 0; i < 5; i++) {
    press_key(3010 + i, i * 10);     // Staggered presses
    release_key(3010 + i, 20);       // Staggered releases
}
release_key(TAP_DANCE_KEY, 50);      // t=150ms

// Immediate second sequence - should start clean
press_key(TAP_DANCE_KEY, 10);        // t=160ms
release_key(TAP_DANCE_KEY, 50);      // t=210ms
platform_wait_ms(200);              // t=410ms
```

**Expected Output**:
```cpp
// First sequence - hold triggered by first complete interruption cycle
{3010, PRESS, 0}, {3010, RELEASE, 20},
{LAYER_1, ACTIVATE, 20}, 
// ... (other interrupting keys)
{LAYER_1, DEACTIVATE, 150},
// Second sequence - clean tap
{3001, PRESS, 410}, {3001, RELEASE, 410}
```

---

## Test 10.7: Memory Pressure Simulation

**Objective**: Verify system maintains state correctly under memory constraints

**Configuration**: Same as Test 10.1

**Input Sequence**:
```cpp
// Create many incomplete sequences to test state management
for (int i = 0; i < 100; i++) {
    press_key(TAP_DANCE_KEY);        // Start sequence
    release_key(TAP_DANCE_KEY, 50);  // Enter WAITING_FOR_TAP
    platform_wait_ms(100);          // Partial timeout
    // Leave sequence incomplete (will timeout later)
    if (i < 99) platform_wait_ms(50); // Stagger timeouts
}
platform_wait_ms(300);              // All sequences should timeout
```

**Expected Output**:
```cpp
// 100 separate tap actions as sequences timeout
// Pattern: {3001, PRESS, t}, {3001, RELEASE, t} for each sequence
// where t varies based on individual sequence timing
```

---

## Test 10.8: Clock Rollover Boundary Test

**Objective**: Verify timing calculations work correctly across clock boundaries

**Configuration**: Same as Test 10.1

**Input Sequence**:
```cpp
// Simulate near clock rollover (implementation dependent)
platform_wait_ms(65500);            // Near 16-bit rollover point
press_key(TAP_DANCE_KEY);            // Start sequence near rollover
platform_wait_ms(100);              // Cross potential rollover
release_key(TAP_DANCE_KEY);          // Complete sequence
platform_wait_ms(200);              // Ensure completion
```

**Expected Output**:
```cpp
{3001, PRESS, 65800},   // Timing should remain consistent across rollover
{3001, RELEASE, 65800}
```

---

## Test 10.9: Pathological Input Patterns

**Objective**: Verify system handles unusual but valid input patterns

**Configuration**: Same as Test 10.1

**Test 10.9a - Alternating Rapid Press/Release**:
```cpp
for (int i = 0; i < 20; i++) {
    press_key(TAP_DANCE_KEY);
    platform_wait_ms(5);
    release_key(TAP_DANCE_KEY);
    platform_wait_ms(5);            // 10ms cycles
}
platform_wait_ms(200);
Expected: High tap count sequence completion
```

**Test 10.9b - Sawtooth Timing Pattern**:
```cpp
for (int i = 0; i < 10; i++) {
    tap_key(TAP_DANCE_KEY, i * 10);  // Increasing hold durations
    platform_wait_ms(50 - i * 2);   // Decreasing gaps
}
platform_wait_ms(200);
Expected: Consistent sequence behavior regardless of timing pattern
```

---

## Test 10.10: Extreme Configuration Stress Test

**Objective**: Verify system handles maximum practical configuration complexity

**Configuration**:
```cpp
TAP_DANCE_KEY = 3000
Strategy: BALANCED
// Maximum practical configuration
Tap actions: [1-50: various SENDKEY actions]
Hold actions: [1-50: various LAYER actions]
Hold timeout: 200ms, Tap timeout: 200ms
INTERRUPTING_KEY = 3010
```

**Input Sequence**:
```cpp
// Attempt to reach 50th tap count
for (int i = 0; i < 50; i++) {
    tap_key(TAP_DANCE_KEY, 10);
    platform_wait_ms(20);
}
platform_wait_ms(200);

// Then test 50th hold action
for (int i = 0; i < 49; i++) {
    tap_key(TAP_DANCE_KEY, 10);
    platform_wait_ms(20);
}
press_key(TAP_DANCE_KEY, 20);
platform_wait_ms(250);
release_key(TAP_DANCE_KEY);
```

**Expected Output**:
```cpp
{TAP_50_OUTPUT, PRESS, 1200}, {TAP_50_OUTPUT, RELEASE, 1200},
{LAYER_50, ACTIVATE, 2180}, {LAYER_50, DEACTIVATE, 2430}
```

---

## Test 10.11: Error Recovery and Graceful Degradation

**Objective**: Verify system handles potential error conditions gracefully

**Configuration**: Same as Test 10.1

**Test 10.11a - Invalid Key Sequence Recovery**:
```cpp
// Simulate potential invalid state
press_key(TAP_DANCE_KEY);        // t=0ms
// Simulate system reset/restart mid-sequence
// System should recover gracefully
press_key(TAP_DANCE_KEY, 100);   // t=100ms (new sequence)
release_key(TAP_DANCE_KEY, 50);  // t=150ms
platform_wait_ms(200);          // t=350ms
Expected: Clean single-tap execution (recovery)
```

**Test 10.11b - Overflow Beyond System Limits**:
```cpp
// Test theoretical maximum tap count
for (int i = 0; i < 10000; i++) {
    tap_key(TAP_DANCE_KEY, 1);
    platform_wait_ms(2);         // Minimal timing
}
platform_wait_ms(200);
Expected: System maintains stability (uses overflow behavior)
```

---

## Test 10.12: Timing Jitter Simulation

**Objective**: Verify system handles timing variations gracefully

**Configuration**: Same as Test 10.1

**Input Sequence**:
```cpp
// Simulate timing jitter around boundaries
press_key(TAP_DANCE_KEY);        // t=0ms
// Release with small random jitter around timeout
release_key(TAP_DANCE_KEY, 199 + (random() % 3)); // 199-201ms
platform_wait_ms(200 + (random() % 5));           // Jittered timeout
```

**Expected Output**:
```cpp
// Behavior should be deterministic based on actual timing
// Either tap or hold based on whether 200ms boundary crossed
```

---

## Test 10.13: Resource Exhaustion Simulation

**Objective**: Verify system behavior under resource constraints

**Configuration**: Multiple keys configured

**Input Sequence**:
```cpp
// Start many sequences simultaneously
for (int key = 3000; key < 3100; key++) {
    press_key(key);               // 100 concurrent sequences
    platform_wait_ms(1);
}
// Complete all sequences
for (int key = 3000; key < 3100; key++) {
    release_key(key, 100);
    platform_wait_ms(1);
}
platform_wait_ms(200);
```

**Expected Output**:
```cpp
// All sequences should complete correctly
// System should handle concurrent load gracefully
```

---

## Test 10.14: Edge Case Recovery Verification

**Objective**: Verify system recovers from all edge cases tested

**Configuration**: Same as Test 10.1

**Input Sequence**:
```cpp
// After any edge case test, verify normal operation
platform_wait_ms(1000);         // Ensure clean state
tap_key(TAP_DANCE_KEY, 50);      // Simple tap
platform_wait_ms(200);
press_key(TAP_DANCE_KEY);        // Simple hold
platform_wait_ms(250);
release_key(TAP_DANCE_KEY);
```

**Expected Output**:
```cpp
{3001, PRESS, 1200}, {3001, RELEASE, 1200},     // Normal tap
{LAYER_1, ACTIVATE, 1450}, {LAYER_1, DEACTIVATE, 1700} // Normal hold
```

---

## Test 10.15: Comprehensive Stress Test Integration

**Objective**: Combine multiple stress factors in single test

**Configuration**:
```cpp
Strategy: BALANCED  
Tap actions: [1-10: various SENDKEY]
Hold actions: [1-5: various LAYER]
Multiple interrupting keys
Custom timeouts: Hold 150ms, Tap 250ms
```

**Input Sequence**:
```cpp
// Combined stress: rapid multi-tap, interruptions, timing boundaries
for (int i = 0; i < 10; i++) {
    tap_key(TAP_DANCE_KEY, 10);             // Rapid taps
    if (i % 3 == 0) {
        press_key(3010 + (i % 3), 5);       // Periodic interruptions
        release_key(3010 + (i % 3), 10);
    }
    platform_wait_ms(20);
}
// Final hold test with interruption
press_key(TAP_DANCE_KEY, 30);
press_key(3010, 50);
release_key(3010, 50);
release_key(TAP_DANCE_KEY, 100);
```

**Expected Output**:
```cpp
// Complex but deterministic output based on:
// - 10 rapid taps (overflow behavior)
// - Interruption effects per BALANCED strategy
// - Hold action from final sequence
// System should maintain consistency throughout
```

---

## Test 10.16: Final System Integrity Check

**Objective**: Verify system maintains integrity after all stress tests

**Configuration**: Basic configuration

**Input Sequence**:
```cpp
// Simple verification sequence
tap_key(TAP_DANCE_KEY, 50);      // Basic tap
platform_wait_ms(300);          // Clean gap
press_key(TAP_DANCE_KEY);        // Basic hold
platform_wait_ms(250);
release_key(TAP_DANCE_KEY);
```

**Expected Output**:
```cpp
{3001, PRESS, t1}, {3001, RELEASE, t1},         // Perfect tap
{LAYER_1, ACTIVATE, t2}, {LAYER_1, DEACTIVATE, t3} // Perfect hold
// Timing should be precise and predictable
```

---

## Summary: Edge Case Coverage

**Stress Factors Tested**:
- Extreme timing (zero duration, rapid fire, large timeouts)
- High volume (1000+ taps, 100+ concurrent sequences)
- Boundary conditions (clock rollover, timeout precision)
- Resource constraints (memory pressure, concurrent load)
- Recovery scenarios (error conditions, state cleanup)
- Complex combinations (multiple stress factors)

**System Properties Verified**:
- Timing precision and consistency
- State management integrity
- Resource usage efficiency
- Error recovery capabilities
- Performance under extreme load
- Behavioral predictability across all conditions