# Test Group 3: Multi-Tap Sequences

This group tests tap count progression, sequence continuation vs. reset timing, and multi-tap behavior with various action configurations.

## Test 3.1: Basic Two-Tap Sequence

**Objective**: Verify basic two-tap sequence with proper tap count progression

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
press_key(TAP_DANCE_KEY);        // t=0ms (1st tap begins)
release_key(TAP_DANCE_KEY, 100); // t=100ms (1st tap completes)
press_key(TAP_DANCE_KEY, 150);   // t=250ms (2nd tap begins, within timeout)
release_key(TAP_DANCE_KEY, 100); // t=350ms (2nd tap completes)
platform_wait_ms(200);          // t=550ms (tap timeout expires)
```

**Expected Output**:
```cpp
{3002, PRESS, 550},    // Second tap action executed
{3002, RELEASE, 550}
```

---

## Test 3.2: Three-Tap Sequence

**Objective**: Verify three-tap sequence progression

**Configuration**:
```cpp
Tap actions: [1: SENDKEY(3001), 2: SENDKEY(3002), 3: SENDKEY(3003)]
Hold actions: [1: CHANGELAYER(1)]
Other settings same as Test 3.1
```

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
release_key(TAP_DANCE_KEY, 50);  // t=50ms
press_key(TAP_DANCE_KEY, 100);   // t=150ms (within tap timeout)
release_key(TAP_DANCE_KEY, 50);  // t=200ms
press_key(TAP_DANCE_KEY, 100);   // t=300ms (within tap timeout)
release_key(TAP_DANCE_KEY, 50);  // t=350ms
platform_wait_ms(200);          // t=550ms
```

**Expected Output**:
```cpp
{3003, PRESS, 550},    // Third tap action executed
{3003, RELEASE, 550}
```

---

## Test 3.3: Sequence Reset - Tap Timeout Expiry

**Objective**: Verify sequence resets when tap timeout expires between taps

**Configuration**: Same as Test 3.1

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
release_key(TAP_DANCE_KEY, 100); // t=100ms
platform_wait_ms(200);          // t=300ms (tap timeout expires - sequence resets)
press_key(TAP_DANCE_KEY, 50);    // t=350ms (new sequence begins)
release_key(TAP_DANCE_KEY, 100); // t=450ms
platform_wait_ms(200);          // t=650ms
```

**Expected Output**:
```cpp
{3001, PRESS, 300},    // First sequence completes (1st tap)
{3001, RELEASE, 300},
{3001, PRESS, 650},    // Second sequence (also 1st tap)
{3001, RELEASE, 650}
```

---

## Test 3.4: Multi-Tap with Hold Action (First Tap)

**Objective**: Verify hold action works correctly during multi-tap sequence (1st tap count)

**Configuration**: Same as Test 3.1

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms (1st tap)
platform_wait_ms(250);          // t=250ms (hold timeout exceeded)
release_key(TAP_DANCE_KEY);      // t=250ms
```

**Expected Output**:
```cpp
{LAYER_1, ACTIVATE, 200},        // Hold action for 1st tap count
{LAYER_1, DEACTIVATE, 250}
```

---

## Test 3.5: Multi-Tap with Hold Action (Second Tap)

**Objective**: Verify hold action at second tap count when configured

**Configuration**:
```cpp
Tap actions: [1: SENDKEY(3001), 2: SENDKEY(3002)]
Hold actions: [1: CHANGELAYER(1), 2: CHANGELAYER(2)]
Other settings same as Test 3.1
```

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
release_key(TAP_DANCE_KEY, 100); // t=100ms (1st tap complete)
press_key(TAP_DANCE_KEY, 50);    // t=150ms (2nd tap begins)
platform_wait_ms(250);          // t=400ms (hold timeout exceeded)
release_key(TAP_DANCE_KEY);      // t=400ms
```

**Expected Output**:
```cpp
{LAYER_2, ACTIVATE, 350},        // Hold action for 2nd tap count (150ms + 200ms timeout)
{LAYER_2, DEACTIVATE, 400}
```

---

## Test 3.6: Hold Action Not Available for Tap Count

**Objective**: Verify behavior when hold action not configured for current tap count

**Configuration**:
```cpp
Tap actions: [1: SENDKEY(3001), 2: SENDKEY(3002), 3: SENDKEY(3003)]
Hold actions: [1: CHANGELAYER(1)] // Only 1st tap has hold action
Other settings same as Test 3.1
```

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
release_key(TAP_DANCE_KEY, 50);  // t=50ms
press_key(TAP_DANCE_KEY, 50);    // t=100ms (2nd tap - no hold action)
platform_wait_ms(250);          // t=350ms (hold timeout exceeded)
release_key(TAP_DANCE_KEY);      // t=350ms
platform_wait_ms(200);          // t=550ms
```

