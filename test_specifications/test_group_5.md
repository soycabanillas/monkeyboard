# Test Group 5: Immediate vs. Delayed Execution

This group tests the execution mode determination logic, state machine bypass conditions, and timing verification for immediate vs. delayed execution scenarios.

## Test 5.1: Immediate Execution - No Hold Action Configured

**Objective**: Verify immediate execution when no hold action is available for current tap count

**Configuration**:
```cpp
TAP_DANCE_KEY = 3000
Strategy: TAP_PREFERRED
Tap actions: [1: SENDKEY(3001)]
Hold actions: [] // No hold actions configured
Hold timeout: 200ms, Tap timeout: 200ms
```

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
release_key(TAP_DANCE_KEY, 100); // t=100ms
```

**Expected Output**:
```cpp
{3001, PRESS, 0},     // Immediate execution on key press
{3001, RELEASE, 100}  // Release follows input timing exactly
```

---

## Test 5.2: Delayed Execution - Hold Action Available

**Objective**: Verify delayed execution when hold action is configured for current tap count

**Configuration**:
```cpp
TAP_DANCE_KEY = 3000
Strategy: TAP_PREFERRED
Tap actions: [1: SENDKEY(3001)]
Hold actions: [1: CHANGELAYER(1)] // Hold action available
Hold timeout: 200ms, Tap timeout: 200ms
```

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
release_key(TAP_DANCE_KEY, 100); // t=100ms (before hold timeout)
platform_wait_ms(200);          // t=300ms (tap timeout)
```

**Expected Output**:
```cpp
{3001, PRESS, 300},   // Delayed execution at tap timeout
{3001, RELEASE, 300}
```

---

## Test 5.3: State Machine Bypass - Deterministic Outcome

**Objective**: Verify state machine is bypassed when outcome is deterministic

**Configuration**: Same as Test 5.1 (no hold actions)

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
platform_wait_ms(300);          // t=300ms (hold key well beyond timeout)
release_key(TAP_DANCE_KEY);      // t=300ms
```

**Expected Output**:
```cpp
{3001, PRESS, 0},     // Immediate on press (deterministic - no hold possible)
{3001, RELEASE, 300}  // Release follows input timing
```

---

## Test 5.4: Delayed Execution - Hold Timeout Reached

**Objective**: Verify delayed execution resolves to hold action when timeout reached

**Configuration**: Same as Test 5.2

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
platform_wait_ms(250);          // t=250ms (exceed hold timeout)
release_key(TAP_DANCE_KEY);      // t=250ms
```

**Expected Output**:
```cpp
{LAYER_1, ACTIVATE, 200},   // Hold action at timeout (delayed execution)
{LAYER_1, DEACTIVATE, 250}
```

---

## Test 5.5: Execution Mode Transition - Multi-Tap Sequence

**Objective**: Verify execution mode can change within a single multi-tap sequence

