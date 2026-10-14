/*	SCCS Id: @(#)draft.c	0.0.7E7F3	2026/10/14	*/
/* NetHack may be freely redistributed.  See license for details. */

/*
 * Slash'EM Legacy: fate drafts.  The first time a character reaches a
 * milestone, the game offers a few fates and the player keeps one.  A
 * fresh profile sees two fates at four milestones; the Fortune track of
 * the Lineage widens that to three fates, all eight milestones and a
 * reroll.  Each fate comes from one of four families, and one draft
 * never shows two from the same family.
 */

#include "hack.h"

#define FAM_BODY	0	/* an intrinsic */
#define FAM_CRAFT	1	/* skill, spell or enchantment */
#define FAM_FORTUNE	2	/* identification or an item */
#define FAM_BARGAIN	3	/* a big gain with a lasting cost */
#define NUM_FAMILIES	4

#define SKIP_RENOWN	15

enum fate_id {
	/* early: the Oracle to Mines' End */
	FT_POISON, FT_HP5, FT_STEALTH, FT_INFRA,
	FT_ARMOR1, FT_SPELL1, FT_SLOT1,
	FT_ID2, FT_HEAL2, FT_REMCURSE, FT_GOLD,
	FT_STRENGTH, FT_SPEEDLUCK,
	/* middle: the Quest portal and Medusa */
	FT_SPEED, FT_SLEEP, FT_FIRE, FT_COLD, FT_SHOCK, FT_SEEINV,
	FT_WEAPON1, FT_SPELL23, FT_SLOT1M, FT_ERODEPROOF,
	FT_ID4, FT_RING, FT_ENCHANT,
	FT_TELEPATHY, FT_TPORTITIS,
	/* late: the Castle and the Valley */
	FT_MAGICRES, FT_REFLECT, FT_REGEN, FT_FREEACT, FT_TCONTROL,
	FT_ITEM2, FT_SPELL45, FT_SLOT2,
	FT_IDALL, FT_GENOCIDE, FT_CHARGING,
	FT_WISH, FT_REFLECTHP,
	NUM_FATES
};