**Expected Output**:
```cpp
{3002, PRESS, 550},    // Tap action (no hold available for 2nd tap)
{3002, RELEASE, 550}
```

---

## Test 3.7: Rapid Tap Sequence - All Within Timeout

**Objective**: Verify system handles rapid consecutive taps correctly

**Configuration**: Same as Test 3.2 (3 tap actions)

**Input Sequence**:
```cpp
tap_key(TAP_DANCE_KEY, 20);      // t=0-20ms (very fast tap)
tap_key(TAP_DANCE_KEY, 50, 20);  // t=70-90ms (fast tap with delay)
tap_key(TAP_DANCE_KEY, 30, 20);  // t=120-140ms (fast tap)
platform_wait_ms(200);          // t=340ms
```

**Expected Output**:
```cpp
{3003, PRESS, 340},    // Third tap action (rapid sequence completed)
{3003, RELEASE, 340}
```

---

## Test 3.8: Mixed Tap and Hold in Sequence

**Objective**: Verify mix of tap and hold behaviors within single sequence

**Configuration**:
```cpp
Tap actions: [1: SENDKEY(3001), 2: SENDKEY(3002), 3: SENDKEY(3003)]
Hold actions: [1: CHANGELAYER(1), 2: CHANGELAYER(2)]
Other settings same as Test 3.1
```

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms (1st tap)
release_key(TAP_DANCE_KEY, 50);  // t=50ms (1st tap completes)
press_key(TAP_DANCE_KEY, 50);    // t=100ms (2nd tap begins)
platform_wait_ms(250);          // t=350ms (hold 2nd tap)
release_key(TAP_DANCE_KEY);      // t=350ms
platform_wait_ms(50);           // t=400ms
press_key(TAP_DANCE_KEY);        // t=400ms (new sequence - 1st tap)
release_key(TAP_DANCE_KEY, 50);  // t=450ms
platform_wait_ms(200);          // t=650ms
```

**Expected Output**:
```cpp
{LAYER_2, ACTIVATE, 300},       // Hold action for 2nd tap
{LAYER_2, DEACTIVATE, 350},
{3001, PRESS, 650},             // New sequence - 1st tap
{3001, RELEASE, 650}
```

---

## Test 3.9: Tap Count Boundary - Exact Timeout Edge

**Objective**: Verify timing precision at tap timeout boundaries

**Configuration**: Same as Test 3.1

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
release_key(TAP_DANCE_KEY, 100); // t=100ms
press_key(TAP_DANCE_KEY, 200);   // t=300ms (exactly at timeout boundary)
release_key(TAP_DANCE_KEY, 50);  // t=350ms
platform_wait_ms(200);          // t=550ms
```

**Expected Output**:
```cpp
{3001, PRESS, 300},    // First sequence completes (timeout exactly reached)
{3001, RELEASE, 300},
{3001, PRESS, 550},    // Second sequence starts fresh
{3001, RELEASE, 550}
```

---

## Test 3.10: Maximum Practical Tap Count

**Objective**: Verify system handles high tap counts correctly

**Configuration**:
```cpp
Tap actions: [1: SENDKEY(3001), 2: SENDKEY(3002), 3: SENDKEY(3003), 
              4: SENDKEY(3004), 5: SENDKEY(3005)]
Hold actions: [1: CHANGELAYER(1)]
Other settings same as Test 3.1
```

**Input Sequence**:
```cpp
// Perform 5 rapid taps
for (int i = 0; i < 5; i++) {
    press_key(TAP_DANCE_KEY);        // t=i*80ms
    release_key(TAP_DANCE_KEY, 30);  // t=i*80+30ms
    platform_wait_ms(50);           // t=i*80+80ms
}
platform_wait_ms(200);              // Final timeout
```

**Expected Output**:
```cpp
{3005, PRESS, 600},    // Fifth tap action executed
{3005, RELEASE, 600}
```

---

## Test 3.11: Sequence Continuation vs New Sequence

**Objective**: Verify clear distinction between sequence continuation and new sequence

**Configuration**: Same as Test 3.1

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);        // t=0ms
release_key(TAP_DANCE_KEY, 100); // t=100ms
press_key(TAP_DANCE_KEY, 199);   // t=299ms (1ms before timeout)
release_key(TAP_DANCE_KEY, 50);  // t=349ms
platform_wait_ms(200);          // t=549ms

