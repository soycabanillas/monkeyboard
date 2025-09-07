# Combo System: How Ordering Works

## The Problem: Conflicting Combos

When you press keys, multiple combos might activate at the same time. Since keys can't be "used" by multiple combos simultaneously, the system needs to pick which ones actually fire. This is where **combo order** becomes crucial.

Think of it like this: when you press `A+B+C`, both a `A+B` combo and a `A+B+C` combo could activate. The system looks at your combo list from top to bottom and picks the first valid set that doesn't share any keys.

## Example: Why Order Matters

Let's say you have these combos:
```
1. A+B → "Hello"
2. C+D → "World" 
3. A+B+C → "Goodbye"
4. B+C → "Maybe"
```

**Scenario:** You press `A+B+C+D` all within the timeout window.

**What happens:** 
- System checks combo #1 (`A+B`) ✅ - Can activate, uses keys A,B
- System checks combo #2 (`C+D`) ✅ - Can activate, uses keys C,D  
- System checks combo #3 (`A+B+C`) ❌ - Can't activate, A and B already used by combo #1
- System checks combo #4 (`B+C`) ❌ - Can't activate, B and C already used

**Result:** "Hello" + "World" fires, but "Goodbye" never gets a chance.

If you reorder them:
```
1. A+B+C → "Goodbye"  
2. C+D → "World"
3. A+B → "Hello"
4. B+C → "Maybe" 
```

**Same key press `A+B+C+D`:**
- System checks combo #1 (`A+B+C`) ✅ - Can activate, uses keys A,B,C
- System checks combo #2 (`C+D`) ❌ - Can't activate, C already used
- Result: Only "Goodbye" fires

## The Subset Problem

When one combo contains all the keys of another combo, the smaller one becomes "unreachable" if placed after the larger one.

**Example:**
```
1. A+B+C → "Big combo"
2. A+B → "Small combo"  
```

If you press just `A+B`, the "Small combo" will **never activate** because the system is still waiting to see if `C` gets pressed to complete the "Big combo". It will only fire after the timeout expires.

**The fix:** Put specific combos first:
```
1. A+B → "Small combo"
2. A+B+C → "Big combo"
```

Now `A+B` fires immediately when you press those keys, and `A+B+C` only fires if you press all three.

## Timing Consequences  

**⚠️ Important:** When a larger combo is placed before a smaller subset combo, the smaller one experiences **timeout delay**.

Instead of firing immediately when its keys are pressed, it waits for the full timeout period to see if the larger combo completes. This creates a noticeable delay that might feel unresponsive.

---

# UI Alert Messages

## Alert 1: Conflicting Combo Sets
**When to show:** Two or more combos share keys and could activate simultaneously.

**Message:**
> ⚠️ **Combo Conflict Detected**
> 
> The key combination you're adding conflicts with existing combos. When you press these keys, only the **higher-ordered combo** will activate.
> 
> **Conflicting with:** [List of combo names]
> 
> **Tip:** Reorder your combos to control which one has priority, or modify the key combinations to avoid conflicts.

## Alert 2: Subset Combo After Superset
**When to show:** User places a combo that's a subset of a previous combo.

**Message:**
> ⚠️ **Unreachable Combo**
> 
> This combo will **never activate** because "[Superset combo name]" (positioned above) contains all the same keys plus more.
> 
> **Fix:** Move this combo **above** "[Superset combo name]" in the list, or change the key combination.
> 
> **What happens if you don't:** This combo becomes impossible to trigger.

## Alert 3: Subset Combo Before Superset (Timeout Warning)
**When to show:** A smaller combo is correctly placed before a larger superset, but user should understand the timing.

**Message:**
> ℹ️ **Timing Notice**
> 
> Good ordering! "[Subset combo name]" will fire immediately when you press its keys.
> 
> **However:** "[Superset combo name]" will only activate if you press the additional keys within the timeout window. If you press keys slowly, you might trigger the smaller combo instead of the larger one.
> 
> **This is usually what you want** - just be aware of the timing behavior.

## Alert 4: Multiple Conflict Sets
**When to show:** Complex scenarios where multiple groups of combos conflict.

**Message:**
> ⚠️ **Multiple Combo Conflicts**
> 
> Your combo setup has several conflicting groups. Here's what will happen:
> 
> **Group 1:** [Combo A, Combo B] - Only the higher one activates  
> **Group 2:** [Combo C, Combo D] - Only the higher one activates
> 
> **Tip:** Review your combo order to ensure the most important combos are positioned higher in each conflict group.
