/*	SCCS Id: @(#)insight.c	0.0.7E7F3	2026/10/13	*/
/* NetHack may be freely redistributed.  See license for details. */

/*
 * Slash'EM Legacy: Insight, the help earned by doing things.
 *
 * Each Insight item switches on once the player has done enough of the
 * related thing, counted across all games and modes.  It is knowledge,
 * not power: it never costs Renown and can't be bought, and each item
 * can be switched off in the Hall's Settings.
 */

#include "hack.h"

const char *legacy_frozen_by = 0;

#define KILLS_THREAT	50L	/* kills for Threat sense */
#define CORPSES_SENSE	20L	/* corpses eaten for Corpse sense */
#define PRAYERS_SENSE	3L	/* safe prayers for Prayer sense */
#define IDS_PRICE	15	/* object types identified for Price sense */
#define WANDS_SENSE	10L	/* wands engrave-tested for Wand sense */
#define ELBERETH_QUICK	5L	/* Elbereths engraved for Quick Elbereth */

static const struct insight_def {
	const char *name;
	const char *desc;
} insights[NUM_INSIGHT] = {
	{ "Monster fact sheets",
	  "Looking at a monster you have killed shows its level, speed, armor class and attacks; after five kills, also its resistances, what eating it does and its deadliest tricks." },
	{ "Threat sense",
	  "Looking at a monster tells you whether it is harmless, easy, even, dangerous or deadly for you." },
	{ "Learned warnings",
	  "The game asks before you repeat something that has killed you, such as hitting a floating eye you can see." },
	{ "Corpse sense",
	  "The game warns you before you eat a corpse that is old, poisonous, acidic, petrifying or your own kind." },
	{ "Prayer sense",
	  "\"Pray\" appears on the status line while praying would be safe." },
	{ "Price sense",
	  "When a shopkeeper quotes a price for something unidentified, you learn which kinds of item match that price." },
	{ "Wand sense",
	  "When you engrave with an unidentified wand, you learn which wands could give that result." },
	{ "Quick Elbereth",
	  "The #elbereth command writes Elbereth in the dust in one step. On Android, add it to a command panel button." },
	{ "Sokoban memory",
	  "Each Sokoban layout you have got through has its solution in the Codex's Atlas." },
};

/* what has killed the hero before, for the learned warnings */
STATIC_OVL boolean
lesson_has(s)
const char *s;
{
	return legacy_know_lesson_like(s);
}

/* has a Lesson been learned that a learned warning answers? */
STATIC_OVL boolean
warning_learned(w)
int w;
{
	switch (w) {
	case LW_FLOATEYE:
	    return lesson_has("floating eye");
	case LW_TOUCH:
	    return lesson_has("cockatrice") || lesson_has("chickatrice");
	case LW_WATER:
	    return lesson_has("drown");
	case LW_LAVA:
	    return lesson_has("lava");
	case LW_FOUNTAIN:
	    return lesson_has("watchman") || lesson_has("watch captain");
	}
	return FALSE;
}

/* a death from bad food */
STATIC_OVL boolean
food_death()
{
	return lesson_has("rotted") || lesson_has("tainted")
	    || lesson_has("poisonous corpse") || lesson_has("acidic corpse")
	    || lesson_has("by eating");
}

boolean
insight_earned(i)
int i;
{
	int w;

	legacy_init();
	switch (i) {
	case INS_FACTS:
	    return codex_total_kills() > 0L;
	case INS_THREAT:
	    return codex_total_kills() >= KILLS_THREAT;
	case INS_WARN:
	    for (w = 0; w < NUM_LWARN; w++)
		if (warning_learned(w))
		    return TRUE;
	    return FALSE;
	case INS_CORPSE:
	    return lprof.icount[IC_CORPSES] >= CORPSES_SENSE || food_death();
	case INS_PRAYER:
	    return lprof.icount[IC_PRAYERS] >= PRAYERS_SENSE;
	case INS_PRICE:
	    return codex_armory_count() >= IDS_PRICE;
	case INS_WAND:
	    return lprof.icount[IC_WANDS] >= WANDS_SENSE;
	case INS_ELBERETH:
	    return lprof.icount[IC_ELBERETH] >= ELBERETH_QUICK;
	case INS_SOKOBAN:
	    return codex_count(-1) > 0;
	}
	return FALSE;
}