**Configuration**:
```cpp
Tap actions: [1: SENDKEY(3001), 2: SENDKEY(3002)]
Hold actions: [1: CHANGELAYER(1)] // Hold only for 1st tap
Other settings same as Test 5.2
```

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms (1st tap - hold available, delayed)
release_key(TAP_DANCE_KEY, 100); // t=100ms
press_key(TAP_DANCE_KEY, 50);    // t=150ms (2nd tap - no hold, immediate)
release_key(TAP_DANCE_KEY, 100); // t=250ms
platform_wait_ms(200);          // t=450ms
```

**Expected Output**:
```cpp
{3002, PRESS, 450},   // Delayed execution (whole sequence evaluated)
{3002, RELEASE, 450}
```

---

## Test 5.6: Immediate Execution - Overflow with SENDKEY Only

**Objective**: Verify immediate execution in overflow when no hold actions available

**Configuration**:
```cpp
Tap actions: [1: SENDKEY(3001), 2: SENDKEY(3002)]
Hold actions: [] // No hold actions
Other settings same as Test 5.2
```

**Input Sequence**:
```cpp
tap_key(TAP_DANCE_KEY, 30);      // t=0-30ms (1st tap)
tap_key(TAP_DANCE_KEY, 40, 30);  // t=70-110ms (2nd tap)
press_key(TAP_DANCE_KEY, 40);    // t=150ms (3rd tap - overflow)
release_key(TAP_DANCE_KEY, 100); // t=250ms
```

**Expected Output**:
```cpp
{3002, PRESS, 150},   // Immediate execution on overflow press
{3002, RELEASE, 250}  // Release follows input
```

---

## Test 5.7: Delayed Execution - Overflow with Hold Available

**Objective**: Verify delayed execution in overflow when hold actions exist at lower counts

**Configuration**:
```cpp
Tap actions: [1: SENDKEY(3001), 2: SENDKEY(3002)]
Hold actions: [1: CHANGELAYER(1), 2: CHANGELAYER(2)] // Hold available at lower counts
Other settings same as Test 5.2
```

**Input Sequence**:
```cpp
tap_key(TAP_DANCE_KEY, 30);      // t=0-30ms
tap_key(TAP_DANCE_KEY, 40, 30);  // t=70-110ms
tap_key(TAP_DANCE_KEY, 40, 30);  // t=150-180ms (3rd tap - overflow, but delayed)
platform_wait_ms(200);          // t=380ms
```

**Expected Output**:
```cpp
{3002, PRESS, 380},   // Delayed execution (hold exists at lower counts)
{3002, RELEASE, 380}
```

---

## Test 5.8: Immediate Execution Timing Precision

**Objective**: Verify immediate execution happens exactly at key press timing

**Configuration**: Same as Test 5.1

**Input Sequence**:
```cpp
platform_wait_ms(100);          // t=100ms (establish baseline)
press_key(TAP_DANCE_KEY);        // t=100ms
platform_wait_ms(50);           // t=150ms
release_key(TAP_DANCE_KEY);      // t=150ms
```

**Expected Output**:
```cpp
{3001, PRESS, 100},   // Exactly at press time
{3001, RELEASE, 150}  // Exactly at release time
```

---

## Test 5.9: Delayed Execution Timing Precision

**Objective**: Verify delayed execution happens exactly at timeout boundaries

**Configuration**: Same as Test 5.2

**Input Sequence**:
```cpp
platform_wait_ms(100);          // t=100ms (establish baseline)
press_key(TAP_DANCE_KEY);        // t=100ms
release_key(TAP_DANCE_KEY, 50);  // t=150ms (before hold timeout)
platform_wait_ms(200);          // t=350ms (tap timeout from release)
```

**Expected Output**:
```cpp
{3001, PRESS, 350},   // Exactly at tap timeout (150ms + 200ms)
{3001, RELEASE, 350}
```

---

## Test 5.10: Mixed Execution Modes - Strategy Integration

**Objective**: Verify execution mode determination with different hold strategies

**Configuration**:
```cpp
Strategy: HOLD_PREFERRED
Tap actions: [1: SENDKEY(3001), 2: SENDKEY(3002)]
Hold actions: [1: CHANGELAYER(1)] // Hold only for 1st tap
INTERRUPTING_KEY = 3010
Other settings same as Test 5.2
```

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);          // t=0ms (1st tap - hold available)
release_key(TAP_DANCE_KEY, 50);    // t=50ms
press_key(TAP_DANCE_KEY, 50);      // t=100ms (2nd tap - no hold available)
press_key(INTERRUPTING_KEY, 50);   // t=150ms (interrupt - would trigger hold if available)
release_key(INTERRUPTING_KEY, 50); // t=200ms
release_key(TAP_DANCE_KEY, 50);    // t=250ms
platform_wait_ms(200);            // t=450ms
```

**Expected Output**:
```cpp
{INTERRUPTING_KEY, PRESS, 150},
{3002, PRESS, 450},              // Delayed execution (sequence level decision)
{3002, RELEASE, 450},
{INTERRUPTING_KEY, RELEASE, 200}
```

---

## Test 5.11: Execution Mode Decision Table Verification

**Objective**: Systematically verify all execution mode decision conditions

