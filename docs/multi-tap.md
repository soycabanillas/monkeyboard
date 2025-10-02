# Multi-Tap: Advanced Key Sequences

Multi-Tap lets you pack multiple actions into a single key based on how many times you tap it and whether you hold it down. Think of it as your key's secret menu - tap once for semicolon, hold for a navigation layer, or tap twice for something completely different.

## Why Multi-Tap?

Instead of cluttering your keyboard with dedicated keys for every function, Multi-Tap lets you:
- **Save space**: One key, multiple functions
- **Stay in flow**: Access layers and shortcuts without hand movement
- **Customize deeply**: Different actions for 1-tap, 2-tap, 3-tap, and holds
- **Game efficiently**: Quick macros without breaking your grip

## Quick Example

Here's a semicolon key that also activates a movement layer when held:

```c
// Actions: tap once = semicolon, hold = movement layer
pipeline_tap_dance_action_config_t* actions[] = {
    createbehaviouraction_tap(1, KC_SCLN),
    createbehaviouraction_hold(1, _MOVEMENT_LAYER, TAP_DANCE_BALANCED)
};

// Create the behavior
pipeline_tap_dance_behaviour_t* semicolon_behavior = 
    createbehaviour(MY_SEMICOLON_KEY, actions, 2);

// Fine-tune timing
semicolon_behavior->config->hold_timeout = 200;  // 200ms to trigger hold
semicolon_behavior->config->tap_timeout = 200;   // 200ms between taps
```

## How It Works

Multi-Tap tracks your key presses in real-time and decides what action to take based on:

1. **Tap count**: How many times you've tapped (1, 2, 3...)
2. **Hold detection**: Whether you're holding the key down
3. **Timing**: How fast you're tapping and how long you hold

### The State Machine

Your key moves through these states:
- **IDLE**: Waiting for first press
- **WAITING_FOR_HOLD**: Just pressed, checking if you'll hold
- **WAITING_FOR_RELEASE**: Pressed but no hold action, waiting for release
- **WAITING_FOR_TAP**: Released, waiting to see if you'll tap again
- **HOLDING**: Hold action is active

## Configuration Deep Dive

### Basic Structure

```c
// Define what happens on each tap/hold
pipeline_tap_dance_action_config_t* actions[] = {
    createbehaviouraction_tap(1, KC_A),           // 1 tap = 'A'
    createbehaviouraction_tap(3, KC_B),           // 3 taps = 'B' (skip tap 2)
    createbehaviouraction_hold(1, LAYER_NUM, TAP_DANCE_BALANCED), // Hold after 1 tap = number layer
    createbehaviouraction_hold(2, LAYER_SYM, TAP_DANCE_BALANCED)  // Hold after 2 taps = symbol layer
};

pipeline_tap_dance_behaviour_t* behavior = createbehaviour(MY_KEY, actions, 4);
```

### Action Types

**Tap Actions** (`createbehaviouraction_tap`):
```c
createbehaviouraction_tap(tap_count, keycode)
```
- `tap_count`: Which tap number triggers this (1, 2, 3...)
- `keycode`: What key to send (can be modified keys like `MONKEEB_LCTL(KC_C)`)

**Hold Actions** (`createbehaviouraction_hold`):
```c
createbehaviouraction_hold(tap_count, layer, hold_strategy)
```
- `tap_count`: Hold after this many taps
- `layer`: Which layer to activate temporarily
- `hold_strategy`: How aggressive the hold detection is

### Hold Strategies

Choose how your Multi-Tap behaves when other keys are pressed while waiting for a hold decision:

#### `TAP_DANCE_HOLD_PREFERRED` ⚡

**When Hold Activates:**
- Immediately when another key is pressed while in `WAITING_FOR_HOLD` state
- After hold timeout expires (if no other keys pressed)

**Behavior with Interrupting Keys:**
```c
// Example: Space key with hold-preferred strategy
pipeline_tap_dance_action_config_t* actions[] = {
    createbehaviouraction_tap(1, KC_SPC),
    createbehaviouraction_hold(1, SYMBOL_LAYER, TAP_DANCE_HOLD_PREFERRED)
};

// Sequence: Press Space, then press 'A' within hold timeout
// 1. Press Space → enters WAITING_FOR_HOLD state
// 2. Press 'A' → IMMEDIATELY activates SYMBOL_LAYER
// 3. 'A' gets processed using the symbol layer (might output '!' instead of 'a')
// 4. Release Space → deactivates layer
```