/* earned and not switched off */
boolean
insight_on(i)
int i;
{
	return (boolean) (insight_earned(i) && !lprof.ins_off[i]);
}

/* the items earned, as bits */
long
insight_mask()
{
	long m = 0L;
	int i;

	for (i = 0; i < NUM_INSIGHT; i++)
	    if (insight_earned(i))
		m |= 1L << i;
	return m;
}

const char *
insight_name(i)
int i;
{
	return insights[i].name;
}

const char *
insight_desc(i)
int i;
{
	return insights[i].desc;
}

/* how close an item is, such as "12 of 50 kills" */
void
insight_progress(i, buf)
int i;
char *buf;
{
	long n;
	int w, have = 0;

	switch (i) {
	case INS_FACTS:
	    Sprintf(buf, "%d kinds of monster killed", codex_count(-2));
	    break;
	case INS_THREAT:
	    n = codex_total_kills();
	    Sprintf(buf, "%ld of %ld kills", min(n, KILLS_THREAT), KILLS_THREAT);
	    break;
	case INS_WARN:
	    for (w = 0; w < NUM_LWARN; w++)
		if (warning_learned(w))
		    have++;
	    Sprintf(buf, "%d of %d warnings learned", have, NUM_LWARN);
	    break;
	case INS_CORPSE:
	    n = lprof.icount[IC_CORPSES];
	    Sprintf(buf, "%ld of %ld corpses eaten%s", min(n, CORPSES_SENSE),
		    CORPSES_SENSE, food_death() ? "; bad food has killed you"
					      : "");
	    break;
	case INS_PRAYER:
	    n = lprof.icount[IC_PRAYERS];
	    Sprintf(buf, "%ld of %ld successful prayers", min(n, PRAYERS_SENSE),
		    PRAYERS_SENSE);
	    break;
	case INS_PRICE:
	    Sprintf(buf, "%d of %d object types identified",
		    min(codex_armory_count(), IDS_PRICE), IDS_PRICE);
	    break;
	case INS_WAND:
	    n = lprof.icount[IC_WANDS];
	    Sprintf(buf, "%ld of %ld wands engrave-tested", min(n, WANDS_SENSE),
		    WANDS_SENSE);
	    break;
	case INS_ELBERETH:
	    n = lprof.icount[IC_ELBERETH];
	    Sprintf(buf, "%ld of %ld Elbereths engraved",
		    min(n, ELBERETH_QUICK), ELBERETH_QUICK);
	    break;
	case INS_SOKOBAN:
	    Sprintf(buf, "%d Sokoban layouts got through", codex_count(-1));
	    break;
	default:
	    *buf = '\0';
	}
}

/* count something toward an Insight item */
void
insight_count(c)
int c;
{
	legacy_init();
	lprof.icount[c]++;
	lprof.dirty = TRUE;
	insight_check();
}

/* announce any item earned since the last check */
void
insight_check()
{
	long now = insight_mask();
	int i;

	/* lrun.ins_told is saved with the game, so a restore doesn't
	   announce again */
	if (program_state.something_worth_saving && !program_state.gameover)
	    for (i = 0; i < NUM_INSIGHT; i++)
		if ((now & (1L << i)) && !(lrun.ins_told & (1L << i))) {
		    pline("You have gained Insight: %s.", insights[i].name);
		    pline("%s", insights[i].desc);
		}
	lrun.ins_told = now;
	if (now == (1L << NUM_INSIGHT) - 1L)
	    legacy_feat(F_INSIGHT);
}

/* ------------------------------------------------------------------ */
/* learned warnings */

boolean
insight_warning(w)
int w;
{
	return (boolean) (insight_on(INS_WARN) && warning_learned(w));
}

