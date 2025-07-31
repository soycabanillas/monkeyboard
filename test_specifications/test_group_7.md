# Test Group 7: Complex Interruption Scenarios

This group tests complex interruption patterns, multiple interrupting keys, rapid interruption sequences, and strategy-specific interruption behavior.

## Test 7.1: Multiple Sequential Interruptions - TAP_PREFERRED

**Objective**: Verify multiple interrupting keys are all ignored with TAP_PREFERRED strategy

**Configuration**:
```cpp
TAP_DANCE_KEY = 3000
Strategy: TAP_PREFERRED
Tap actions: [1: SENDKEY(3001)]
Hold actions: [1: CHANGELAYER(1)]
INTERRUPTING_KEY_1 = 3010
INTERRUPTING_KEY_2 = 3011
Hold timeout: 200ms, Tap timeout: 200ms
```

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);            // t=0ms
press_key(INTERRUPTING_KEY_1, 30);   // t=30ms (first interrupt)
press_key(INTERRUPTING_KEY_2, 40);   // t=70ms (second interrupt)
release_key(INTERRUPTING_KEY_1, 30); // t=100ms
release_key(INTERRUPTING_KEY_2, 50); // t=150ms
release_key(TAP_DANCE_KEY, 30);      // t=180ms (before hold timeout)
platform_wait_ms(200);              // t=380ms
```

**Expected Output**:
```cpp
{INTERRUPTING_KEY_1, PRESS, 30},
{INTERRUPTING_KEY_2, PRESS, 70},
{INTERRUPTING_KEY_1, RELEASE, 100},
{INTERRUPTING_KEY_2, RELEASE, 150},
{3001, PRESS, 380},                  // Tap action (all interruptions ignored)
{3001, RELEASE, 380}
```

---

## Test 7.2: Multiple Sequential Interruptions - BALANCED

**Objective**: Verify BALANCED strategy triggers hold on first complete press/release cycle

**Configuration**: Same as Test 7.1, but Strategy: BALANCED

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);            // t=0ms
press_key(INTERRUPTING_KEY_1, 30);   // t=30ms
press_key(INTERRUPTING_KEY_2, 20);   // t=50ms
release_key(INTERRUPTING_KEY_1, 30); // t=80ms (first complete cycle)
release_key(INTERRUPTING_KEY_2, 40); // t=120ms (second complete cycle)
release_key(TAP_DANCE_KEY, 30);      // t=150ms
```

**Expected Output**:
```cpp
{INTERRUPTING_KEY_1, PRESS, 30},
{INTERRUPTING_KEY_2, PRESS, 50},
{INTERRUPTING_KEY_1, RELEASE, 80},
{LAYER_1, ACTIVATE, 80},             // Hold triggered by first complete cycle
{INTERRUPTING_KEY_2, RELEASE, 120},
{LAYER_1, DEACTIVATE, 150}
```

---

## Test 7.3: Multiple Sequential Interruptions - HOLD_PREFERRED

**Objective**: Verify HOLD_PREFERRED triggers hold on first key press

**Configuration**: Same as Test 7.1, but Strategy: HOLD_PREFERRED

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);            // t=0ms
press_key(INTERRUPTING_KEY_1, 30);   // t=30ms (first interrupt - triggers hold)
press_key(INTERRUPTING_KEY_2, 20);   // t=50ms (second interrupt - ignored)
release_key(INTERRUPTING_KEY_1, 30); // t=80ms
release_key(INTERRUPTING_KEY_2, 40); // t=120ms
release_key(TAP_DANCE_KEY, 30);      // t=150ms
```

**Expected Output**:
```cpp
{INTERRUPTING_KEY_1, PRESS, 30},
{LAYER_1, ACTIVATE, 30},             // Hold triggered immediately
{INTERRUPTING_KEY_2, PRESS, 50},     // Second key processed normally
{INTERRUPTING_KEY_1, RELEASE, 80},
{INTERRUPTING_KEY_2, RELEASE, 120},
{LAYER_1, DEACTIVATE, 150}
```

---

## Test 7.4: Rapid Interruption Sequence

**Objective**: Verify system handles very rapid interruption patterns

**Configuration**: Same as Test 7.1, Strategy: BALANCED

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);            // t=0ms
// Rapid fire interruptions
press_key(INTERRUPTING_KEY_1, 10);   // t=10ms
release_key(INTERRUPTING_KEY_1, 5);  // t=15ms (very fast complete cycle)
press_key(INTERRUPTING_KEY_2, 5);    // t=20ms
release_key(INTERRUPTING_KEY_2, 5);  // t=25ms (second fast cycle)
release_key(TAP_DANCE_KEY, 25);      // t=50ms
```