// Clear gap, then new sequence
platform_wait_ms(100);          // t=649ms
press_key(TAP_DANCE_KEY);        // t=649ms
release_key(TAP_DANCE_KEY, 50);  // t=699ms
platform_wait_ms(200);          // t=899ms
```

**Expected Output**:
```cpp
{3002, PRESS, 549},    // Second tap (sequence continued)
{3002, RELEASE, 549},
{3001, PRESS, 899},    // New sequence - first tap
{3001, RELEASE, 899}
```

---

## Test 3.12: Multi-Tap with Strategy Interruption

**Objective**: Verify multi-tap behavior combined with hold strategy interruption

**Configuration**:
```cpp
Strategy: BALANCED
Tap actions: [1: SENDKEY(3001), 2: SENDKEY(3002)]
Hold actions: [1: CHANGELAYER(1), 2: CHANGELAYER(2)]
INTERRUPTING_KEY = 3010
Other settings same as Test 3.1
```

**Input Sequence**:
```cpp
press_key(TAP_DANCE_KEY);          // t=0ms (1st tap)
release_key(TAP_DANCE_KEY, 50);    // t=50ms
press_key(TAP_DANCE_KEY, 50);      // t=100ms (2nd tap begins)
press_key(INTERRUPTING_KEY, 50);   // t=150ms (interrupt)
release_key(INTERRUPTING_KEY, 50); // t=200ms (complete cycle)
release_key(TAP_DANCE_KEY, 50);    // t=250ms
```

**Expected Output**:
```cpp
{INTERRUPTING_KEY, PRESS, 150},
{INTERRUPTING_KEY, RELEASE, 200},
{LAYER_2, ACTIVATE, 200},         // Hold action for 2nd tap count
{LAYER_2, DEACTIVATE, 250}
```

---

## Test 3.13: Tap Count Reset Verification

**Objective**: Verify tap count properly resets between independent sequences

**Configuration**: Same as Test 3.2 (3 tap actions)

**Input Sequence**:
```cpp
// First incomplete sequence (2 taps)
press_key(TAP_DANCE_KEY);        // t=0ms
release_key(TAP_DANCE_KEY, 50);  // t=50ms
press_key(TAP_DANCE_KEY, 50);    // t=100ms
release_key(TAP_DANCE_KEY, 50);  // t=150ms
platform_wait_ms(200);          // t=350ms (completes as 2nd tap)

// Second sequence should start fresh
press_key(TAP_DANCE_KEY, 50);    // t=400ms
release_key(TAP_DANCE_KEY, 50);  // t=450ms
platform_wait_ms(200);          // t=650ms
```

**Expected Output**:
```cpp
{3002, PRESS, 350},    // First sequence - 2nd tap
{3002, RELEASE, 350},
{3001, PRESS, 650},    // Second sequence - 1st tap (reset)
{3001, RELEASE, 650}
```

---

## Test 3.14: Very Fast Multi-Tap Sequence

**Objective**: Verify system handles extremely rapid tap sequences

**Configuration**: Same as Test 3.2

**Input Sequence**:
```cpp
tap_key(TAP_DANCE_KEY, 5);       // t=0-5ms
tap_key(TAP_DANCE_KEY, 10, 5);   // t=15-20ms
tap_key(TAP_DANCE_KEY, 10, 5);   // t=30-35ms
platform_wait_ms(200);          // t=235ms
```

**Expected Output**:
```cpp
{3003, PRESS, 235},    // Third tap action (very fast sequence)
{3003, RELEASE, 235}
```

---

## Test 3.15: Multi-Tap Overflow Preview

**Objective**: Verify behavior approaching overflow conditions (sets up for Group 4)

**Configuration**:
```cpp
Tap actions: [1: SENDKEY(3001), 2: SENDKEY(3002)]  // Only 2 actions configured
Hold actions: [1: CHANGELAYER(1)]
Other settings same as Test 3.1
```

**Input Sequence**:
```cpp
// Attempt 3 taps (more than configured)
tap_key(TAP_DANCE_KEY, 30);      // t=0-30ms
tap_key(TAP_DANCE_KEY, 50, 30);  // t=80-110ms
tap_key(TAP_DANCE_KEY, 50, 30);  // t=160-190ms (3rd tap - overflow)
platform_wait_ms(200);          // t=390ms
```

**Expected Output**:
```cpp
{3002, PRESS, 390},    // Uses last configured action (2nd tap action)
{3002, RELEASE, 390}
```