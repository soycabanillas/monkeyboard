# Test Group 2: Hold Strategy Behavior

This group tests the three hold strategies and their interruption handling behavior.

## Test 2.1: TAP_PREFERRED - Interruption Ignored (Basic)

**Objective**: Verify TAP_PREFERRED ignores interrupting keys and only uses timeout

**Configuration**:
```cpp
TAP_DANCE_KEY = 3000
OUTPUT_KEY = 3001
INTERRUPTING_KEY = 3002
Strategy: TAP_PREFERRED
Tap actions: [1: SENDKEY(OUTPUT_KEY)]
Hold actions: [1: CHANGELAYER(1)]
Hold timeout: 200ms, Tap timeout: 200ms
```

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);          // t=0ms
press_key(INTERRUPTING_KEY, 50);   // t=50ms (interrupt)
release_key(INTERRUPTING_KEY, 50); // t=100ms
release_key(TAP_DANCE_KEY, 50);    // t=150ms (before hold timeout)
platform_wait_ms(200);            // Wait for tap timeout
```

**Expected Output**:
```cpp
{INTERRUPTING_KEY, PRESS, 50},     // Interrupting key processed normally
{INTERRUPTING_KEY, RELEASE, 100},
{OUTPUT_KEY, PRESS, 350},          // Tap action (interruption ignored)
{OUTPUT_KEY, RELEASE, 350}
```

---

## Test 2.2: TAP_PREFERRED - Hold via Timeout Only

**Objective**: Verify TAP_PREFERRED only triggers hold via timeout, not interruption

**Configuration**: Same as Test 2.1

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);          // t=0ms
press_key(INTERRUPTING_KEY, 50);   // t=50ms (interrupt)
release_key(INTERRUPTING_KEY, 50); // t=100ms
platform_wait_ms(150);            // t=250ms (hold timeout exceeded)
release_key(TAP_DANCE_KEY);        // t=250ms
```

**Expected Output**:
```cpp
{INTERRUPTING_KEY, PRESS, 50},
{INTERRUPTING_KEY, RELEASE, 100},
{LAYER_1, ACTIVATE, 200},          // Hold triggered by timeout only
{LAYER_1, DEACTIVATE, 250}
```

---

## Test 2.3: TAP_PREFERRED - Multiple Interruptions

**Objective**: Verify multiple interruptions are all ignored

**Configuration**: Same as Test 2.1

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);          // t=0ms
press_key(INTERRUPTING_KEY, 30);   // t=30ms
tap_key(3003, 40);                 // t=70ms (another interruption)
release_key(INTERRUPTING_KEY, 30); // t=100ms
release_key(TAP_DANCE_KEY, 50);    // t=150ms
platform_wait_ms(200);
```

**Expected Output**:
```cpp
{INTERRUPTING_KEY, PRESS, 30},
{3003, PRESS, 70},
{3003, RELEASE, 70},
{INTERRUPTING_KEY, RELEASE, 100},
{OUTPUT_KEY, PRESS, 350},          // Still tap (all interruptions ignored)
{OUTPUT_KEY, RELEASE, 350}
```

---

## Test 2.4: BALANCED - Hold on Complete Press/Release Cycle

**Objective**: Verify BALANCED triggers hold when interrupting key completes full cycle

**Configuration**:
```cpp
Strategy: BALANCED
Other settings same as Test 2.1
```

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);          // t=0ms
press_key(INTERRUPTING_KEY, 50);   // t=50ms
release_key(INTERRUPTING_KEY, 50); // t=100ms (complete cycle)
release_key(TAP_DANCE_KEY, 50);    // t=150ms (trigger still held)
```

**Expected Output**:
```cpp
{INTERRUPTING_KEY, PRESS, 50},
{INTERRUPTING_KEY, RELEASE, 100},
{LAYER_1, ACTIVATE, 100},          // Hold triggered by complete cycle
{LAYER_1, DEACTIVATE, 150}
```

---

## Test 2.5: BALANCED - Tap when Trigger Released First

**Objective**: Verify BALANCED triggers tap when trigger key released before interrupting key