**Expected Output**:
```cpp
{INTERRUPTING_KEY_1, PRESS, 10},
{INTERRUPTING_KEY_1, RELEASE, 15},
{LAYER_1, ACTIVATE, 15},             // Hold triggered by first rapid cycle
{INTERRUPTING_KEY_2, PRESS, 20},
{INTERRUPTING_KEY_2, RELEASE, 25},
{LAYER_1, DEACTIVATE, 50}
```

---

## Test 7.5: Overlapping Interruption Windows

**Objective**: Verify behavior when interrupting keys have overlapping press/release windows

**Configuration**: Same as Test 7.1, Strategy: BALANCED

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);            // t=0ms
press_key(INTERRUPTING_KEY_1, 30);   // t=30ms
press_key(INTERRUPTING_KEY_2, 20);   // t=50ms (overlap begins)
release_key(INTERRUPTING_KEY_1, 40); // t=90ms (first key releases while second still held)
release_key(INTERRUPTING_KEY_2, 30); // t=120ms
release_key(TAP_DANCE_KEY, 30);      // t=150ms
```

**Expected Output**:
```cpp
{INTERRUPTING_KEY_1, PRESS, 30},
{INTERRUPTING_KEY_2, PRESS, 50},
{INTERRUPTING_KEY_1, RELEASE, 90},
{LAYER_1, ACTIVATE, 90},             // Hold triggered by first complete cycle
{INTERRUPTING_KEY_2, RELEASE, 120},
{LAYER_1, DEACTIVATE, 150}
```

---

## Test 7.6: Interruption During Different States

**Objective**: Verify interruption behavior during different state machine states

**Configuration**: Same as Test 7.1, Strategy: BALANCED

**Test 7.6a - Interruption during WAITING_FOR_HOLD**:
```cpp
press_key(TAP_DANCE_KEY);            // t=0ms (enter WAITING_FOR_HOLD)
press_key(INTERRUPTING_KEY_1, 50);   // t=50ms (interrupt during WAITING_FOR_HOLD)
release_key(INTERRUPTING_KEY_1, 50); // t=100ms (complete cycle)
release_key(TAP_DANCE_KEY, 50);      // t=150ms
Expected: Hold action triggered
```

**Test 7.6b - Interruption during WAITING_FOR_TAP**:
```cpp
press_key(TAP_DANCE_KEY);            // t=0ms
release_key(TAP_DANCE_KEY, 100);     // t=100ms (enter WAITING_FOR_TAP)
press_key(INTERRUPTING_KEY_1, 50);   // t=150ms (interrupt during WAITING_FOR_TAP)
release_key(INTERRUPTING_KEY_1, 50); // t=200ms
platform_wait_ms(200);              // t=400ms
Expected: Original sequence completes normally (interruption in WAITING_FOR_TAP ignored)
```

---

## Test 7.7: Interruption Race with Timeout

**Objective**: Verify interruption vs timeout race conditions

**Configuration**: Same as Test 7.1, Strategy: BALANCED

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);            // t=0ms
press_key(INTERRUPTING_KEY_1, 199);  // t=199ms (1ms before hold timeout)
release_key(INTERRUPTING_KEY_1, 2);  // t=201ms (complete cycle after timeout)
release_key(TAP_DANCE_KEY, 49);      // t=250ms
```

**Expected Output**:
```cpp
{INTERRUPTING_KEY_1, PRESS, 199},
{LAYER_1, ACTIVATE, 200},            // Hold timeout wins (earlier timestamp)
{INTERRUPTING_KEY_1, RELEASE, 201},
{LAYER_1, DEACTIVATE, 250}
```

---

## Test 7.8: Chain of Interruptions with Different Strategies