/* about to hit mtmp in melee; TRUE means the player changed their mind */
boolean
insight_hit_check(mtmp, barehanded)
struct monst *mtmp;
boolean barehanded;
{
	char qbuf[QBUFSZ];

	if (!canseemon(mtmp) || Hallucination || Confusion || Stunned)
	    return FALSE;
	if (mtmp->data == &mons[PM_FLOATING_EYE] && !Blind && mtmp->mcansee
		&& !Free_action && !Reflecting
		&& insight_warning(LW_FLOATEYE)) {
	    Sprintf(qbuf, "Its gaze froze you before. Really hit %s?",
		    mon_nam(mtmp));
	    if (yn(qbuf) != 'y')
		return TRUE;
	}
	if (touch_petrifies(mtmp->data) && barehanded && !uarmg
		&& !Stone_resistance && !poly_when_stoned(youmonst.data)
		&& insight_warning(LW_TOUCH)) {
	    Sprintf(qbuf, "Touching %s can turn you to stone. Attack anyway?",
		    mon_nam(mtmp));
	    if (yn(qbuf) != 'y')
		return TRUE;
	}
	return FALSE;
}

/* about to drink from or dip into a fountain; TRUE: don't */
boolean
insight_fountain_check()
{
	struct monst *mtmp;

	if (!insight_warning(LW_FOUNTAIN) || !in_town(u.ux, u.uy))
	    return FALSE;
	for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
	    if (!DEADMONSTER(mtmp) && (mtmp->data == &mons[PM_WATCHMAN]
		    || mtmp->data == &mons[PM_WATCH_CAPTAIN])
		    && mtmp->mpeaceful && couldsee(mtmp->mx, mtmp->my)) {
		if (yn("The watch will be angry if the fountain dries up. Use it anyway?")
			!= 'y')
		    return TRUE;
		break;
	    }
	return FALSE;
}

/* ------------------------------------------------------------------ */
/* corpse sense */

/* about to eat a corpse; TRUE means don't */
boolean
insight_corpse_check(otmp)
struct obj *otmp;
{
	int mnum = otmp->corpsenm;
	struct permonst *ptr;
	const char *why = 0;
	long rotted = 0L;
	char qbuf[QBUFSZ];

	if (otmp->otyp != CORPSE || mnum < LOW_PM || !insight_on(INS_CORPSE)
		|| Hallucination)
	    return FALSE;
	ptr = &mons[mnum];
	if (mnum != PM_LIZARD && mnum != PM_LICHEN) {
	    rotted = (monstermoves - peek_at_iced_corpse_age(otmp)) / 10L;
	    if (otmp->cursed) rotted += 2L;
	    else if (otmp->blessed) rotted -= 2L;
	}
	if (touch_petrifies(ptr) && !Stone_resistance
		&& !poly_when_stoned(youmonst.data))
	    why = "would turn you to stone";
	else if (mnum == PM_GREEN_SLIME && !Unchanging
		&& youmonst.data != &mons[PM_GREEN_SLIME])
	    why = "would turn you into slime";
	else if (mnum != PM_ACID_BLOB && rotted > 5L && !Sick_resistance
		&& u.umonnum != PM_GHOUL && u.umonnum != PM_GHAST)
	    why = "old enough to be tainted, which can kill you";
	else if (your_race(ptr) && !Role_if(PM_CAVEMAN) && !Race_if(PM_ORC)
		&& !Race_if(PM_HUMAN_WEREWOLF) && !Race_if(PM_VAMPIRE))
	    why = "one of your own kind: eating it angers your god";
	else if (is_were(ptr) && !Race_if(PM_HUMAN_WEREWOLF))
	    why = "a lycanthrope, and would give you lycanthropy";
	else if (poisonous(ptr) && !Poison_resistance)
	    why = "poisonous";
	else if (acidic(ptr) && !Acid_resistance)
	    why = "acidic";
	else if (mnum != PM_ACID_BLOB && rotted > 3L)
	    why = "old and may be rotten";
	if (!why)
	    return FALSE;
	Sprintf(qbuf, "That corpse is %s. Eat it anyway?", why);
	return (boolean) (yn(qbuf) != 'y');
}

/* ------------------------------------------------------------------ */
/* prayer sense */

boolean
insight_pray_marker()
{
	return (boolean) (program_state.something_worth_saving
			  && insight_on(INS_PRAYER) && prayer_is_safe());
}