**Configuration**: Same as Test 2.4

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);          // t=0ms
press_key(INTERRUPTING_KEY, 50);   // t=50ms
release_key(TAP_DANCE_KEY, 50);    // t=100ms (trigger released first)
release_key(INTERRUPTING_KEY, 50); // t=150ms
platform_wait_ms(200);
```

**Expected Output**:
```cpp
{INTERRUPTING_KEY, PRESS, 50},
{OUTPUT_KEY, PRESS, 100},          // Tap triggered when trigger released
{OUTPUT_KEY, RELEASE, 100},
{INTERRUPTING_KEY, RELEASE, 150}
```

---

## Test 2.6: BALANCED - Incomplete Interruption Cycle

**Objective**: Verify BALANCED behavior when interrupting key pressed but not released

**Configuration**: Same as Test 2.4

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);          // t=0ms
press_key(INTERRUPTING_KEY, 50);   // t=50ms
release_key(TAP_DANCE_KEY, 100);   // t=150ms (trigger released, interrupt still held)
release_key(INTERRUPTING_KEY, 50); // t=200ms
```

**Expected Output**:
```cpp
{INTERRUPTING_KEY, PRESS, 50},
{OUTPUT_KEY, PRESS, 150},          // Tap (incomplete cycle)
{OUTPUT_KEY, RELEASE, 150},
{INTERRUPTING_KEY, RELEASE, 200}
```

---

## Test 2.7: BALANCED - Multiple Interrupting Keys

**Objective**: Verify BALANCED with multiple interrupting keys (first complete cycle wins)

**Configuration**: Same as Test 2.4

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);          // t=0ms
press_key(INTERRUPTING_KEY, 30);   // t=30ms
press_key(3003, 20);               // t=50ms (second interruption)
release_key(INTERRUPTING_KEY, 30); // t=80ms (first completes cycle)
release_key(TAP_DANCE_KEY, 20);    // t=100ms
release_key(3003, 50);             // t=150ms
```

**Expected Output**:
```cpp
{INTERRUPTING_KEY, PRESS, 30},
{3003, PRESS, 50},
{INTERRUPTING_KEY, RELEASE, 80},
{LAYER_1, ACTIVATE, 80},           // Hold triggered by first complete cycle
{LAYER_1, DEACTIVATE, 100},
{3003, RELEASE, 150}
```

---

## Test 2.8: BALANCED - Timeout vs Complete Cycle Race

**Objective**: Verify behavior when hold timeout and complete cycle occur close together

**Configuration**: Same as Test 2.4

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);          // t=0ms
press_key(INTERRUPTING_KEY, 190);  // t=190ms (close to timeout)
release_key(INTERRUPTING_KEY, 15); // t=205ms (complete cycle after timeout)
release_key(TAP_DANCE_KEY, 45);    // t=250ms
```

**Expected Output**:
```cpp
{INTERRUPTING_KEY, PRESS, 190},
{LAYER_1, ACTIVATE, 200},          // Hold via timeout (wins the race)
{INTERRUPTING_KEY, RELEASE, 205},
{LAYER_1, DEACTIVATE, 250}
```

---

## Test 2.9: HOLD_PREFERRED - Immediate Hold on Any Press

**Objective**: Verify HOLD_PREFERRED triggers hold immediately on any interrupting key press

**Configuration**:
```cpp
Strategy: HOLD_PREFERRED
Other settings same as Test 2.1
```

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);          // t=0ms
press_key(INTERRUPTING_KEY, 50);   // t=50ms (immediate hold trigger)
release_key(INTERRUPTING_KEY, 50); // t=100ms
release_key(TAP_DANCE_KEY, 50);    // t=150ms
```

**Expected Output**:
```cpp
{INTERRUPTING_KEY, PRESS, 50},
{LAYER_1, ACTIVATE, 50},           // Immediate hold on interrupt press
{INTERRUPTING_KEY, RELEASE, 100},
{LAYER_1, DEACTIVATE, 150}
```

---

## Test 2.10: HOLD_PREFERRED - First Interruption Wins

**Objective**: Verify HOLD_PREFERRED triggers on first interruption only

**Configuration**: Same as Test 2.9

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);          // t=0ms
press_key(INTERRUPTING_KEY, 30);   // t=30ms (first interrupt - triggers hold)
press_key(3003, 20);               // t=50ms (second interrupt - ignored)
release_key(INTERRUPTING_KEY, 50); // t=100ms
release_key(3003, 50);             // t=150ms
release_key(TAP_DANCE_KEY, 50);    // t=200ms
```

**Expected Output**:
```cpp
{INTERRUPTING_KEY, PRESS, 30},
{LAYER_1, ACTIVATE, 30},           // Hold on first interrupt
{3003, PRESS, 50},                 // Second key processed normally
{INTERRUPTING_KEY, RELEASE, 100},
{3003, RELEASE, 150},
{LAYER_1, DEACTIVATE, 200}
```

---