**Best for:** Gaming, frequent layer access, speed-focused workflows
**Trade-off:** Can accidentally trigger holds when typing fast

#### `TAP_DANCE_TAP_PREFERRED` 🎯

**When Hold Activates:**
- Only after the full hold timeout expires, regardless of other key presses
- Never activates due to interrupting keys alone

**Behavior with Interrupting Keys:**
```c
// Example: Space key with tap-preferred strategy  
pipeline_tap_dance_action_config_t* actions[] = {
    createbehaviouraction_tap(1, KC_SPC),
    createbehaviouraction_hold(1, SYMBOL_LAYER, TAP_DANCE_TAP_PREFERRED)
};

// Sequence: Press Space, then press 'A' within hold timeout
// 1. Press Space → enters WAITING_FOR_HOLD state  
// 2. Press 'A' → continues waiting (no immediate layer activation)
// 3. 'A' gets processed on current layer (outputs 'a')
// 4. After 200ms timeout → activates SYMBOL_LAYER (if Space still held)
// 5. Release Space → deactivates layer
```

**Best for:** Typing-heavy workflows, when you need reliable taps, avoiding accidental holds
**Trade-off:** Slower layer activation, less responsive for gaming

#### `TAP_DANCE_BALANCED` ⚖️

**When Hold Activates:**
- When another key is pressed AND released while in `WAITING_FOR_HOLD` state
- After hold timeout expires (if key still held)
- Uses smart context detection to avoid false positives

**Behavior with Interrupting Keys:**
```c
// Example: Space key with balanced strategy
pipeline_tap_dance_action_config_t* actions[] = {
    createbehaviouraction_tap(1, KC_SPC),
    createbehaviouraction_hold(1, SYMBOL_LAYER, TAP_DANCE_BALANCED)
};

// Sequence 1: Press Space, press and release 'A', then release Space
// 1. Press Space → enters WAITING_FOR_HOLD state
// 2. Press 'A' → continues waiting (no immediate activation)
// 3. Release 'A' → detects "press+release" pattern, activates SYMBOL_LAYER
// 4. Further keys get processed on symbol layer
// 5. Release Space → deactivates layer

// Sequence 2: Press Space, press 'A' but don't release, then release Space  
// 1. Press Space → enters WAITING_FOR_HOLD state
// 2. Press 'A' → continues waiting
// 3. Release Space before releasing 'A' → executes tap action instead
```

**Best for:** General purpose use, most users, balanced responsiveness
**Trade-off:** Slightly more complex behavior, requires understanding the "press+release" pattern

#### Timing Comparison

**Fastest Hold Activation:** `HOLD_PREFERRED` (immediate on key press)  
**Most Reliable Taps:** `TAP_PREFERRED` (always waits full timeout)  
**Best Balance:** `BALANCED` (smart detection of intentional holds)

#### Real-World Scenarios

**Gaming Example:**
```c
// WASD movement with sprint layer - use HOLD_PREFERRED for instant response
pipeline_tap_dance_action_config_t* w_actions[] = {
    createbehaviouraction_tap(1, KC_W),
    createbehaviouraction_hold(1, SPRINT_LAYER, TAP_DANCE_HOLD_PREFERRED)  // Instant sprint on movement
};
```

**Typing Example:**  
```c
// Space for space/enter, symbols layer - use TAP_PREFERRED to avoid accidental layers
pipeline_tap_dance_action_config_t* space_actions[] = {
    createbehaviouraction_tap(1, KC_SPC),
    createbehaviouraction_hold(1, SYMBOL_LAYER, TAP_DANCE_TAP_PREFERRED)  // Deliberate layer access
};
```

**General Use Example:**
```c
// Semicolon for punctuation and coding symbols - use BALANCED for smart behavior
pipeline_tap_dance_action_config_t* semi_actions[] = {
    createbehaviouraction_tap(1, KC_SCLN),
    createbehaviouraction_hold(1, SYMBOL_LAYER, TAP_DANCE_BALANCED)  // Smart detection
};
```

## Timing Configuration

