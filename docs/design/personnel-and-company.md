# Personnel & Company Management System

> Living design reference for the personnel system, company structure, and organisational growth mechanics. Pre-implementation, subject to revision.

---

## 1. Design Philosophy

The company management layer sits alongside — not above — the core gameplay of building rockets, managing contracts, and operating space infrastructure. It should be:

- **Meaningful but not overwhelming** — rich enough that personnel decisions matter, simple enough that the player can focus on orbital mechanics when needed
- **Hands-off by default, high-leverage when engaged** — most of the time the system runs itself; player intervention should feel like surgery, not maintenance
- **Self-teaching** — the early game forces close engagement with the system; the late game rewards that understanding with delegation

---

## 2. Employee Stats

Every employee has a set of stats, each with a **hidden minimum and maximum**. The current value sits somewhere in that range and can be moved by traits, effects, and events — but not beyond the bounds (with rare exceptions).

### 2.1 Stat Categories

Stats fall into three categories based on how they change:

| Category | Stats | Notes |
|---|---|---|
| **Intrinsic** | Ambition, core competency ceiling | Mostly fixed at hire; represent potential |
| **Developed** | Experience, Institutional Knowledge contribution, Leadership | Grow through activity and conditions |
| **Situational** | Motivation | Most volatile; responds to environment, management quality, blocked ambition |

### 2.2 Stat Definitions

**Motivation**
- Output multiplier — high motivation amplifies performance, low motivation suppresses it
- Most volatile stat; fluctuates in response to team conditions, leadership quality, blocked ambition, recent events
- A demotivated but experienced employee is a slow drain, not a catastrophe — they know what *not* to do

**Ambition**
- Drives pressure to be promoted or given more responsibility
- Not universally high — some employees are content as excellent individual contributors
- Blocked ambition (no promotion path, no stretch opportunity) generates a team morale penalty proportional to the ambition stat

**Core Competency**
- Specialised to a domain (engineering, HR, software, logistics, etc.)
- Cross-domain transfers not modelled — assume people stay in their field
- Has a hidden ceiling that represents the employee's ultimate potential in their domain

**Communication**
- Affects team coordination efficiency
- Low communication on a team leader has outsized negative effects
- Interacts with traits (e.g. "Brilliant but Abrasive")

**Leadership**
- Required for team leader role; unfilled or weak leadership applies a team output penalty
- Develops only through actually leading — high-potential employees need the opportunity
- Distinct from competency: a great engineer is not necessarily a great engineering manager

**Experience**
- Accumulated through time spent working in role
- Scales output contribution; also acts as a floor — experienced employees rarely cause catastrophic failures
- Gained faster under a leader with the **Trainer** trait and when a **Mentor** is present in the team

**Institutional Knowledge**
- Represents familiarity with company processes, history, relationships, and context
- Starts near zero; grows over time, faster under good leadership and mentoring
- Is a **team-level aggregate** — the sum of all members' individual contributions
- Concentration risk: if one employee holds disproportionate share and leaves, the loss is severe — but this emerges naturally from the aggregation, not a separate mechanic
- Losing many people quickly causes a sharp drop

### 2.3 Hidden Max Revelation

- At hire, the player sees the **minimum** value of each stat and knows there is a range — not the ceiling
- The ceiling is slowly revealed through:
  - **Time** — a year of stable employment gives a reasonable picture
  - **Evaluation events** — performance reviews narrow the bracket
  - **HR competency** — better HR staff provide tighter estimates on candidates at hire
- After approximately one year of competent management, most employees should be well understood
- **Edge cases** (rare, memorable):
  - Employee who performs steadily then hits an unexpected ceiling
  - Employee with hidden reserves that only emerge under crisis conditions

### 2.4 Stat Exceptions

Some traits or events can push stats **beyond their normal min/max**:
- **Above ceiling**: rare, reserved for exceptional traits (e.g. "Prodigy") — creates star performers
- **Below floor**: crisis states (e.g. "Burnout") — creates urgent intervention situations
- These should be mechanically distinct and feel qualitatively different from normal stat movement

---

## 3. Traits and Modifiers

Traits are persistent modifiers attached to employees. They are not stat adjustments alone — they can carry **conditional effects** that activate based on context.

### 3.1 Trait Design Principle

> A trait should behave differently in different team contexts. The same trait should be an asset in one configuration and a liability in another.

### 3.2 Example Traits

**Mentor**
- Increases Institutional Knowledge gain rate for other team members
- Makes succession transitions significantly less damaging

