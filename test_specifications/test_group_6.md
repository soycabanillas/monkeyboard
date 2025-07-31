# Test Group 6: Timing Boundary Conditions

This group tests precise timing behavior at timeout boundaries, race conditions, and timing edge cases with millisecond precision.

## Test 6.1: Hold Timeout Boundary - 1ms Before

**Objective**: Verify tap behavior when released exactly 1ms before hold timeout

**Configuration**:
```cpp
TAP_DANCE_KEY = 3000
Strategy: TAP_PREFERRED
Tap actions: [1: SENDKEY(3001)]
Hold actions: [1: CHANGELAYER(1)]
Hold timeout: 200ms, Tap timeout: 200ms
```

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
release_key(TAP_DANCE_KEY, 199); // t=199ms (1ms before hold timeout)
platform_wait_ms(200);          // t=399ms (tap timeout)
```

**Expected Output**:
```cpp
{3001, PRESS, 399},   // Tap action (released before hold timeout)
{3001, RELEASE, 399}
```

---

## Test 6.2: Hold Timeout Boundary - Exactly At

**Objective**: Verify hold behavior when timeout occurs exactly at boundary

**Configuration**: Same as Test 6.1

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
platform_wait_ms(200);          // t=200ms (exactly at hold timeout)
release_key(TAP_DANCE_KEY);      // t=200ms
```

**Expected Output**:
```cpp
{LAYER_1, ACTIVATE, 200},   // Hold action (timeout reached exactly)
{LAYER_1, DEACTIVATE, 200}
```

---

## Test 6.3: Hold Timeout Boundary - 1ms After

**Objective**: Verify hold behavior when held 1ms past timeout

**Configuration**: Same as Test 6.1

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
release_key(TAP_DANCE_KEY, 201); // t=201ms (1ms after hold timeout)
```

**Expected Output**:
```cpp
{LAYER_1, ACTIVATE, 200},   // Hold action at timeout
{LAYER_1, DEACTIVATE, 201}
```

---

## Test 6.4: Tap Timeout Boundary - 1ms Before

**Objective**: Verify sequence continuation when next press occurs 1ms before tap timeout

**Configuration**:
```cpp
Tap actions: [1: SENDKEY(3001), 2: SENDKEY(3002)]
Hold actions: [1: CHANGELAYER(1)]
Other settings same as Test 6.1
```

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
release_key(TAP_DANCE_KEY, 100); // t=100ms
press_key(TAP_DANCE_KEY, 199);   // t=299ms (1ms before tap timeout)
release_key(TAP_DANCE_KEY, 50);  // t=349ms
platform_wait_ms(200);          // t=549ms
```

**Expected Output**:
```cpp
{3002, PRESS, 549},   // Second tap action (sequence continued)
{3002, RELEASE, 549}
```

---

## Test 6.5: Tap Timeout Boundary - Exactly At

**Objective**: Verify sequence reset when tap timeout occurs exactly at boundary

**Configuration**: Same as Test 6.4

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
release_key(TAP_DANCE_KEY, 100); // t=100ms
press_key(TAP_DANCE_KEY, 200);   // t=300ms (exactly at tap timeout)
release_key(TAP_DANCE_KEY, 50);  // t=350ms
platform_wait_ms(200);          // t=550ms
```

**Expected Output**:
```cpp
{3001, PRESS, 300},   // First sequence completes (timeout reached)
{3001, RELEASE, 300},
{3001, PRESS, 550},   // Second sequence starts fresh
{3001, RELEASE, 550}
```

---

## Test 6.6: Tap Timeout Boundary - 1ms After

**Objective**: Verify sequence reset when next press occurs 1ms after tap timeout

**Configuration**: Same as Test 6.4

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
release_key(TAP_DANCE_KEY, 100); // t=100ms
press_key(TAP_DANCE_KEY, 201);   // t=301ms (1ms after tap timeout)
release_key(TAP_DANCE_KEY, 50);  // t=351ms
platform_wait_ms(200);          // t=551ms
```

**Expected Output**:
```cpp
{3001, PRESS, 300},   // First sequence completes at timeout
{3001, RELEASE, 300},
{3001, PRESS, 551},   // Second sequence (fresh start)
{3001, RELEASE, 551}
```

---

## Test 6.7: Race Condition - Hold vs Tap Timeout