### Hold Timeout
```c
behavior->config->hold_timeout = 200;  // milliseconds
```
How long to wait before "this is definitely a hold, not a tap."

- **Too fast (50-100ms)**: Accidental holds while typing
- **Just right (150-250ms)**: Responsive but deliberate
- **Too slow (500ms+)**: Feels sluggish, breaks flow

### Tap Timeout
```c
behavior->config->tap_timeout = 200;  // milliseconds
```
How long to wait between taps before "the sequence is over."

- **Too fast (100ms)**: Hard to execute multi-taps
- **Just right (200-300ms)**: Natural rhythm
- **Too slow (500ms+)**: Feels unresponsive

## Real-World Examples

### Gaming: WASD with Extras
```c
// W key: tap for forward, hold for sprint layer
pipeline_tap_dance_action_config_t* w_actions[] = {
    createbehaviouraction_tap(1, KC_W),
    createbehaviouraction_hold(1, SPRINT_LAYER, TAP_DANCE_HOLD_PREFERRED)
};
```

### Programming: Semicolon Power Key
```c
// Semicolon: tap for ;, double-tap for ::, hold for symbols
pipeline_tap_dance_action_config_t* semi_actions[] = {
    createbehaviouraction_tap(1, KC_SCLN),
    createbehaviouraction_tap(2, MONKEEB_LSFT(KC_SCLN)),  // Shift+; = :, twice = ::
    createbehaviouraction_hold(1, SYMBOL_LAYER, TAP_DANCE_BALANCED)
};
```

### Navigation: Space Cadet
```c
// Space: tap for space, double-tap for enter, hold for numbers
pipeline_tap_dance_action_config_t* space_actions[] = {
    createbehaviouraction_tap(1, KC_SPC),
    createbehaviouraction_tap(2, KC_ENT),
    createbehaviouraction_hold(1, NUMBER_LAYER, TAP_DANCE_BALANCED)
};
```

## Advanced Patterns

### Gaps in Sequences
You can skip tap counts - this is totally valid:
```c
pipeline_tap_dance_action_config_t* gap_actions[] = {
    createbehaviouraction_tap(1, KC_A),    // 1 tap = A
    createbehaviouraction_tap(3, KC_C),    // 3 taps = C (no 2-tap action)
    createbehaviouraction_tap(5, KC_E),    // 5 taps = E (no 4-tap action)
};
```

### Sequence Completion Behavior

Multi-Tap has smart logic for when sequences end:

**Immediate Execution** - When you reach a tap action and there are no further actions (tap or hold) configured:
```c
pipeline_tap_dance_action_config_t* actions[] = {
    createbehaviouraction_tap(1, KC_A),    // 1 tap = A
    createbehaviouraction_tap(3, KC_C),    // 3 taps = C (final action)
    createbehaviouraction_hold(2, LAYER_1, TAP_DANCE_BALANCED)  // Hold after 2 taps
};

// Behavior:
// 1st tap → waits (more actions possible)
// 3rd tap → executes immediately (no actions beyond tap 3)
```

**Timeout Execution** - When you reach a tap action but hold actions exist at the same tap count OR beyond:
```c
pipeline_tap_dance_action_config_t* actions[] = {
    createbehaviouraction_tap(1, KC_A),    // 1 tap = A
    createbehaviouraction_tap(2, KC_B),    // 2 taps = B
    createbehaviouraction_hold(3, LAYER_1, TAP_DANCE_BALANCED)  // Hold after 3 taps
};

// Behavior:
// 1st tap → waits (more actions possible)
// 2nd tap → waits for tap timeout (hold action exists beyond tap 2)
// If 3rd tap before timeout → continues to hold action
// If timeout reached → executes 2nd tap action
```

**Sequence Reset** - When you exceed all configured actions:
```c
pipeline_tap_dance_action_config_t* actions[] = {
    createbehaviouraction_tap(1, KC_A),    // 1 tap = A
    createbehaviouraction_tap(2, KC_B),    // 2 taps = B  
    createbehaviouraction_hold(3, LAYER_1, TAP_DANCE_BALANCED)  // Hold after 3 taps
};

// Max configured count is 3
// 1st-3rd taps → normal sequence behavior
// 4th tap → restarts sequence (begins new tap count from 1)
```