static const struct fate_def {
	const char *name;
	xchar tier, family, weight;
} fates[NUM_FATES] = {
	{ "poison resistance", DT_EARLY, FAM_BODY, 3 },
	{ "+5 maximum hit points", DT_EARLY, FAM_BODY, 3 },
	{ "stealth", DT_EARLY, FAM_BODY, 3 },
	{ "infravision", DT_EARLY, FAM_BODY, 3 },
	{ "+1 to a piece of worn armor", DT_EARLY, FAM_CRAFT, 3 },
	{ "a level 1 spell from your best school", DT_EARLY, FAM_CRAFT, 3 },
	{ "one more skill slot", DT_EARLY, FAM_CRAFT, 3 },
	{ "identify two items", DT_EARLY, FAM_FORTUNE, 3 },
	{ "two potions of healing", DT_EARLY, FAM_FORTUNE, 3 },
	{ "a blessed scroll of remove curse", DT_EARLY, FAM_FORTUNE, 3 },
	{ "300 gold pieces", DT_EARLY, FAM_FORTUNE, 3 },
	{ "+2 Strength, but you hunger 25% faster", DT_EARLY, FAM_BARGAIN, 3 },
	{ "speed, but Luck -1 for the rest of the game", DT_EARLY, FAM_BARGAIN,
	  3 },

	{ "speed", DT_MIDDLE, FAM_BODY, 3 },
	{ "sleep resistance", DT_MIDDLE, FAM_BODY, 3 },
	{ "fire resistance", DT_MIDDLE, FAM_BODY, 1 },
	{ "cold resistance", DT_MIDDLE, FAM_BODY, 1 },
	{ "shock resistance", DT_MIDDLE, FAM_BODY, 1 },
	{ "see invisible", DT_MIDDLE, FAM_BODY, 3 },
	{ "+1 to your wielded weapon", DT_MIDDLE, FAM_CRAFT, 3 },
	{ "a level 2 or 3 spell", DT_MIDDLE, FAM_CRAFT, 3 },
	{ "one more skill slot", DT_MIDDLE, FAM_CRAFT, 3 },
	{ "your worn armor can't rust, burn or rot", DT_MIDDLE, FAM_CRAFT, 3 },
	{ "identify four items", DT_MIDDLE, FAM_FORTUNE, 3 },
	{ "a random ring, identified", DT_MIDDLE, FAM_FORTUNE, 3 },
	{ "a blessed scroll of enchant armor or weapon", DT_MIDDLE,
	  FAM_FORTUNE, 3 },
	{ "telepathy, but your Luck can never pass 8", DT_MIDDLE, FAM_BARGAIN,
	  3 },
	{ "teleport control, but also teleportitis", DT_MIDDLE, FAM_BARGAIN, 3 },

	{ "magic resistance", DT_LATE, FAM_BODY, 1 },
	{ "reflection", DT_LATE, FAM_BODY, 1 },
	{ "regeneration", DT_LATE, FAM_BODY, 2 },
	{ "free action", DT_LATE, FAM_BODY, 2 },
	{ "teleport control", DT_LATE, FAM_BODY, 2 },
	{ "+2 to a worn item", DT_LATE, FAM_CRAFT, 3 },
	{ "a level 4 or 5 spell", DT_LATE, FAM_CRAFT, 3 },
	{ "two more skill slots", DT_LATE, FAM_CRAFT, 3 },
	{ "identify your whole pack", DT_LATE, FAM_FORTUNE, 3 },
	{ "a blessed scroll of genocide", DT_LATE, FAM_FORTUNE, 3 },
	{ "a blessed scroll of charging", DT_LATE, FAM_FORTUNE, 3 },
	{ "a wish, but you can never pray again", DT_LATE, FAM_BARGAIN, 3 },
	{ "reflection, but -20 maximum hit points", DT_LATE, FAM_BARGAIN, 3 },
};

static const struct milestone_def {
	const char *name;
	xchar tier;
	boolean extra;		/* needs Fortune II */
} milestones[NUM_DMILESTONES] = {
	{ "the Oracle", DT_EARLY, FALSE },
	{ "Minetown", DT_EARLY, TRUE },
	{ "Sokoban finished", DT_EARLY, TRUE },
	{ "Mines' End", DT_EARLY, FALSE },
	{ "the Quest portal level", DT_MIDDLE, FALSE },
	{ "Medusa killed", DT_MIDDLE, TRUE },
	{ "the Castle", DT_LATE, FALSE },
	{ "the Valley of the Dead", DT_LATE, TRUE },
};

static const char *const family_names[NUM_FAMILIES] = {
	"Body", "Craft", "Fortune", "Bargain"
};
static const char *const act_names[] = { "", "I", "II", "III" };

STATIC_DCL boolean FDECL(fate_taken, (int));
STATIC_DCL boolean FDECL(fate_ok, (int));
STATIC_DCL int FDECL(pick_fate, (int, int));
STATIC_DCL int FDECL(make_offer, (int, int, int *));
STATIC_DCL void FDECL(run_draft, (int, const char *));
STATIC_DCL void FDECL(apply_fate, (int));
STATIC_DCL struct obj *FDECL(worn_armor, (BOOLEAN_P));
STATIC_DCL int FDECL(spell_book, (int, int, BOOLEAN_P));
STATIC_DCL int NDECL(best_school);
STATIC_DCL boolean FDECL(spell_known, (int));
STATIC_DCL boolean NDECL(any_unidentified);
STATIC_DCL boolean NDECL(casts_spells);
STATIC_DCL void FDECL(give_item, (int, BOOLEAN_P));

/* ------------------------------------------------------------------ */

/* does this game have fate drafts? */
boolean
drafts_on()
{
	return (boolean) (LEGACY_RULES() && !lprof.drafts_off
			  && !pact_rank(PACT_STARLESS));
}