## Test 2.11: HOLD_PREFERRED - Tap without Interruption

**Objective**: Verify HOLD_PREFERRED still allows tap when no interruption occurs

**Configuration**: Same as Test 2.9

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);          // t=0ms
release_key(TAP_DANCE_KEY, 100);   // t=100ms (no interruption)
platform_wait_ms(200);
```

**Expected Output**:
```cpp
{OUTPUT_KEY, PRESS, 300},          // Tap action (no interruption)
{OUTPUT_KEY, RELEASE, 300}
```

---

## Test 2.12: Strategy Comparison - Same Input Pattern

**Objective**: Verify different strategies produce different outputs with identical input

**Input Pattern for All Strategies**:
```cpp
press_key(TAP_DANCE_KEY);          // t=0ms
press_key(INTERRUPTING_KEY, 50);   // t=50ms
release_key(INTERRUPTING_KEY, 50); // t=100ms
release_key(TAP_DANCE_KEY, 50);    // t=150ms
```

**TAP_PREFERRED Expected Output**:
```cpp
{INTERRUPTING_KEY, PRESS, 50},
{INTERRUPTING_KEY, RELEASE, 100},
{OUTPUT_KEY, PRESS, 350},          // Tap (interruption ignored)
{OUTPUT_KEY, RELEASE, 350}
```

**BALANCED Expected Output**:
```cpp
{INTERRUPTING_KEY, PRESS, 50},
{INTERRUPTING_KEY, RELEASE, 100},
{LAYER_1, ACTIVATE, 100},          // Hold (complete cycle)
{LAYER_1, DEACTIVATE, 150}
```

**HOLD_PREFERRED Expected Output**:
```cpp
{INTERRUPTING_KEY, PRESS, 50},
{LAYER_1, ACTIVATE, 50},           // Hold (immediate)
{INTERRUPTING_KEY, RELEASE, 100},
{LAYER_1, DEACTIVATE, 150}
```

---

## Test 2.13: Interruption During WAITING_FOR_TAP State

**Objective**: Verify interruptions during tap timeout period don't affect completed sequence

**Configuration**: Same as Test 2.1 (TAP_PREFERRED)

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);          // t=0ms
release_key(TAP_DANCE_KEY, 100);   // t=100ms (enter WAITING_FOR_TAP)
press_key(INTERRUPTING_KEY, 50);   // t=150ms (interrupt during tap wait)
release_key(INTERRUPTING_KEY, 50); // t=200ms
platform_wait_ms(150);            // t=350ms (tap timeout expires)
```

**Expected Output**:
```cpp
{INTERRUPTING_KEY, PRESS, 150},    // Interrupt processed normally
{INTERRUPTING_KEY, RELEASE, 200},
{OUTPUT_KEY, PRESS, 300},          // Original sequence completes
{OUTPUT_KEY, RELEASE, 300}
```

---

## Test 2.14: Edge Case - Interruption at Exact Timeout Boundary

**Objective**: Verify interruption timing at exact hold timeout boundary

**Configuration**: Same as Test 2.4 (BALANCED)

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);          // t=0ms
press_key(INTERRUPTING_KEY, 200);  // t=200ms (exactly at timeout)
release_key(INTERRUPTING_KEY, 1);  // t=201ms (complete cycle just after)
release_key(TAP_DANCE_KEY, 49);    // t=250ms
```

**Expected Output**:
```cpp
{INTERRUPTING_KEY, PRESS, 200},
{LAYER_1, ACTIVATE, 200},          // Timeout wins (happens first)
{INTERRUPTING_KEY, RELEASE, 201},
{LAYER_1, DEACTIVATE, 250}
```

---

## Test 2.15: Strategy with No Hold Action Available

**Objective**: Verify strategy behavior when hold action not configured

**Configuration**:
```cpp
Strategy: HOLD_PREFERRED
Tap actions: [1: SENDKEY(OUTPUT_KEY)]
Hold actions: [] // No hold actions
```

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);          // t=0ms
press_key(INTERRUPTING_KEY, 50);   // t=50ms (would trigger hold if available)
release_key(INTERRUPTING_KEY, 50); // t=100ms
release_key(TAP_DANCE_KEY, 50);    // t=150ms
```

**Expected Output**:
```cpp
{OUTPUT_KEY, PRESS, 0},            // Immediate execution (no hold available)
{INTERRUPTING_KEY, PRESS, 50},
{INTERRUPTING_KEY, RELEASE, 100},
{OUTPUT_KEY, RELEASE, 150}
```