**Key insight:** Tap actions execute immediately only when no further actions are possible. If hold actions exist beyond the current tap count, the system waits to see if you'll continue the sequence, preventing premature execution.

## Layer Behavior

When a hold activates a layer:
- ✅ **Layer is momentary**: Active only while holding
- ✅ **Affects new key presses**: Keys pressed after activation use the new layer
- ✅ **Auto-deactivates**: Releasing the key returns to previous layer
- ✅ **Handles nesting**: Multiple layers can stack and unstack properly

### Layer Nesting and Stacking

Multi-Tap supports sophisticated layer stacking that lets you build complex layer hierarchies:

#### How Layer Stacking Works

When you activate hold actions on multiple keys, layers stack on top of each other:

```c
// Example: Two keys with hold actions
// Key A: Hold for SYMBOLS layer
// Key B: Hold for NUMBERS layer

// Sequence:
// 1. Hold A → SYMBOLS layer active
// 2. Hold B → NUMBERS layer stacks on top (SYMBOLS + NUMBERS active)
// 3. Release A → Only NUMBERS layer remains active  
// 4. Release B → Return to base layer
```

#### Stack Behavior Rules

**Stacking Up** 📚
- Each hold action pushes a new layer onto the stack
- Newer layers have priority over older ones
- Multiple layers can be active simultaneously

**Stacking Down** 📉  
- Only the key that activated a layer can deactivate it
- Releasing a key removes its layer from anywhere in the stack
- Other active layers remain unaffected

#### Real-World Example

```c
// Setup two Multi-Tap keys with hold actions
pipeline_tap_dance_action_config_t* left_thumb_actions[] = {
    createbehaviouraction_tap(1, KC_SPC),
    createbehaviouraction_hold(1, SYMBOLS_LAYER, TAP_DANCE_BALANCED)  // Layer 1
};

pipeline_tap_dance_action_config_t* right_thumb_actions[] = {
    createbehaviouraction_tap(1, KC_ENT), 
    createbehaviouraction_hold(1, NUMBERS_LAYER, TAP_DANCE_BALANCED)  // Layer 2
};
```

**Interaction Scenario:**

1. **Hold left thumb** → `SYMBOLS_LAYER` activated
   - Stack: `[BASE, SYMBOLS]`
   - Press `1` → outputs `!` (from symbols layer)

2. **Hold right thumb** (while still holding left)
   - Stack: `[BASE, SYMBOLS, NUMBERS]` 
   - Press `1` → outputs `1` (numbers layer has priority)

3. **Release left thumb** (right thumb still held)
   - Stack: `[BASE, NUMBERS]`
   - Press `1` → outputs `1` (numbers layer still active)

4. **Release right thumb**
   - Stack: `[BASE]`
   - Press `1` → outputs `1` (back to base layer)

#### Independent Layer Management

Each Multi-Tap key manages its own layer independently:

```c
// This won't interfere with layer stacking
pipeline_tap_dance_action_config_t* complex_actions[] = {
    createbehaviouraction_tap(1, KC_A),                               // 1 tap = A
    createbehaviouraction_tap(2, KC_B),                               // 2 taps = B  
    createbehaviouraction_hold(1, LAYER_NAV, TAP_DANCE_BALANCED),     // Hold after 1 tap = navigation
    createbehaviouraction_hold(2, LAYER_FUNC, TAP_DANCE_BALANCED)    // Hold after 2 taps = function keys
};
```

**Key Independence:**
- Tapping twice then holding activates `LAYER_FUNC`
- Other Multi-Tap keys can still activate their own layers
- All layers stack properly regardless of tap count used to activate them

#### Stack Edge Cases ⚠️

**Rapid Key Releases:**
```c
// Scenario: Hold A, Hold B, quickly release both
// Result: Both layers deactivate in release order
// Stack properly manages cleanup automatically
```

**Nested Same Layers:**
```c
// Multiple keys can activate the same layer number:
// Key A: Hold for LAYER_1
// Key B: Hold for LAYER_1 (same layer)
// 
// Both activations work independently - no conflicts
// Each key manages its own layer activation/deactivation
// Releasing either key removes only that key's layer contribution
```