/* once a turn: redraw the status line when the prayer marker changes */
void
insight_turn()
{
	boolean safe = insight_pray_marker();

	if (safe != lrun.pray_safe) {
	    lrun.pray_safe = safe;
	    flags.botl = 1;
	}
}

/* ------------------------------------------------------------------ */
/* wand sense */

#define WG_NONE		0	/* nothing to see */
#define WG_FIGHT	1	/* "fights your attempt to write" */
#define WG_SLOW		2
#define WG_SPEED	3
#define WG_HEAL		4
#define WG_FEAR		5
#define WG_STOP		6	/* the bugs stop moving */
#define WG_HOLES	7	/* riddled by bullet holes */
#define WG_ICE		8
#define WG_VANISH	9	/* the engraving vanishes */
#define WG_CHANGE	10	/* the engraving changes */
#define WG_IDENT	11	/* the wand identifies itself */

/* what engraving with this wand shows; 'old' means there was an
   engraving to act on */
STATIC_OVL int
wand_group(otyp, old)
int otyp;
boolean old;
{
	switch (otyp) {
	case WAN_STRIKING:		return WG_FIGHT;
	case WAN_SLOW_MONSTER:		return WG_SLOW;
	case WAN_SPEED_MONSTER:		return WG_SPEED;
	case WAN_HEALING:
	case WAN_EXTRA_HEALING:		return WG_HEAL;
	case WAN_FEAR:			return WG_FEAR;
	case WAN_SLEEP:
	case WAN_DEATH:			return WG_STOP;
	case WAN_MAGIC_MISSILE:		return WG_HOLES;
	case WAN_COLD:			return WG_ICE;
	case WAN_CANCELLATION:
	case WAN_MAKE_INVISIBLE:
	case WAN_TELEPORTATION:		return old ? WG_VANISH : WG_NONE;
	case WAN_POLYMORPH:		return old ? WG_CHANGE : WG_NONE;
	case WAN_DRAINING:		return old ? WG_VANISH : WG_NONE;
	case WAN_DIGGING:
	case WAN_FIRE:
	case WAN_FIREBALL:
	case WAN_LIGHTNING:		return WG_IDENT;
	case WAN_LIGHT:
	case WAN_SECRET_DOOR_DETECTION:
	case WAN_CREATE_MONSTER:
	case WAN_CREATE_HORDE:
	case WAN_ENLIGHTENMENT:
	case WAN_WISHING:		/* these zap with no direction; when
					   nothing visible happens, it looks
					   like any other quiet wand */
	default:			return WG_NONE;
	}
}

/* the hero engraved with an unidentified wand of type otyp and saw its
   effect: name the wands that look the same */
void
insight_wand_sense(otyp, old)
int otyp;
boolean old;
{
	char buf[BUFSZ];
	int i, n = 0, cnt = 0, grp = wand_group(otyp, old);

	if (!insight_on(INS_WAND) || grp == WG_IDENT)
	    return;
	for (i = bases[WAND_CLASS]; i < NUM_OBJECTS
		&& objects[i].oc_class == WAND_CLASS; i++)
	    if (OBJ_NAME(objects[i]) && (i == otyp || !objects[i].oc_name_known)
		    && wand_group(i, old) == grp)
		cnt++;
	Strcpy(buf, cnt == 1 ? "Wand sense: that can only be a wand of "
			     : "Wand sense: that could be a wand of ");
	for (i = bases[WAND_CLASS]; i < NUM_OBJECTS
		&& objects[i].oc_class == WAND_CLASS; i++)
	    if (OBJ_NAME(objects[i]) && (i == otyp || !objects[i].oc_name_known)
		    && wand_group(i, old) == grp) {
		n++;
		if (n > 1)
		    Strcat(buf, n == cnt ? " or " : ", ");
		if (strlen(buf) > BUFSZ - 40) {
		    Strcat(buf, "...");
		    break;
		}
		Strcat(buf, OBJ_NAME(objects[i]));
	    }
	Strcat(buf, ".");
	pline("%s", buf);
	if (grp == WG_NONE && !old)
	    pline("(Engraving over an existing engraving would tell you more.)");
}

/*insight.c*/