**Test 5.11a - Immediate: No hold, within config**:
```cpp
Config: Tap[1: SENDKEY], Hold[]
Input: 1 tap
Expected: Immediate execution
```

**Test 5.11b - Immediate: No hold, overflow**:
```cpp
Config: Tap[1: SENDKEY], Hold[]
Input: 3 taps (overflow)
Expected: Immediate execution
```

**Test 5.11c - Delayed: Hold available, within config**:
```cpp
Config: Tap[1: SENDKEY], Hold[1: LAYER]
Input: 1 tap
Expected: Delayed execution
```

**Test 5.11d - Delayed: Hold at lower count, overflow**:
```cpp
Config: Tap[1,2: SENDKEY], Hold[1: LAYER]
Input: 3 taps (overflow, but hold exists at count 1)
Expected: Delayed execution
```

---

## Test 5.12: State Machine Bypass Verification

**Objective**: Verify internal state machine is actually bypassed in immediate execution

**Configuration**: Same as Test 5.1

**Input Sequence**:
```cpp
// Rapid sequence that would normally require state machine
press_key(TAP_DANCE_KEY);        // t=0ms
release_key(TAP_DANCE_KEY, 10);  // t=10ms
press_key(TAP_DANCE_KEY, 10);    // t=20ms (rapid second press)
release_key(TAP_DANCE_KEY, 10);  // t=30ms
```

**Expected Output**:
```cpp
{3001, PRESS, 0},     // First immediate execution
{3001, RELEASE, 10},
{3001, PRESS, 20},    // Second immediate execution (bypassed)
{3001, RELEASE, 30}
```

---

## Test 5.13: Execution Responsiveness Comparison

**Objective**: Compare response timing between immediate and delayed execution

**Immediate Execution Test**:
```cpp
Config: No hold actions
press_key(TAP_DANCE_KEY);        // t=0ms
// Expected output at t=0ms (immediate)
```

**Delayed Execution Test**:
```cpp
Config: Hold action available
press_key(TAP_DANCE_KEY);        // t=0ms
release_key(TAP_DANCE_KEY, 100); // t=100ms
// Expected output at t=300ms (after tap timeout)
```

**Timing Difference**: 300ms delay for evaluation vs 0ms immediate response

---

## Test 5.14: Execution Mode with Zero-Duration Actions

**Objective**: Verify execution mode handling with instantaneous press/release

**Configuration**: Same as Test 5.1 (immediate execution)

**Input Sequence**:
```cpp
tap_key(TAP_DANCE_KEY, 0);       // t=0ms (instant press+release)
```

**Expected Output**:
```cpp
{3001, PRESS, 0},     // Immediate execution on press
{3001, RELEASE, 0}    // Immediate release
```

---

## Test 5.15: Complex Execution Mode Scenario

**Objective**: Verify execution mode determination in complex multi-tap with mixed availability

**Configuration**:
```cpp
Tap actions: [1: SENDKEY(3001), 2: SENDKEY(3002), 3: SENDKEY(3003), 4: SENDKEY(3004)]
Hold actions: [2: CHANGELAYER(2), 4: CHANGELAYER(4)] // Hold at 2nd and 4th only
Other settings same as Test 5.2
```

**Scenarios**:

**5.15a - 1st tap (no hold) - Immediate expected**:
```cpp
tap_key(TAP_DANCE_KEY, 50);
Expected: Immediate execution
```

**5.15b - 2nd tap (hold available) - Delayed expected**:
```cpp
tap_key(TAP_DANCE_KEY, 30);
tap_key(TAP_DANCE_KEY, 50, 30);
Expected: Delayed execution
```

**5.15c - 3rd tap (no hold at this count, but exists at others) - Delayed expected**:
```cpp
tap_key(TAP_DANCE_KEY, 30);
tap_key(TAP_DANCE_KEY, 40, 30);
tap_key(TAP_DANCE_KEY, 50, 30);
Expected: Delayed execution (hold exists in sequence)
```

**5.15d - 5th tap (overflow, hold exists at 4th) - Delayed expected**:
```cpp
// 5 taps total
Expected: Delayed execution (hold exists at lower count)
```