**Empire Builder**
- Reduces Institutional Knowledge gain of *other* team members (knowledge hoarding)
- May inflate headcount requests beyond what is warranted
- Valuable for growth phases, destructive in stable teams

**Trainer** *(leadership trait)*
- Accelerates experience gain for direct reports
- Amplifies the benefit of having a Mentor in the team

**Lazy**
- Reduces personal output
- Under strong leadership: managed adequately
- Under weak/absent leadership: becomes a visible drag
- Under a **Micromanager** leader: motivation penalty compounds — being watched makes it worse

**Brilliant but Abrasive**
- High competency, low communication score
- Applies a morale penalty to nearby employees
- Penalty negated if recipient has **Thick-Skinned** trait

**Clock-Watcher**
- Performs exactly to job description — no gap-filling, no initiative
- Reliable in mature stable teams; problematic in dynamic or understaffed ones

**Micromanager** *(leadership trait)*
- Reduces autonomy of direct reports; compounds negative traits like Lazy
- Suppresses motivation in high-ambition employees
- May produce consistent output short-term while quietly destroying morale

**Veteran** *(emergent, time-based)*
- Unlocked after sufficient tenure and breadth of experience
- Represents understanding of the whole system, not just one domain
- Uniquely valuable when standing up new teams or diagnosing cross-team problems
- Losing the last Veteran is a strategic event

### 3.3 Ambition Archetypes

Not all employees respond to blocked promotion the same way:

| Ambition | Competency | Behaviour |
|---|---|---|
| High | High | Will leave or cause visible friction if blocked; lateral moves help |
| High | Low | Friction through political behaviour, blame-shifting |
| Low | High | Reliable backbone; content in role; valuable but may occupy slots |
| Low | Low | Neutral drag; manageable in small numbers |

---

## 4. Teams

### 4.1 Team Structure

- Every team has an assigned **leader slot**
- Unfilled leader slot applies a **team output penalty** (scales with team size and task complexity — a small experienced team self-organises better than a large or complex one)
- Teams have a collective **Institutional Knowledge** score (sum of member contributions)
- Teams are **permanently assigned** to a position in the Work Pyramid

### 4.2 Team Output

Team output is the primary metric. It feeds directly into company-level work items:
- Rocket manufacturing speed
- Purchase contract fulfilment
- Launch cadence
- Hiring pipeline throughput
- Training completion rates
- etc.

Output is affected by:
- Leader quality (leadership stat + relevant traits)
- Average competency of members
- Motivation levels
- Institutional Knowledge (provides a floor — reduces catastrophic failures)
- Trait interactions between members
- Whether the team is in its permanent pyramid slot or temporarily reassigned

### 4.3 Succession and Bus Factor

- Teams with a **Mentor** present recover faster from leadership loss
- Teams where knowledge is concentrated in one person are fragile — this emerges from the IK distribution, not a separate flag
- A **Veteran** in the team significantly accelerates recovery and knowledge transfer during splits

### 4.4 Temporary Reassignment

- Teams can be pulled from their pyramid slot for urgent work elsewhere
- Penalties:
  - Context-switching cost to the team (motivation dip, temporary output reduction)
  - The vacated pyramid slot degrades — work backs up
  - Reintegration cost on return
- This should feel like surgery — a deliberate trade-off, not a free action

---

## 5. The Work Pyramid

### 5.1 Concept

Work items are organised into a hierarchy. Teams are permanently assigned to levels of the pyramid. Early-stage companies have one team covering an entire branch; growth warrants splitting into specialist sub-teams further down.

```
        [Motor Assembly]
       /        |        \
  [Cooling] [Housing] [Electronics]
```

At the top: generalist oversight. At the bottom: deep specialist execution.

### 5.2 Team Splitting

When a team splits:
- Institutional Knowledge divides (imperfectly — the cooling specialists carry some housing knowledge away with them)
- The existing leader probably cannot lead both successors
- Hidden stat ceilings may be revealed — who is the natural leader of the new team?
- A Veteran or Mentor present at split time significantly reduces transition cost

### 5.3 Cross-Team Dependencies

Downstream teams are blocked by upstream failures. A bottleneck in the cooling team delays motor assembly. This creates a dependency graph the player reads to diagnose blockages — not a list of team health numbers.

HR dependency chains are particularly insidious: a weak hiring team starves all other teams of new blood, but with a lag that obscures the cause.

---

## 6. HR as a System

### 6.1 HR Work Metric