**Objective**: Verify behavior when hold and tap timeouts could occur simultaneously

**Configuration**:
```cpp
Hold timeout: 200ms, Tap timeout: 200ms  // Same timeout values
Other settings same as Test 6.4
```

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
release_key(TAP_DANCE_KEY, 100); // t=100ms (start tap timeout)
// Next press at exactly when both timeouts could occur
press_key(TAP_DANCE_KEY, 200);   // t=300ms (tap timeout + hold start)
platform_wait_ms(200);          // t=500ms (hold timeout)
release_key(TAP_DANCE_KEY);      // t=500ms
```

**Expected Output**:
```cpp
{3001, PRESS, 300},         // Tap timeout wins (sequence completes first)
{3001, RELEASE, 300},
{LAYER_1, ACTIVATE, 500},   // New sequence hold action
{LAYER_1, DEACTIVATE, 500}
```

---

## Test 6.8: Race Condition - Strategy vs Timeout

**Objective**: Verify strategy behavior when interruption and timeout occur near simultaneously

**Configuration**:
```cpp
Strategy: BALANCED
Hold timeout: 200ms
INTERRUPTING_KEY = 3010
Other settings same as Test 6.1
```

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);          // t=0ms
press_key(INTERRUPTING_KEY, 199);  // t=199ms (1ms before timeout)
release_key(INTERRUPTING_KEY, 2);  // t=201ms (complete cycle after timeout)
release_key(TAP_DANCE_KEY, 49);    // t=250ms
```

**Expected Output**:
```cpp
{INTERRUPTING_KEY, PRESS, 199},
{LAYER_1, ACTIVATE, 200},          // Hold timeout wins the race
{INTERRUPTING_KEY, RELEASE, 201},
{LAYER_1, DEACTIVATE, 250}
```

---

## Test 6.9: Rapid Sequence Timing - Sub-Timeout Windows

**Objective**: Verify system handles rapid sequences well within timeout windows

**Configuration**: Same as Test 6.4

**Input Sequence**:
```cpp
// Very rapid sequence - all within first 50ms
press_key(TAP_DANCE_KEY);        // t=0ms
release_key(TAP_DANCE_KEY, 10);  // t=10ms
press_key(TAP_DANCE_KEY, 10);    // t=20ms
release_key(TAP_DANCE_KEY, 10);  // t=30ms
press_key(TAP_DANCE_KEY, 10);    // t=40ms
release_key(TAP_DANCE_KEY, 10);  // t=50ms
platform_wait_ms(200);          // t=250ms
```

**Expected Output**:
```cpp
{3002, PRESS, 250},   // Second tap action (rapid 3-tap sequence uses 2nd action overflow)
{3002, RELEASE, 250}
```

---

## Test 6.10: Timing Precision - Millisecond Accuracy

**Objective**: Verify system maintains millisecond timing precision

**Configuration**: Same as Test 6.1

**Input Sequence**:
```cpp
platform_wait_ms(1000);         // t=1000ms (establish high baseline)
press_key(TAP_DANCE_KEY);        // t=1000ms
release_key(TAP_DANCE_KEY, 150); // t=1150ms
platform_wait_ms(200);          // t=1350ms
```

**Expected Output**:
```cpp
{3001, PRESS, 1350},   // Precise timing maintained
{3001, RELEASE, 1350}
```

---

## Test 6.11: Multiple Timeout Windows - Sequence Chain

**Objective**: Verify correct timeout calculation across multiple tap timeout windows