/* the first time a milestone is reached: a draft at the end of the turn */
void
draft_milestone(m)
int m;
{
	if (m < 0 || m >= NUM_DMILESTONES || !drafts_on()
		|| (lrun.draft_seen & (1L << m)))
	    return;
	if (milestones[m].extra && lrun.lineage[LIN_FORTUNE] < 2)
	    return;
	lrun.draft_seen |= 1L << m;
	lrun.draft_pending |= 1L << m;
}

/* an Expedition act was cleared: one more draft at that act's tier */
void
draft_act(n)
int n;
{
	if (n >= 1 && n <= NUM_ACTS && drafts_on())
	    lrun.act_drafts |= 1 << (n - 1);
}

/* once per turn: offer what is waiting */
void
draft_turn()
{
	char buf[BUFSZ];
	int m, n;

	if (program_state.gameover || (!lrun.draft_pending && !lrun.act_drafts))
	    return;
	for (m = 0; m < NUM_DMILESTONES; m++)
	    if (lrun.draft_pending & (1L << m)) {
		lrun.draft_pending &= ~(1L << m);
		Sprintf(buf, "Fate draft: %s", milestones[m].name);
		run_draft(milestones[m].tier, buf);
	    }
	for (n = 1; n <= NUM_ACTS; n++)
	    if (lrun.act_drafts & (1 << (n - 1))) {
		lrun.act_drafts &= ~(1 << (n - 1));
		Sprintf(buf, "Fate draft: Act %s cleared", act_names[n]);
		run_draft(n - 1, buf);
	    }
}

/* the fates taken, for the help line and the recap */
void
draft_fate_text(buf, maxlen)
char *buf;
int maxlen;
{
	int i;

	*buf = '\0';
	for (i = 0; i < lrun.nfates; i++) {
	    const char *nm = fates[(int) lrun.fate[i]].name;

	    if ((int) (strlen(buf) + strlen(nm) + 8) >= maxlen) {
		Strcat(buf, ", ...");
		break;
	    }
	    Sprintf(eos(buf), "%s%s", *buf ? ", " : "", nm);
	}
}

/* ------------------------------------------------------------------ */
/* building a draft */

STATIC_OVL boolean
fate_taken(f)
int f;
{
	int i;

	for (i = 0; i < lrun.nfates; i++)
	    if (lrun.fate[i] == f)
		return TRUE;
	return FALSE;
}

/* may this fate be offered now?  never one the character already has */
STATIC_OVL boolean
fate_ok(f)
int f;
{
	if (fate_taken(f))
	    return FALSE;
	switch (f) {
	case FT_POISON:		return (boolean) !Poison_resistance;
	case FT_STEALTH:	return (boolean) !Stealth;
	case FT_INFRA:		return (boolean) !Infravision;
	case FT_SPEED:
	case FT_SPEEDLUCK:	return (boolean) !Fast;
	case FT_SLEEP:		return (boolean) !Sleep_resistance;
	case FT_FIRE:		return (boolean) !Fire_resistance;
	case FT_COLD:		return (boolean) !Cold_resistance;
	case FT_SHOCK:		return (boolean) !Shock_resistance;
	case FT_SEEINV:		return (boolean) !See_invisible;
	case FT_MAGICRES:	return (boolean) !Antimagic;
	case FT_REFLECT:	return (boolean) !Reflecting;
	case FT_REFLECTHP:	return (boolean) (!Reflecting && u.uhpmax > 30);
	case FT_REGEN:		return (boolean) !Regeneration;
	case FT_FREEACT:	return (boolean) !Free_action;
	case FT_TCONTROL:
	case FT_TPORTITIS:	return (boolean) !Teleport_control;
	case FT_TELEPATHY:	return (boolean) !HTelepat;
	case FT_STRENGTH:	return (boolean) (ABASE(A_STR) < STR18(100));
	case FT_ARMOR1:		return (boolean) (worn_armor(FALSE) != 0);
	case FT_ERODEPROOF:	return (boolean) (worn_armor(TRUE) != 0);
	case FT_WEAPON1:
	    return (boolean) (uwep && (uwep->oclass == WEAPON_CLASS
				       || is_weptool(uwep)));
	case FT_ITEM2:
	    return (boolean) (worn_armor(FALSE) != 0 || (uwep
			      && (uwep->oclass == WEAPON_CLASS
				  || is_weptool(uwep))));
	case FT_SPELL1:		return (boolean) (spell_book(1, 1, TRUE) != 0);
	case FT_SPELL23:	return (boolean) (spell_book(2, 3, TRUE) != 0);
	case FT_SPELL45:	return (boolean) (spell_book(4, 5, TRUE) != 0);
	case FT_ID2:
	case FT_ID4:
	case FT_IDALL:		return any_unidentified();
	case FT_WISH:		return (boolean) !lrun.fate_nopray;
	}
	return TRUE;
}

