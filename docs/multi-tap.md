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

Choose how your Multi-Tap behaves when other keys are pressed:

**`TAP_DANCE_HOLD_PREFERRED`** ⚡
- Activates hold immediately when another key is pressed
- Best for: Frequent layer access, gaming where speed matters
- Trade-off: Might accidentally trigger holds

**`TAP_DANCE_TAP_PREFERRED`** 🎯
- Waits full timeout before considering holds
- Best for: When you need reliable taps, typing-heavy workflows
- Trade-off: Slower layer activation

**`TAP_DANCE_BALANCED`** ⚖️
- Smart middle ground - considers context
- Best for: Most users, general purpose
- Trade-off: Slightly more complex behavior

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

### Reset Behavior
When you exceed the maximum configured taps, the sequence resets:
```c
// Max is 3 taps
// Tap 4 times → resets to tap 1, no action executed on the 4th tap
// This prevents accidental triggers and lets you restart sequences
```

## Layer Behavior

When a hold activates a layer:
- ✅ **Layer is momentary**: Active only while holding
- ✅ **Affects new key presses**: Keys pressed after activation use the new layer
- ✅ **Auto-deactivates**: Releasing the key returns to previous layer
- ✅ **Handles nesting**: Multiple layers can stack and unstack properly

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