**Configuration**: Same as Test 6.4

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
release_key(TAP_DANCE_KEY, 50);  // t=50ms
// Wait near tap timeout, then continue
press_key(TAP_DANCE_KEY, 195);   // t=245ms (within first tap timeout)
release_key(TAP_DANCE_KEY, 50);  // t=295ms
// Wait near second tap timeout, then continue
press_key(TAP_DANCE_KEY, 195);   // t=490ms (within second tap timeout)
release_key(TAP_DANCE_KEY, 50);  // t=540ms
platform_wait_ms(200);          // t=740ms
```

**Expected Output**:
```cpp
{3002, PRESS, 740},   // Third tap uses second action (overflow)
{3002, RELEASE, 740}
```

---

## Test 6.12: Timeout Accumulation - Long Sequence

**Objective**: Verify timeout calculations don't accumulate errors over long sequences

**Configuration**: Same as Test 6.4

**Input Sequence**:
```cpp
// 5 taps, each at 180ms intervals (within tap timeout)
for (int i = 0; i < 5; i++) {
    press_key(TAP_DANCE_KEY);        // t=i*180ms
    release_key(TAP_DANCE_KEY, 50);  // t=i*180+50ms
    platform_wait_ms(130);          // t=i*180+180ms
}
platform_wait_ms(200);              // Final timeout
```

**Expected Output**:
```cpp
{3002, PRESS, 1100},   // Uses second action (overflow from 5 taps)
{3002, RELEASE, 1100}
```

---

## Test 6.13: Zero-Duration Edge Cases

**Objective**: Verify timing behavior with zero-duration key presses

**Configuration**: Same as Test 6.1

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
release_key(TAP_DANCE_KEY, 0);   // t=0ms (zero duration)
press_key(TAP_DANCE_KEY, 0);     // t=0ms (immediate second press)
release_key(TAP_DANCE_KEY, 100); // t=100ms
platform_wait_ms(200);          // t=300ms
```

**Expected Output**:
```cpp
{3002, PRESS, 300},   // Second tap action (two zero-duration taps)
{3002, RELEASE, 300}
```

---

## Test 6.14: Timeout Boundary with Strategy Integration

**Objective**: Verify timing boundaries work correctly with different hold strategies

**Configuration**:
```cpp
Strategy: HOLD_PREFERRED
Hold timeout: 200ms
INTERRUPTING_KEY = 3010
Other settings same as Test 6.1
```

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);          // t=0ms
// Interrupt exactly at hold timeout
press_key(INTERRUPTING_KEY, 200);  // t=200ms (exactly at timeout)
release_key(INTERRUPTING_KEY, 1);  // t=201ms
release_key(TAP_DANCE_KEY, 49);    // t=250ms
```

**Expected Output**:
```cpp
{INTERRUPTING_KEY, PRESS, 200},
{LAYER_1, ACTIVATE, 200},          // Timeout and strategy both trigger hold
{INTERRUPTING_KEY, RELEASE, 201},
{LAYER_1, DEACTIVATE, 250}
```

---

## Test 6.15: Complex Timing Scenario - Mixed Boundaries

**Objective**: Verify system handles complex timing with multiple near-boundary conditions

**Configuration**:
```cpp
Strategy: BALANCED
Tap actions: [1: SENDKEY(3001), 2: SENDKEY(3002), 3: SENDKEY(3003)]
Hold actions: [1: CHANGELAYER(1), 2: CHANGELAYER(2)]
Hold timeout: 200ms, Tap timeout: 200ms
INTERRUPTING_KEY = 3010
```

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);          // t=0ms (1st tap)
release_key(TAP_DANCE_KEY, 199);   // t=199ms (1ms before hold timeout)

press_key(TAP_DANCE_KEY, 1);       // t=200ms (2nd tap, exactly at first timeout)
press_key(INTERRUPTING_KEY, 199);  // t=399ms (1ms before second hold timeout)
release_key(INTERRUPTING_KEY, 2);  // t=401ms (complete cycle after timeout)
release_key(TAP_DANCE_KEY, 49);    // t=450ms
```

**Expected Output**:
```cpp
{INTERRUPTING_KEY, PRESS, 399},
{LAYER_2, ACTIVATE, 400},          // Hold timeout for 2nd tap wins
{INTERRUPTING_KEY, RELEASE, 401},
{LAYER_2, DEACTIVATE, 450}
```

---

## Test 6.16: Timing Boundary Documentation - Reference Values

**Objective**: Document exact timing behavior for reference (not executable test)

**Timing Rules Verification**:
- Hold timeout: Action triggers at exactly 200ms from key press
- Tap timeout: Sequence resets at exactly 200ms from key release
- Race conditions: Earlier event (by timestamp) takes precedence
- Boundary conditions: >= timeout value triggers timeout behavior
- Precision: System maintains millisecond accuracy

**Critical Boundaries**:
- 199ms: Still before timeout
- 200ms: Exactly at timeout (triggers)
- 201ms: After timeout (already triggered)

**Sequence Timing**:
- Each tap starts new hold timeout window
- Tap timeout starts from each key release
- Timeouts are independent per tap within sequence