/* one fate of a family and tier, weighted; spells lean to casters */
STATIC_OVL int
pick_fate(tier, fam)
int tier, fam;
{
	int f, w, total = 0;

	for (f = 0; f < NUM_FATES; f++)
	    if (fates[f].tier == tier && fates[f].family == fam && fate_ok(f))
		total += (f == FT_SPELL1 || f == FT_SPELL23 || f == FT_SPELL45)
			 && !casts_spells() ? 1 : fates[f].weight;
	if (!total)
	    return -1;
	total = rn2(total);
	for (f = 0; f < NUM_FATES; f++)
	    if (fates[f].tier == tier && fates[f].family == fam && fate_ok(f)) {
		w = (f == FT_SPELL1 || f == FT_SPELL23 || f == FT_SPELL45)
		    && !casts_spells() ? 1 : fates[f].weight;
		if ((total -= w) < 0)
		    return f;
	    }
	return -1;
}

/* up to n fates, each from a different family */
STATIC_OVL int
make_offer(tier, n, offer)
int tier, n;
int *offer;
{
	int fam[NUM_FAMILIES], i, j, t, got = 0, f;

	for (i = 0; i < NUM_FAMILIES; i++)
	    fam[i] = i;
	for (i = NUM_FAMILIES - 1; i > 0; i--) {	/* shuffle */
	    j = rn2(i + 1);
	    t = fam[i], fam[i] = fam[j], fam[j] = t;
	}
	for (i = 0; i < NUM_FAMILIES && got < n; i++)
	    if ((f = pick_fate(tier, fam[i])) >= 0)
		offer[got++] = f;
	return got;
}

STATIC_OVL void
run_draft(tier, title)
int tier;
const char *title;
{
	winid win;
	anything any;
	menu_item *pick = 0;
	char buf[BUFSZ];
	int offer[NUM_FAMILIES], nchoice, n, i, choice, fortune;
	boolean rerolled = FALSE;

	fortune = lrun.lineage[LIN_FORTUNE];
	nchoice = (fortune >= 1) ? 3 : 2;
	n = make_offer(tier, nchoice, offer);
	if (!n)
	    return;
	for (;;) {
	    win = create_nhwindow(NHW_MENU);
	    start_menu(win);
	    any.a_void = 0;
	    add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE,
		     "Fate offers you a choice. Keep one.", MENU_UNSELECTED);
	    add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE, "", MENU_UNSELECTED);
	    for (i = 0; i < n; i++) {
		Sprintf(buf, "%s: %s", family_names[(int) fates[offer[i]].family],
			fates[offer[i]].name);
		any.a_int = 1 + i;
		add_menu(win, NO_GLYPH, &any, 'a' + i, 0, ATR_NONE, buf,
			 MENU_UNSELECTED);
	    }
	    any.a_void = 0;
	    add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE, "", MENU_UNSELECTED);
	    if (fortune >= 3 && !rerolled) {
		any.a_int = 100;
		add_menu(win, NO_GLYPH, &any, 'r', 0, ATR_NONE,
			 "Reroll these fates (once)", MENU_UNSELECTED);
	    }
	    Sprintf(buf, "Skip the draft for %d Renown", SKIP_RENOWN);
	    any.a_int = 101;
	    add_menu(win, NO_GLYPH, &any, 's', 0, ATR_NONE, buf,
		     MENU_UNSELECTED);
	    end_menu(win, title);
	    i = select_menu(win, PICK_ONE, &pick);
	    destroy_nhwindow(win);
	    choice = (i > 0) ? pick[0].item.a_int : 0;
	    if (pick)
		free((genericptr_t) pick), pick = 0;
	    if (choice == 100) {
		rerolled = TRUE;
		if ((i = make_offer(tier, nchoice, offer)) > 0)
		    n = i;
		continue;
	    }
	    if (choice == 101 || (!choice && (program_state.done_hup
		    || yn("Skip this draft?") == 'y'))) {
		lrun.draft_skips++;
		You("let fate pass, for %d Renown.", SKIP_RENOWN);
		return;
	    }
	    if (choice >= 1 && choice <= n)
		break;
	}
	apply_fate(offer[choice - 1]);
}