**Objective**: Verify how different strategies handle chains of interruptions

**Configuration**: Same as Test 7.1

**Test 7.8a - TAP_PREFERRED Chain**:
```cpp
Strategy: TAP_PREFERRED
Input: 5 sequential interrupting keys, all with complete cycles
Expected: All ignored, tap action executed
```

**Test 7.8b - BALANCED Chain**:
```cpp
Strategy: BALANCED  
Input: 5 sequential interrupting keys, all with complete cycles
Expected: Hold triggered by first complete cycle
```

**Test 7.8c - HOLD_PREFERRED Chain**:
```cpp
Strategy: HOLD_PREFERRED
Input: 5 sequential interrupting keys
Expected: Hold triggered by first key press
```

---

## Test 7.9: Interruption with Multi-Tap Sequence

**Objective**: Verify interruption behavior during multi-tap sequences

**Configuration**:
```cpp
Strategy: BALANCED
Tap actions: [1: SENDKEY(3001), 2: SENDKEY(3002)]
Hold actions: [1: CHANGELAYER(1), 2: CHANGELAYER(2)]
Other settings same as Test 7.1
```

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);            // t=0ms (1st tap)
release_key(TAP_DANCE_KEY, 50);      // t=50ms
press_key(TAP_DANCE_KEY, 50);        // t=100ms (2nd tap begins)
press_key(INTERRUPTING_KEY_1, 30);   // t=130ms (interrupt during 2nd tap)
release_key(INTERRUPTING_KEY_1, 40); // t=170ms (complete cycle)
release_key(TAP_DANCE_KEY, 30);      // t=200ms
```

**Expected Output**:
```cpp
{INTERRUPTING_KEY_1, PRESS, 130},
{INTERRUPTING_KEY_1, RELEASE, 170},
{LAYER_2, ACTIVATE, 170},            // Hold action for 2nd tap count
{LAYER_2, DEACTIVATE, 200}
```

---

## Test 7.10: Interruption Timing Precision

**Objective**: Verify precise timing of interruption processing

**Configuration**: Same as Test 7.1, Strategy: HOLD_PREFERRED

**Input Sequence**:
```cpp
platform_wait_ms(1000);             // t=1000ms (establish baseline)
press_key(TAP_DANCE_KEY);            // t=1000ms
press_key(INTERRUPTING_KEY_1, 50);   // t=1050ms (precise interrupt timing)
release_key(INTERRUPTING_KEY_1, 50); // t=1100ms
release_key(TAP_DANCE_KEY, 50);      // t=1150ms
```

**Expected Output**:
```cpp
{INTERRUPTING_KEY_1, PRESS, 1050},
{LAYER_1, ACTIVATE, 1050},           // Hold triggered at exact interrupt time
{INTERRUPTING_KEY_1, RELEASE, 1100},
{LAYER_1, DEACTIVATE, 1150}
```

---

## Test 7.11: Complex Interruption Pattern - Nested Timing

**Objective**: Verify handling of complex nested interruption patterns

**Configuration**: Same as Test 7.1, Strategy: BALANCED

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);            // t=0ms
press_key(INTERRUPTING_KEY_1, 20);   // t=20ms
  press_key(INTERRUPTING_KEY_2, 10); // t=30ms (nested interrupt)
  release_key(INTERRUPTING_KEY_2, 20); // t=50ms (nested complete)
release_key(INTERRUPTING_KEY_1, 30); // t=80ms (first complete)
release_key(TAP_DANCE_KEY, 20);      // t=100ms
```

**Expected Output**:
```cpp
{INTERRUPTING_KEY_1, PRESS, 20},
{INTERRUPTING_KEY_2, PRESS, 30},
{INTERRUPTING_KEY_2, RELEASE, 50},
{LAYER_1, ACTIVATE, 50},             // Hold triggered by first complete cycle (nested)
{INTERRUPTING_KEY_1, RELEASE, 80},
{LAYER_1, DEACTIVATE, 100}
```

---

## Test 7.12: Interruption State Recovery

**Objective**: Verify system properly recovers state after complex interruption sequences

**Configuration**: Same as Test 7.1, Strategy: TAP_PREFERRED