- Every team generates HR work (hiring, performance management, training, conflict resolution)
- Work scales with team count but not linearly — many small teams generate more HR work than fewer large ones
- Insufficient HR capacity causes teams to suffer (longer vacancies, unresolved conflicts, undertrained staff)

### 6.2 HR Specialisation Over Time

Early game: one HR team handles everything.

As the company grows, HR work warrants dedicated sub-teams:
- **Hiring team** — manages pipeline, vets candidates
- **Performance management team** — reviews, PIPs, promotions
- **Training team** — onboarding, skill development, leadership programmes

Same pyramid logic applies. Same splitting costs apply.

### 6.3 HR as Scout

HR competency affects the **quality of information** the player has:
- Better HR staff provide tighter min/max brackets on candidates at hire
- Evaluation events (performance reviews) are more accurate with a competent PM team
- A weak HR director follows policy but misses intent — promotes the technically eligible wrong candidate

### 6.4 HR Work Types

Two flavours of HR work have meaningfully different properties:

| Type | Examples | Character |
|---|---|---|
| **Routine** | Hiring, onboarding, scheduled reviews | Predictable, scales with headcount |
| **Intervention** | Conflicts, performance issues, succession crises | Spiky, urgent, penalty for delay |

A single HR work number flattens this — consider surfacing both to the player.

---

## 7. Delegation and Strategic Control

### 7.1 The Delegation Gradient

Delegation is not a binary unlock. It is a progression:

1. **Player makes every decision** — early game, full visibility, full workload
2. **Player sets policies** — "prioritise internal promotion," "maintain two months of hiring pipeline," "flag motivation drops before acting"
3. **Computer executes within policies** — quality of execution scales with HR director's stats
4. **Player sets strategic priorities only** — headcount targets, quality bars, growth vs. stability

### 7.2 Threshold-Triggered Decisions

When work in an area crosses a threshold, a **strategic decision unlocks**. This is the core mechanic of delegation progression.

Properties:
- The threshold crossing explains *why* the decision is available now — it is self-documenting
- Decisions can be reversed — encourages experimentation
- Two players with different growth strategies hit decisions in different orders

Example chain for hiring:
```
Hiring work > threshold_1 → Unlock: Set minimum candidate standards (slims pipeline)
Hiring work > threshold_2 → Unlock: Allow department heads to hire automatically
Hiring work > threshold_3 → Unlock: Dedicated hiring sub-team warranted
```

This grammar generalises across all departments:
- Engineering complexity → team split decisions
- Launch cadence → dedicated scheduling function
- Training backlog → training sub-team

### 7.3 Failure Modes Under Delegation

- **Escalation creep**: a low-judgment HR director escalates everything back to the player, defeating the purpose
- **Silent failure**: a low-judgment director makes bad decisions quietly; player only notices six months later when teams underperform
- **Policy conflict**: rapid expansion + quality bars will eventually conflict; director's judgment stat determines how well this is navigated without escalation

### 7.4 Strategic Visibility

Full delegation risks invisible drift. Periodic strategic reports surface systemic issues without granular noise:
- "Mid-level engineering pipeline has thinned considerably"
- "Three teams flagging leadership quality issues"
- "Institutional Knowledge in Launch Operations has declined 30% over six months"

Player stays strategic; not flying blind.

---

## 8. Growth Arc

| Phase | Company State | Player Engagement |
|---|---|---|
| **Early** | One or two teams, generalists, no delegation | High — every hire, every promotion decided manually |
| **Mid** | Splitting teams, partial specialisation, first delegation unlocks | Medium — managing transitions, setting first policies |
| **Late** | Deep specialist pyramid, full HR sub-department, most decisions delegated | Low routine — high-leverage interventions only; strategic priority setting |

The mid-game is intentionally the hardest to navigate. The late game rewards surviving it with a largely self-managing organisation.

---

## 9. Open Questions

- [ ] Does the Peter Principle (promoting to incompetence) need an explicit mechanic, or does the competency vs. leadership stat split handle it implicitly?
- [ ] Lateral moves as ambition pressure valve — needs a concrete mechanic for how the player initiates and what the costs/benefits are
- [ ] Knowledge concentration risk flag — does this need to be surfaced explicitly or is the natural IK drop on departure sufficient?
- [ ] Large team coordination overhead — countervailing pressure to prevent optimal strategy collapsing to "one giant team"
- [ ] Morale as a distinct team-level stat vs. emergent from individual motivations?
- [ ] How does the Veteran trait interact with team splits — does one Veteran's presence in a split team mean the other team loses disproportionately?