/* ------------------------------------------------------------------ */
/* the fates themselves */

/* a random piece of worn armor; 'need_proof': only one that can still
   rust, burn or rot */
STATIC_OVL struct obj *
worn_armor(need_proof)
boolean need_proof;
{
	struct obj *list[7], *o;
	int n = 0, i;

	list[0] = uarm, list[1] = uarmc, list[2] = uarmh, list[3] = uarms;
	list[4] = uarmg, list[5] = uarmf;
#ifdef TOURIST
	list[6] = uarmu;
#else
	list[6] = 0;
#endif
	for (i = 0; i < 7; i++)
	    if ((o = list[i]) != 0 && (!need_proof || !o->oerodeproof))
		list[n++] = o;
	return n ? list[rn2(n)] : (struct obj *) 0;
}

STATIC_OVL boolean
spell_known(otyp)
int otyp;
{
	int i;

	for (i = 0; i < MAXSPELL && spl_book[i].sp_id != NO_SPELL; i++)
	    if (spl_book[i].sp_id == otyp)
		return TRUE;
	return FALSE;
}

/* the spell school this role can go furthest in */
STATIC_OVL int
best_school()
{
	int sk, best = P_FIRST_SPELL;

	for (sk = P_FIRST_SPELL; sk <= P_LAST_SPELL; sk++)
	    if (P_MAX_SKILL(sk) > P_MAX_SKILL(best))
		best = sk;
	return best;
}

STATIC_OVL boolean
casts_spells()
{
	return (boolean) (P_MAX_SKILL(best_school()) >= P_SKILLED);
}

/* a spellbook of level lo..hi not known yet, from the best school if it
   has one, else any; 'check': only say whether one exists */
STATIC_OVL int
spell_book(lo, hi, check)
int lo, hi;
boolean check;
{
	int otyp, pass, n, school = best_school();

	for (pass = 0; pass < 2; pass++) {
	    n = 0;
	    for (otyp = bases[SPBOOK_CLASS];
		    otyp < NUM_OBJECTS && objects[otyp].oc_class == SPBOOK_CLASS;
		    otyp++)
		if (otyp != SPE_BLANK_PAPER && otyp != SPE_BOOK_OF_THE_DEAD
			&& objects[otyp].oc_prob > 0
			&& objects[otyp].oc_level >= lo
			&& objects[otyp].oc_level <= hi
			&& (pass || objects[otyp].oc_skill == school)
			&& !spell_known(otyp))
		    n++;
	    if (!n)
		continue;
	    if (check)
		return 1;
	    n = rn2(n);
	    for (otyp = bases[SPBOOK_CLASS];
		    otyp < NUM_OBJECTS && objects[otyp].oc_class == SPBOOK_CLASS;
		    otyp++)
		if (otyp != SPE_BLANK_PAPER && otyp != SPE_BOOK_OF_THE_DEAD
			&& objects[otyp].oc_prob > 0
			&& objects[otyp].oc_level >= lo
			&& objects[otyp].oc_level <= hi
			&& (pass || objects[otyp].oc_skill == school)
			&& !spell_known(otyp) && !n--)
		    return otyp;
	}
	return 0;
}

