# Hold-Tap Behavior Configuration Specification

## Hold Decision Triggers
*When to activate hold behavior:*

### On Key Press Or Tapping Term
- Activate hold behavior when another key is pressed OR when tapping term expires

### On Key Press And Release Or Tapping Term
- Activate hold behavior when the same key is pressed AND released OR when tapping term expires

### On Tapping Term Only
- Activate hold behavior only when tapping term expires (ignore key interrupts)

## Retroactive Key Processing
*How to handle keys in the window between main key down and hold decision:*

### Include Pressed Keys
- Apply hold behavior to all keys that were pressed between main key down and hold decision

### Include Pressed And Released Keys
- Apply hold behavior to all keys that were both pressed AND released between main key down and hold decision

## Post-Release Key Processing
*How to handle keys pressed between hold decision and main key release (but released after main key release):*

### Time-Based Layer Assignment (milliseconds)
- `0`: All keys pressed during hold use hold layer (even if released after main key)
- `>0`: Keys pressed within X ms before main key release use original layer; keys pressed earlier use hold layer

---

## Time Windows

```
main_key_down → [Retroactive Window] → **HOLD DECIDED** → [Post-Release Window] → main_key_up
```

- **Retroactive Key Processing:** Manages keys between `main_key_down` and `**HOLD DECIDED**`
- **Post-Release Key Processing:** Manages keys pressed between `**HOLD DECIDED**` and `main_key_up` (but released after `main_key_up`)

---

## Example Usage

### Complete Example:
```
main_key_down → A_down → A_up → **HOLD DECIDED** → B_down → main_key_up → B_up
```
- **A_down/A_up**: Managed by Retroactive Key Processing
- **B_down/B_up**: Managed by Post-Release Key Processing

### Post-Release = 0:
```
main_key_down → hold_decided → A_down → main_key_up → A_up
// A_up uses hold layer
```

### Post-Release = 50ms:
```
main_key_down → hold_decided → A_down → [48ms] → main_key_up → A_up
// A_up uses original layer (pressed within 50ms of main key release)

main_key_down → hold_decided → B_down → [60ms] → main_key_up → B_up  
// B_up uses hold layer (pressed more than 50ms before main key release)
```

## Example Mappings to Existing Systems

### QMK HOLD_ON_OTHER_KEY_PRESS / ZMK Hold-Preferred:
- **Hold Decision:** On Key Press Or Tapping Term
- **Retroactive:** Include Pressed Keys

### QMK PERMISSIVE_HOLD / ZMK Balanced:
- **Hold Decision:** On Key Press And Release Or Tapping Term  
- **Retroactive:** Include Pressed And Released Keys

### QMK Default / ZMK Tap-Preferred:
- **Hold Decision:** On Tapping Term Only
- **Retroactive:** Include Pressed Keys