**Duplicate Physical Key Presses:**
If the same physical key is pressed again before releasing (possible when reassigning keycodes):
```c
// Example: Key physically pressed twice before any release
// 1. First press → Multi-Tap sequence starts normally
// 2. Second press → Ignored (key already being processed)
// 3. First release → Ignored (doesn't match the active press)
// 4. Second release → Processed as normal release
//
// Only the last press/release pair is processed
```

This prevents conflicts when keys are remapped or when rapid key bouncing occurs.

**Memory Management:**
The layer stack automatically handles memory and prevents leaks - you don't need to manually track active layers.

#### Debugging Layer Stack

When troubleshooting layer issues:

1. **Check layer definitions**: Ensure layer numbers exist in your layout
2. **Verify key mappings**: Confirm each layer has the expected key assignments  
3. **Test isolation**: Try each Multi-Tap key individually first
4. **Timing conflicts**: Make sure hold timeouts don't interfere with each other

The layer stacking system is designed to "just work" - hold keys to stack layers, release keys to unstack them. Your muscle memory will quickly adapt to the natural flow. 🏗️

## Performance & Limitations

### Performance Impact
Almost negligible. Your screen refresh rate and finger movement speed matter way more than Multi-Tap processing time.

### Memory Constraints
Maximum custom keycodes: `0x00200000` to `0x7FFFFFFF` (over 1.5 billion possible custom actions)

### Conflict with Combos ⚠️

Multi-Tap can conflict with the "Combos" feature in rare cases:

**What's a Combo?** Combos detect multiple keys pressed simultaneously (like `Ctrl+C` but for any keys).

**The Conflict:** If you set up Multi-Tap with a hold action, and that same key combination is also configured as a Combo:
- **Combo wins**: It has higher priority
- **Multi-Tap loses**: The hold action might not trigger

**How to Avoid:**
```c
// BAD: This could conflict if a combo uses the same keys
// Multi-tap on A (hold for layer) + pressing B
// vs Combo for A+B = some action

// GOOD: Use different key combinations
// Multi-tap on A (hold for layer) + pressing C
// vs Combo for A+B = some action
```

This is rare in practice - you'd need to specifically configure conflicting patterns.

## Integration Example

Complete setup for a global Multi-Tap configuration:

```c
void setup_multitap() {
    // Create global config
    pipeline_tap_dance_global_config_t* tap_dance_config = malloc(sizeof(*tap_dance_config));
    tap_dance_config->length = 2;
    tap_dance_config->behaviours = malloc(2 * sizeof(pipeline_tap_dance_behaviour_t*));
    
    // Semicolon multi-tap
    pipeline_tap_dance_action_config_t* semi_actions[] = {
        createbehaviouraction_tap(1, KC_SCLN),
        createbehaviouraction_hold(1, MOVEMENT_LAYER, TAP_DANCE_BALANCED)
    };
    tap_dance_config->behaviours[0] = createbehaviour(SEMI_KEY, semi_actions, 2);
    
    // Space multi-tap
    pipeline_tap_dance_action_config_t* space_actions[] = {
        createbehaviouraction_tap(1, KC_SPC),
        createbehaviouraction_tap(2, KC_ENT),
        createbehaviouraction_hold(1, NUMBER_LAYER, TAP_DANCE_BALANCED)
    };
    tap_dance_config->behaviours[1] = createbehaviour(SPACE_KEY, space_actions, 3);
    
    // Initialize the system
    pipeline_tap_dance_global_state_create();
    
    // Register with the pipeline system
    pipeline_executor_register_callback(
        pipeline_tap_dance_callback_process_data_executor,
        pipeline_tap_dance_callback_reset_executor,
        tap_dance_config
    );
}
```

## Troubleshooting

**Multi-tap not triggering?**
- Check your `tap_timeout` - might be too short
- Verify the keycode matches your configuration

**Accidental holds?**
- Increase `hold_timeout`
- Switch to `TAP_DANCE_TAP_PREFERRED` strategy

**Layer not activating?**
- Confirm the layer number exists in your layout
- Check for Combo conflicts on the same key combination

**Timing feels off?**
- Start with 200ms for both timeouts
- Adjust based on your typing/gaming speed
- Faster typists often prefer shorter timeouts

---

Multi-Tap transforms any key into a Swiss Army knife of functionality. Start simple with one tap + one hold, then expand as you find your rhythm. Your muscle memory will thank you. 🎹