STATIC_OVL boolean
any_unidentified()
{
	struct obj *o;

	for (o = invent; o; o = o->nobj)
	    if (not_fully_identified(o))
		return TRUE;
	return FALSE;
}

/* an item, made without the Hexed pact touching it */
STATIC_OVL void
give_item(otyp, bless)
int otyp;
boolean bless;
{
	boolean was = lrun.inv_made;
	struct obj *obj;

	lrun.inv_made = FALSE;
	obj = legacy_item(otyp, 1, 0, bless ? 1 : 0);
	lrun.inv_made = was;
	prinv((char *) 0, obj, 0L);
}

STATIC_OVL void
apply_fate(f)
int f;
{
	struct obj *obj, dummy;
	boolean was;
	int otyp;

	if (lrun.nfates < MAX_FATES)
	    lrun.fate[lrun.nfates++] = (xchar) f;
	pline("Fate grants you %s.", fates[f].name);
	switch (f) {
	case FT_POISON:
	    HPoison_resistance |= FROMOUTSIDE;
	    You_feel("healthy.");
	    break;
	case FT_HP5:
	    u.uhpmax += 5;
	    u.uhp += 5;
	    break;
	case FT_STEALTH:
	    HStealth |= FROMOUTSIDE;
	    You_feel("stealthy!");
	    break;
	case FT_INFRA:
	    HInfravision |= FROMOUTSIDE;
	    see_monsters();
	    break;
	case FT_SPEED:
	    HFast |= FROMOUTSIDE;
	    You_feel("quick!");
	    lrun.fleet_until = 0;
	    break;
	case FT_SPEEDLUCK:
	    HFast |= FROMOUTSIDE;
	    You_feel("quick, and a little unlucky.");
	    lrun.fleet_until = 0;
	    lrun.fate_luck--;
	    change_luck(-1);
	    break;
	case FT_SLEEP:
	    HSleep_resistance |= FROMOUTSIDE;
	    You_feel("wide awake.");
	    break;
	case FT_FIRE:
	    HFire_resistance |= FROMOUTSIDE;
	    You_feel("a momentary chill.");
	    break;
	case FT_COLD:
	    HCold_resistance |= FROMOUTSIDE;
	    You_feel("full of hot air.");
	    break;
	case FT_SHOCK:
	    HShock_resistance |= FROMOUTSIDE;
	    Your("health currently feels amplified!");
	    break;
	case FT_SEEINV:
	    HSee_invisible |= FROMOUTSIDE;
	    set_mimic_blocking();
	    see_monsters();
	    You("can see invisible things.");
	    break;
	case FT_MAGICRES:
	    u.uprops[ANTIMAGIC].extrinsic |= W_FATE;
	    You_feel("magic slide off you.");
	    break;
	case FT_REFLECT:
	case FT_REFLECTHP:
	    u.uprops[REFLECTING].extrinsic |= W_FATE;
	    You_feel("a mirror-bright shell around you.");
	    if (f == FT_REFLECTHP) {
		u.uhpmax = max(1, u.uhpmax - 20);
		if (u.uhp > u.uhpmax)
		    u.uhp = u.uhpmax;
	    }
	    break;
	case FT_REGEN:
	    HRegeneration |= FROMOUTSIDE;
	    You_feel("your wounds knit faster.");
	    break;
	case FT_FREEACT:
	    u.uprops[FREE_ACTION].extrinsic |= W_FATE;
	    You_feel("nothing can hold you.");
	    break;
	case FT_TCONTROL:
	    HTeleport_control |= FROMOUTSIDE;
	    You_feel("in control of yourself.");
	    break;
	case FT_TPORTITIS:
	    HTeleport_control |= FROMOUTSIDE;
	    HTeleportation |= FROMOUTSIDE;
	    You_feel("in control, and very jumpy.");
	    break;
	case FT_TELEPATHY:
	    HTelepat |= FROMOUTSIDE;
	    You_feel("a strange mental acuity.");
	    lrun.fate_luckcap = TRUE;
	    change_luck(0);	/* applies the cap */
	    break;
	case FT_STRENGTH:
	    (void) adjattrib(A_STR, 2, FALSE);
	    lrun.fate_hunger = TRUE;
	    break;
	case FT_ARMOR1:
	case FT_WEAPON1:
	case FT_ITEM2:
	    obj = (f == FT_WEAPON1) ? uwep : worn_armor(FALSE);
	    if (f == FT_ITEM2 && uwep && (!obj || !rn2(2))
		    && (uwep->oclass == WEAPON_CLASS || is_weptool(uwep)))
		obj = uwep;
	    if (obj) {
		obj->spe += (f == FT_ITEM2) ? 2 : 1;
		Your("%s %s for a moment.", xname(obj),
		     otense(obj, "glow"));
		if (obj->oclass == ARMOR_CLASS)
		    find_ac();
		prinv((char *) 0, obj, 0L);
	    }
	    break;
	case FT_ERODEPROOF:
	    for (obj = invent; obj; obj = obj->nobj)
		if ((obj->owornmask & W_ARMOR) && obj->oclass == ARMOR_CLASS) {
		    obj->oerodeproof = 1;
		    obj->oeroded = obj->oeroded2 = 0;
		    obj->rknown = 1;
		}
	    Your("armor feels sturdier.");
	    break;
	case FT_SPELL1:
	case FT_SPELL23:
	case FT_SPELL45:
	    otyp = (f == FT_SPELL1) ? spell_book(1, 1, FALSE)
		   : (f == FT_SPELL23) ? spell_book(2, 3, FALSE)
		   : spell_book(4, 5, FALSE);
	    if (otyp) {
		dummy.otyp = otyp;
		initialspell(&dummy);
		You("know the spell %s.", OBJ_NAME(objects[otyp]));
	    }
	    break;
	case FT_SLOT1:
	case FT_SLOT1M:
	    u.weapon_slots++;
	    You("feel more skillful.");
	    break;
	case FT_SLOT2:
	    u.weapon_slots += 2;
	    You("feel more skillful.");
	    break;
	case FT_ID2:
	    identify_pack(2);
	    break;
	case FT_ID4:
	    identify_pack(4);
	    break;
	case FT_IDALL:
	    identify_pack(0);
	    break;
	case FT_HEAL2:
	    was = lrun.inv_made;
	    lrun.inv_made = FALSE;
	    obj = legacy_item(POT_HEALING, 2, 0, 0);
	    lrun.inv_made = was;
	    prinv((char *) 0, obj, 0L);
	    break;
	case FT_REMCURSE:
	    give_item(SCR_REMOVE_CURSE, TRUE);
	    break;
	case FT_GOLD:
	    u.ugold += 300;
	    flags.botl = 1;
	    break;
	case FT_RING:
	    was = lrun.inv_made;
	    lrun.inv_made = FALSE;
	    obj = mkobj(RING_CLASS, TRUE);
	    lrun.inv_made = was;
	    fully_identify_obj(obj);
	    obj = addinv(obj);
	    prinv((char *) 0, obj, 0L);
	    break;
	case FT_ENCHANT:
	    give_item((uwep && (uwep->oclass == WEAPON_CLASS
				|| is_weptool(uwep))
		       && (!worn_armor(FALSE) || !rn2(2)))
		      ? SCR_ENCHANT_WEAPON : SCR_ENCHANT_ARMOR, TRUE);
	    break;
	case FT_GENOCIDE:
	    give_item(SCR_GENOCIDE, TRUE);
	    break;
	case FT_CHARGING:
	    give_item(SCR_CHARGING, TRUE);
	    break;
	case FT_WISH:
	    lrun.fate_nopray = TRUE;
	    pline("Your gods turn away from you, forever.");
	    makewish();
	    break;
	}
	flags.botl = 1;
	legacy_write(FALSE);
}

/*draft.c*/
