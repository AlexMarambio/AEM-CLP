# Implementation Continuation Guide

## Next Task: Complete BlockSwapOperator

### File to Edit
`/home/aropunto/Escritorio/AEM-CLP/metasolver/strategies/BlockSwapOperator.cpp`

### What to Implement

#### 1. `isValidSwap()` method (lines ~58-60)
**Purpose**: Check if two blocks can swap positions

**Logic**:
```cpp
// Get dimensions of both blocks from container
// Check if block1 fits in block2's position
// Check if block2 fits in block1's position
// Return true only if both fit
```

**Key Methods to Use**:
- `s.cont->blocks->top()` - Get first AABB
- `aabb.getL()`, `aabb.getW()`, `aabb.getH()` - Get dimensions
- Iterate using `has_next()` and `next()`

#### 2. `performSwap()` method (lines ~64-68)
**Purpose**: Create new state with blocks swapped

**Logic**:
```cpp
// 1. Clone input state
// 2. Find AABBs at positions block1_idx and block2_idx
// 3. Remove both blocks from container
// 4. Reinsert in swapped positions
// 5. Return new state
```

**Pseudo-code**:
```cpp
clpState* result = dynamic_cast<clpState*>(s.clone());
// Get block1 and block2 positions
Vector3 pos1 = aabb1.getMins();
Vector3 pos2 = aabb2.getMins();
// Remove, swap, reinsert
result->cont->erase(aabb1);
result->cont->erase(aabb2);
result->cont->insert(block1_ref, pos2);
result->cont->insert(block2_ref, pos1);
return result;
```

---

## Next Task: Complete BlockRotateOperator

### File to Edit
`/home/aropunto/Escritorio/AEM-CLP/metasolver/strategies/BlockRotateOperator.cpp`

### What to Implement

#### `attemptRotationAndReplacement()` method (lines ~75-85)
**Purpose**: Rotate a block and reinserter it optimally

**Logic**:
```cpp
// 1. Clone state
// 2. Remove block at block_idx
// 3. Change block dimensions to new_dims
// 4. Find best position for rotated block
// 5. Reinsert block
// 6. Return modified state or NULL if fails
```

**Pseudo-code**:
```cpp
clpState* rotated = dynamic_cast<clpState*>(s.clone());
// Get block at block_idx
const AABB& aabb = getBlockAtIndex(rotated, block_idx);
// Remove block
rotated->cont->erase(aabb);
// Create modified block with new dimensions
// Get all free spaces
list<const Space*> spaces = rotated->cont->spaces->getAll();
// Find best space for new dimensions
// If found: insert and return rotated
// Else: return NULL (revert)
```

---

## Testing Each Implementation

### Test BlockSwapOperator
```bash
cd /home/aropunto/Escritorio/AEM-CLP/build
# Build after editing
make

# Run test
./BSG_CLP ../problems/clp/benchs/BR/BR0.txt -i 0 -t 10 -f BR --strategy vns

# Expected: Better results than current (should improve beyond initial state)
```

### Test BlockRotateOperator
Same as above - run full test and check if results improve

---

## Key APIs Reference

### Getting blocks from container
```cpp
// Start iteration
const AABB& first = s.cont->blocks->top();

// Loop through blocks
while(true) {
    // Process current AABB
    // ...
    
    // Check for more blocks
    if(s.cont->blocks->has_next()) {
        AABB& next_aabb = s.cont->blocks->next();
    } else {
        break;
    }
}
```

### Accessing block information
```cpp
AABB aabb = /* from iteration */;
double l = aabb.getL();
double w = aabb.getW();
double h = aabb.getH();
Vector3 position = aabb.getMins();
const Block* block = aabb.getBlock();
```

### Modifying container
```cpp
// Remove block
s.cont->erase(aabb);

// Add block
s.cont->insert(aabb);

// Get free spaces
list<const Space*> spaces;
// Use s.cont->spaces to get free spaces
```

### State operations
```cpp
// Clone state
clpState* new_state = dynamic_cast<clpState*>(s.clone());

// Apply action (transition)
new_state->transition(*action);

// Get state value
double value = new_state->get_value();

// Get all possible actions
list<Action*> actions;
new_state->get_actions(actions);
```

---

## Debugging Checklist

- [ ] Compilation succeeds without errors
- [ ] No new compiler warnings
- [ ] VNS runs without crashing
- [ ] Results improve (compared to current BR0/BR4/BR8)
- [ ] Results are reasonable (don't exceed 100% or go negative)
- [ ] Performance is acceptable (completes within 30 seconds)

---

## Performance Expectations

After implementing BlockSwapOperator and BlockRotateOperator:
- **BR0-BR2** (homogeneous): Expect 1-3% improvement
- **BR4-BR7** (weakly heterogeneous): Expect 2-5% improvement
- **BR8-BR15** (strongly heterogeneous): Expect 3-7% improvement

These are rough estimates - actual results depend on instance characteristics.

---

## Estimated Effort

- BlockSwapOperator implementation: **15-20 min**
- BlockRotateOperator implementation: **20-30 min**
- Testing & debugging: **20-30 min**
- **Total**: 55-80 minutes for production-ready VNS

After this, the remaining tasks are:
- Benchmark testing (30 min)
- Results analysis & table generation (30 min)
- Academic report writing (1-2 hours)

---

## Alternative: Use Existing Action API (Simpler Approach)

If direct block manipulation seems complex, consider using the existing action API:

```cpp
// Instead of direct manipulation, use get_actions() like RemoveReinsert does:
list<Action*> actions;
s->get_actions(actions);

// For each action:
for(auto action : actions) {
    State* neighbor = s->clone();
    neighbor->transition(*action);
    // Evaluate neighbor
}
```

This is simpler but less targeted. For full VNS effectiveness, implement proper operators.

---

## Success Indicators

✅ **You'll know it's working when**:
1. Code compiles without errors
2. `./BSG_CLP ... --strategy vns` produces results > 0
3. Results improve gradually during execution (visible in traces)
4. VNS results match or exceed BSG after 30+ seconds
5. No crashes or memory issues

❌ **Troubleshooting signs**:
- Results are 0 or very low → Operators not generating valid neighbors
- Program crashes → Memory management issue (likely state cloning)
- No improvement → Operators not improving solutions (stubs are still there)
- Very slow → Operators exploring too many neighbors (tune limit)