**Input Sequence**:
```cpp
// First sequence with interruptions
press_key(TAP_DANCE_KEY);            // t=0ms
press_key(INTERRUPTING_KEY_1, 30);   // t=30ms
release_key(INTERRUPTING_KEY_1, 40); // t=70ms
release_key(TAP_DANCE_KEY, 30);      // t=100ms
platform_wait_ms(200);              // t=300ms (first sequence completes)

// Second sequence should start clean
press_key(TAP_DANCE_KEY, 50);        // t=350ms
platform_wait_ms(250);              // t=600ms (hold timeout)
release_key(TAP_DANCE_KEY);          // t=600ms
```

**Expected Output**:
```cpp
{INTERRUPTING_KEY_1, PRESS, 30},
{INTERRUPTING_KEY_1, RELEASE, 70},
{3001, PRESS, 300},                  // First sequence - tap (interruption ignored)
{3001, RELEASE, 300},
{LAYER_1, ACTIVATE, 550},            // Second sequence - hold (clean state)
{LAYER_1, DEACTIVATE, 600}
```

---

## Test 7.13: Maximum Interruption Load

**Objective**: Verify system handles high number of interrupting keys

**Configuration**: Same as Test 7.1, Strategy: BALANCED

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);            // t=0ms
// 10 rapid interrupting keys
for (int i = 0; i < 10; i++) {
    press_key(3010 + i, i * 5);      // t=i*5ms (staggered presses)
    release_key(3010 + i, 20);       // t=i*5+20ms (staggered releases)
}
release_key(TAP_DANCE_KEY, 100);     // t=150ms
```

**Expected Output**:
```cpp
// All interrupting keys processed
{3010, PRESS, 0}, {3010, RELEASE, 20},   // First key completes cycle
{LAYER_1, ACTIVATE, 20},                 // Hold triggered by first complete
{3011, PRESS, 5}, {3011, RELEASE, 25},   // Remaining keys processed normally
// ... (all other keys)
{LAYER_1, DEACTIVATE, 150}
```

---

## Test 7.14: Interruption with Overflow Scenarios

**Objective**: Verify interruption behavior during action overflow

**Configuration**:
```cpp
Strategy: BALANCED
Tap actions: [1: SENDKEY(3001), 2: SENDKEY(3002)]  // Only 2 actions
Hold actions: [1: CHANGELAYER(1)]                  // Only 1st tap has hold
Other settings same as Test 7.1
```

**Input Sequence**:
```cpp
tap_key(TAP_DANCE_KEY, 20);          // t=0-20ms (1st tap)
tap_key(TAP_DANCE_KEY, 30, 20);      // t=50-80ms (2nd tap)
press_key(TAP_DANCE_KEY, 30);        // t=110ms (3rd tap - overflow)
press_key(INTERRUPTING_KEY_1, 30);   // t=140ms (interrupt during overflow)
release_key(INTERRUPTING_KEY_1, 30); // t=170ms (complete cycle)
release_key(TAP_DANCE_KEY, 30);      // t=200ms
platform_wait_ms(200);              // t=400ms
```

**Expected Output**:
```cpp
{INTERRUPTING_KEY_1, PRESS, 140},
{INTERRUPTING_KEY_1, RELEASE, 170},
{3002, PRESS, 400},                  // Tap action (no hold available for 3rd tap)
{3002, RELEASE, 400}
```

---

## Test 7.15: Interruption Edge Case - Simultaneous Events

**Objective**: Verify behavior when trigger and interrupt events occur simultaneously

**Configuration**: Same as Test 7.1, Strategy: HOLD_PREFERRED

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);            // t=0ms
press_key(INTERRUPTING_KEY_1, 0);    // t=0ms (simultaneous with trigger)
release_key(INTERRUPTING_KEY_1, 50); // t=50ms
release_key(TAP_DANCE_KEY, 50);      // t=100ms
```

**Expected Output**:
```cpp
{INTERRUPTING_KEY_1, PRESS, 0},      // Both processed at same time
{LAYER_1, ACTIVATE, 0},              // Hold triggered immediately
{INTERRUPTING_KEY_1, RELEASE, 50},
{LAYER_1, DEACTIVATE, 100}
```