/*	SCCS Id: @(#)fate.c	0.0.7E7F3	2026/10/13	*/
/* NetHack may be freely redistributed.  See license for details. */

/*
 * Slash'EM Legacy: run choices.  Blessings drawn at the start of a game
 * (the Favor track), vows declared before it, role mastery with its
 * alternate kits, and the heirloom vault (the Inheritance track).
 * Everything here applies to Legacy games only; Classic is untouched.
 */

#include "hack.h"

/* ------------------------------------------------------------------ */
/* the tables */

static const struct bless_def {
	const char *key, *name, *desc;
	short cost;		/* Renown; 0: in the pool from the start */
} blessings[NUM_BLESSINGS] = {
	{ "hearty", "Hearty meal", "start with 1,500 nutrition instead of 900",
	  0 },
	{ "lucky", "Lucky start", "Luck starts 3 higher and fades as usual",
	  0 },
	{ "cartographer", "Cartographer",
	  "dungeon levels 1 and 2 start mapped", 0 },
	{ "strongback", "Strong back", "carrying capacity +25%", 0 },
	{ "purse", "Gold purse", "300 extra gold", 0 },
	{ "fleet", "Fleet", "intrinsic speed for the first 1,500 turns", 0 },
	{ "keeneye", "Keen eye", "automatic searching for the first 2,000 turns",
	  60 },
	{ "scholar", "Scholar", "one extra scroll type known", 60 },
	{ "petbond", "Pet bond", "your pet starts one level higher", 60 },
	{ "packrat", "Pack rat", "a sack", 60 },
	{ "torch", "Torchbearer", "an oil lamp", 60 },
	{ "herbalist", "Herbalist",
	  "two sprigs of wolfsbane and two cloves of garlic", 60 },
	{ "lightstep", "Light step", "intrinsic stealth", 80 },
	{ "holywater", "Holy water", "two potions of holy water", 80 },
	{ "nighteyes", "Night eyes", "infravision, if your race lacks it", 80 },
	{ "merchant", "Merchant's friend",
	  "shops price you as if your Charisma were 18", 80 },
	{ "devout", "Devout", "alignment record starts at 10", 80 },
	{ "tinner", "Tinner", "a tinning kit", 80 },
	{ "poisonward", "Poison ward",
	  "poison resistance for the first 1,000 turns", 100 },
	{ "removecurse", "Remove curse",
	  "a blessed scroll of remove curse, identified", 100 },
	{ "stoneward", "Stoneward",
	  "a lizard corpse, which never rots and cures stoning", 100 },
	{ "sparewand", "Spare wand",
	  "a random wand with three charges, never wishing", 120 },
	{ "trained", "Trained hand", "one extra skill slot", 120 },
	{ "heirsluck", "Heir's luck",
	  "every draw offers one more blessing", -1 },
};

static const struct vow_def {
	const char *key, *name, *never;
	short bonus;		/* percent */
	boolean hard;		/* opens with "Keep a vow through Act I" */
} vows[NUM_VOWS] = {
	{ "vegetarian", "Vegetarian", "eat meat", 15, FALSE },
	{ "atheist", "Atheist", "pray, sacrifice or use an altar", 20, FALSE },
	{ "illiterate", "Illiterate", "read anything but fortune cookies", 15,
	  FALSE },
	{ "wishless", "Wishless", "make a wish", 15, FALSE },
	{ "polyselfless", "Polyselfless", "polymorph yourself", 10, FALSE },
	{ "celibate", "Celibate", "accept a foocubus's offer", 5, FALSE },
	{ "artiwishless", "Artifact-wishless", "wish for an artifact", 5,
	  FALSE },
	{ "polypileless", "Polypileless", "polymorph objects", 5, FALSE },
	{ "vegan", "Vegan", "eat any animal product", 25, TRUE },
	{ "weaponless", "Weaponless", "hit with a wielded weapon", 25, TRUE },
	{ "foodless", "Foodless", "eat anything at all", 40, TRUE },
	{ "pacifist", "Pacifist", "kill a monster yourself", 40, TRUE },
};

/* the alternate kits, by role */
static const struct kit_def {
	const char *role;	/* roles[].name.m */
	const char *name[3];	/* kits A, B and C */
	const char *desc[3];
} kits[] = {
	{ "Archeologist", { "Explorer", "Surveyor", "Duelist" },
	  { "+2 bullwhip, pick-axe, tinning kit, touchstone",
	    "two scrolls of magic mapping instead of the tinning kit and touchstone",
	    "a rapier instead of the bullwhip, no tinning kit" } },
	{ "Barbarian", { "Warlord", "Reaver", "Pit fighter" },
	  { "a two-handed weapon and a spare",
	    "a +1 axe and a large shield instead of the two-handed weapon",
	    "no weapons; +2 gloves and an extra food ration" } },
	{ "Caveman", { "Hunter", "Spearman", "Bowyer" },
	  { "+1 club, +2 sling, flint stones",
	    "a +1 spear instead of the club",
	    "a bow and 30 arrows instead of the sling and flint" } },
	{ "Flame Mage", { "Flamecaller", "Ember blade", "Lamplighter" },
	  { "+1 quarterstaff, wand of fire, fireball",
	    "three daggers instead of the quarterstaff",
	    "an oil lamp and two potions of oil instead of the ring" } },
	{ "Healer", { "Physician", "Field surgeon", "Apothecary" },
	  { "scalpel, stethoscope, healing potions, wand of sleep, gold",
	    "a +1 quarterstaff instead of the scalpel, half the gold",
	    "four more healing potions instead of the wand of sleep" } },
	{ "Ice Mage", { "Frostcaller", "Ice blade", "Snow warden" },
	  { "+1 quarterstaff, wand of cold, cone of cold",
	    "three daggers instead of the quarterstaff",
	    "an elven cloak instead of the ring" } },
	{ "Knight", { "Knight-errant", "Squire", "Jouster" },
	  { "long sword, lance, plate mail, pony",
	    "ring mail instead of plate mail, plus two food rations",
	    "a +1 lance and riding one step better, no apples or carrots" } },
	{ "Monk", { "Disciple", "Wanderer", "Pilgrim" },
	  { "martial arts, robe, one random spellbook",
	    "a +1 leather cloak instead of the robe",
	    "four blessed waters instead of the oranges and fortune cookies" } },
	{ "Necromancer", { "Deathspeaker", "Gravedigger", "Poisoner" },
	  { "ghoul pet, wand of draining, pick-axe",
	    "a second ghoul instead of the wand of draining",
	    "six daggers instead of the quarterstaff and one ring" } },
	{ "Priest", { "Acolyte", "Templar", "Missionary" },
	  { "blessed mace, +2 small shield, holy water, two spellbooks",
	    "a +1 flail instead of the mace, two fewer holy waters",
	    "four more holy waters instead of the random spellbook" } },
	{ "Ranger", { "Archer", "Skirmisher", "Crossbowman" },
	  { "+1 bow, arrows, cloak of displacement",
	    "a +1 sling and 40 flint stones instead of the bow, plus two cram rations",
	    "a crossbow and 40 bolts instead of the bow and arrows" } },
	{ "Rogue", { "Cutpurse", "Gunslinger", "Burglar" },
	  { "short sword, daggers, darts, teleport scrolls",
	    "the pistol kit every time, instead of a 30% chance",
	    "two scrolls of magic mapping instead of the darts" } },
	{ "Samurai", { "Ronin", "Kensai", "Archer" },
	  { "katana, short sword, bow and arrows, splint mail",
	    "a +1 katana, no bow or arrows",
	    "a +1 bow and 60 arrows, no short sword" } },
	{ "Tourist", { "Sightseer", "Backpacker", "Photographer" },
	  { "+2 darts, magic mapping, camera, lots of gold",
	    "half the gold; a +1 dagger and two more extra healing potions",
	    "a camera with double charges, half the darts" } },
	{ "Undead Slayer", { "Hunter", "Crossbow", "Gunslinger" },
	  { "one of four kits at random",
	    "always the crossbow kit",
	    "always the pistol kit" } },
	{ "Valkyrie", { "Shieldmaiden", "Axe-maiden", "Huntress" },
	  { "+1 spear, five daggers, +3 small shield",
	    "a +1 axe instead of the spear, shield +1 instead of +3",
	    "no shield; eight +2 daggers instead of five" } },
	{ "Wizard", { "Apprentice", "Battle mage", "Scholar" },
	  { "+1 quarterstaff, cloak of magic resistance, random wand",
	    "three daggers instead of the quarterstaff",
	    "one extra spellbook instead of the wand" } },
	{ "Yeoman", { "Warder", "Bowman", "Guardsman" },
	  { "+2 short sword, +1 partisan, pony",
	    "a bow and 30 arrows instead of the partisan",
	    "a +2 partisan and riding one step better, no fishing pole" } },
	{ 0, { 0, 0, 0 }, { 0, 0, 0 } }
};

/* what each mastery level gives */
static const char *const mastery_reward[MASTERY_MAX + 1] = {
	"",
	"alternate kit B",
	"one weapon skill of your choice starts at Basic",
	"alternate kit C",
	"a vault slot only this role can use",
	"a gold name in the Hall",
};
static const char *const mastery_reach[MASTERY_MAX + 1] = {
	"", "dungeon level 5", "dungeon level 10", "the Quest portal level",
	"kill Medusa", "complete the Quest"
};

STATIC_DCL void FDECL(add_text, (winid, const char *));
STATIC_DCL void FDECL(add_choice, (winid, int, CHAR_P, const char *,
				   BOOLEAN_P));
STATIC_DCL const struct kit_def *FDECL(find_kit, (int));
STATIC_DCL boolean FDECL(kit_ok, (int, int));
STATIC_DCL long FDECL(vow_count, (int));
STATIC_DCL boolean FDECL(vow_open, (int));
STATIC_DCL boolean FDECL(bless_useless, (int));
STATIC_DCL void FDECL(bless_apply, (int));
STATIC_DCL struct monst *NDECL(first_pet);
STATIC_DCL struct obj *FDECL(kit_find, (int));
STATIC_DCL struct obj *FDECL(kit_find_skill, (int, BOOLEAN_P));
STATIC_DCL void FDECL(kit_drop, (struct obj *));
STATIC_DCL void FDECL(kit_drop_all, (int));
STATIC_DCL struct obj *FDECL(kit_give, (int, int, int, int));
STATIC_DCL void FDECL(kit_skill, (struct obj *));
STATIC_DCL void FDECL(kit_wield, (struct obj *));
STATIC_DCL void FDECL(kit_wear, (struct obj *));
STATIC_DCL void FDECL(kit_quiver, (struct obj *));
STATIC_DCL void NDECL(kit_ride);
STATIC_DCL void FDECL(kit_halve_gold, (int));
STATIC_DCL void NDECL(mastery_skill);
STATIC_DCL boolean FDECL(heir_ok, (struct obj *));
STATIC_DCL int NDECL(vault_slots);
STATIC_DCL int FDECL(vault_used, (int));
STATIC_DCL void FDECL(heir_name, (struct legacy_heirloom *, char *));
STATIC_DCL void FDECL(vault_remove, (int));
STATIC_DCL void FDECL(append_part, (char *, const char *, int));

STATIC_OVL void
add_text(win, str)
winid win;
const char *str;
{
	anything any;

	any.a_void = 0;
	add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE, str, MENU_UNSELECTED);
}

STATIC_OVL void
add_choice(win, val, accel, str, sel)
winid win;
int val;
char accel;
const char *str;
boolean sel;
{
	anything any;

	any.a_void = 0;
	any.a_int = val;
	add_menu(win, NO_GLYPH, &any, accel, 0, ATR_NONE, str,
		 sel ? MENU_SELECTED : MENU_UNSELECTED);
}

/* ------------------------------------------------------------------ */
/* the profile */

boolean
fate_read(key, rest)
const char *key;
char *rest;
{
	int i;

	if (!strcmp(key, "blessing")) {
	    for (i = 0; i < NUM_BLESSINGS; i++)
		if (!strcmp(rest, blessings[i].key))
		    lprof.bless_owned |= 1UL << i;
	    return TRUE;
	}
	if (!strcmp(key, "vow")) {
	    for (i = 0; i < NUM_VOWS; i++)
		if (!strcmp(rest, vows[i].key))
		    lprof.vows |= 1 << i;
	    return TRUE;
	}
	if (!strcmp(key, "mastery") || !strcmp(key, "kit")
		|| !strcmp(key, "mskill")) {
	    /* "mastery 3 Valkyrie", "kit 1 Valkyrie", "mskill axe\tValkyrie" */
	    char *role, *p;
	    int n = 0;

	    if (key[1] == 's') {
		if (!(p = index(rest, '\t')))
		    return TRUE;
		*p = '\0';
		role = p + 1;
	    } else {
		n = atoi(rest);
		if (!(role = index(rest, ' ')))
		    return TRUE;
		role++;
	    }
	    for (i = 0; roles[i].name.m && i < MAX_ROLES; i++)
		if (!strcmp(role, roles[i].name.m)) {
		    if (key[0] == 'm' && key[1] == 'a')
			lprof.mastery[i] = (xchar) max(0, min(n, MASTERY_MAX));
		    else if (key[0] == 'k')
			lprof.kit[i] = (xchar) max(0, min(n, KIT_C2));
		    else {
			int sk;

			for (sk = P_FIRST_WEAPON; sk <= P_LAST_WEAPON; sk++)
			    if (!strcmp(legacy_skill_name(sk), rest))
				lprof.mskill[i] = sk;
		    }
		}
	    return TRUE;
	}
	if (!strcmp(key, "heirloom")) {
	    /* heirloom owner<TAB>class<TAB>name<TAB>spe<TAB>quan<TAB>
	       erodeproof<TAB>runs<TAB>role */
	    struct legacy_heirloom *h;
	    char *p = rest, *t, *f[8];
	    int nf, oc;

	    for (nf = 0; nf < 8 && p; nf++) {
		f[nf] = p;
		if ((t = index(p, '\t')) != 0) {
		    *t = '\0';
		    p = t + 1;
		} else
		    p = 0;
	    }
	    if (nf < 8 || lprof.nvault >= MAX_VAULT)
		return TRUE;
	    h = &lprof.vault[lprof.nvault];
	    (void) memset((genericptr_t) h, 0, sizeof *h);
	    oc = atoi(f[1]);
	    for (i = 0; i < NUM_OBJECTS; i++)
		if (objects[i].oc_class == oc && OBJ_NAME(objects[i])
			&& !strcmp(OBJ_NAME(objects[i]), f[2]))
		    break;
	    if (i >= NUM_OBJECTS)
		return TRUE;
	    legacy_clean_field(h->owner, f[0], PL_NSIZ);
	    h->otyp = i;
	    h->spe = (schar) atoi(f[3]);
	    h->quan = (short) max(1, atoi(f[4]));
	    h->erodeproof = !!atoi(f[5]);
	    h->runs = (short) atoi(f[6]);
	    h->role = -1;
	    for (i = 0; roles[i].name.m && i < MAX_ROLES; i++)
		if (!strcmp(f[7], roles[i].name.m))
		    h->role = i;
	    lprof.nvault++;
	    return TRUE;
	}
	if (!strcmp(key, "heirpick")) {
	    lprof.heir_pick = atoi(rest);
	    return TRUE;
	}
	return FALSE;
}

void
fate_write(fp)
FILE *fp;
{
	struct legacy_heirloom *h;
	int i;

	for (i = 0; i < NUM_BLESSINGS; i++)
	    if (lprof.bless_owned & (1UL << i))
		fprintf(fp, "blessing %s\n", blessings[i].key);
	for (i = 0; i < NUM_VOWS; i++)
	    if (lprof.vows & (1 << i))
		fprintf(fp, "vow %s\n", vows[i].key);
	for (i = 0; roles[i].name.m && i < MAX_ROLES; i++) {
	    if (lprof.mastery[i])
		fprintf(fp, "mastery %d %s\n", lprof.mastery[i],
			roles[i].name.m);
	    if (lprof.kit[i])
		fprintf(fp, "kit %d %s\n", lprof.kit[i], roles[i].name.m);
	    if (lprof.mskill[i])
		fprintf(fp, "mskill %s\t%s\n",
			legacy_skill_name(lprof.mskill[i]), roles[i].name.m);
	}
	for (i = 0; i < lprof.nvault; i++) {
	    h = &lprof.vault[i];
	    fprintf(fp, "heirloom %s\t%d\t%s\t%d\t%d\t%d\t%d\t%s\n", h->owner,
		    (int) objects[h->otyp].oc_class, OBJ_NAME(objects[h->otyp]),
		    (int) h->spe, (int) h->quan, h->erodeproof ? 1 : 0,
		    (int) h->runs, h->role >= 0 ? roles[h->role].name.m : "-");
	}
	if (lprof.heir_pick)
	    fprintf(fp, "heirpick %d\n", lprof.heir_pick);
}

/* ------------------------------------------------------------------ */
/* blessings */

const char *
fate_bless_name(b)
int b;
{
	return blessings[b].name;
}

boolean
fate_bless_owned(b)
int b;
{
	if (b == BL_HEIRSLUCK)
	    return (boolean) (lprof.feat_done[F_BELONGINGS] != 0);
	return (boolean) (!blessings[b].cost
			  || (lprof.bless_owned & (1UL << b)) != 0);
}

/* does this game have blessing b? */
boolean
fate_blessed(b)
int b;
{
	return (boolean) (LEGACY_RULES()
			  && (lrun.blessings & (1L << b)) != 0);
}

void
fate_bless_shop()
{
	winid win;
	menu_item *pick = 0;
	char buf[BUFSZ], qbuf[QBUFSZ];
	int b, n, owned;

	for (;;) {
	    for (owned = 0, b = 0; b < NUM_BLESSINGS; b++)
		if (b != BL_HEIRSLUCK && fate_bless_owned(b))
		    owned++;
	    win = create_nhwindow(NHW_MENU);
	    start_menu(win);
	    Sprintf(buf, "You have %ld Renown to spend.", legacy_renown());
	    add_text(win, buf);
	    add_text(win, "Blessings are small boons drawn at the start of a");
	    add_text(win, "Legacy game. The Favor track decides how many are");
	    Sprintf(buf, "drawn and kept. Your pool holds %d of %d.", owned,
		    NUM_BLESSINGS - 1);
	    add_text(win, buf);
	    if (!lprof.lineage[LIN_FAVOR])
		add_text(win, "You need Favor I (Lineage) before any are drawn.");
	    add_text(win, "");
	    for (b = 0; b < NUM_BLESSINGS; b++) {
		if (fate_bless_owned(b)) {
		    Sprintf(buf, "[x] %s: %s", blessings[b].name,
			    blessings[b].desc);
		    add_text(win, buf);
		} else if (blessings[b].cost < 0) {
		    Sprintf(buf, "[ ] %s: %s [feat: %s]", blessings[b].name,
			    blessings[b].desc,
			    legacy_feat_name(F_BELONGINGS));
		    add_text(win, buf);
		} else {
		    Sprintf(buf, "[ ] %s for %d: %s", blessings[b].name,
			    blessings[b].cost, blessings[b].desc);
		    if (legacy_renown() < blessings[b].cost) {
			Sprintf(eos(buf), " [need %ld more]",
				blessings[b].cost - legacy_renown());
			add_text(win, buf);
		    } else
			add_choice(win, b + 1, 0, buf, FALSE);
		}
	    }
	    end_menu(win, "Blessings");
	    n = select_menu(win, PICK_ONE, &pick);
	    destroy_nhwindow(win);
	    if (n <= 0)
		return;
	    b = pick[0].item.a_int - 1;
	    free((genericptr_t) pick), pick = 0;
	    Sprintf(qbuf, "Spend %d Renown on %s?", blessings[b].cost,
		    blessings[b].name);
	    if (yn(qbuf) == 'y' && legacy_renown() >= blessings[b].cost) {
		lprof.renown_spent += blessings[b].cost;
		lprof.bless_owned |= 1UL << b;
		lprof.dirty = TRUE;
		legacy_write(FALSE);
		pline("%s joins your pool of blessings.", blessings[b].name);
	    }
	}
}

STATIC_OVL struct monst *
first_pet()
{
	struct monst *mtmp;

	for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
	    if (mtmp->m_id == lrun.first_pet && !DEADMONSTER(mtmp))
		return mtmp;
	return 0;
}

/* a blessing that would do nothing for this character isn't drawn */
STATIC_OVL boolean
bless_useless(b)
int b;
{
	switch (b) {
	case BL_HEIRSLUCK:
	    return TRUE;
	case BL_FLEET:
	    return (boolean) ((HFast & INTRINSIC) != 0);
	case BL_LIGHTSTEP:
	    return (boolean) ((HStealth & INTRINSIC) != 0);
	case BL_NIGHTEYES:
	    return (boolean) (Infravision != 0);
	case BL_POISONWARD:
	    return (boolean) ((HPoison_resistance & INTRINSIC) != 0);
	case BL_PETBOND:
	    return (boolean) (!first_pet());
	case BL_DEVOUT:
	    return (boolean) (u.ualign.record >= 10);
	}
	return FALSE;
}

STATIC_OVL void
bless_apply(b)
int b;
{
	struct monst *pet;
	int otyp, n;

	lrun.blessings |= 1L << b;
	switch (b) {
	case BL_HEARTY:
	    if (u.uhunger < 1500)
		u.uhunger = 1500;
	    newuhs(FALSE);
	    break;
	case BL_LUCKY:
	    u.uluck = min(u.uluck + 3, LUCKMAX);
	    break;
	case BL_CARTOGRAPHER:
	    if (u.uz.dnum == 0 && u.uz.dlevel <= 2)
		do_mapping();
	    break;
	case BL_PURSE:
#ifndef GOLDOBJ
	    u.ugold += 300;
	    u.ugold0 += 300;
#else
	    {
		struct obj *gold = mksobj(GOLD_PIECE, FALSE, FALSE);

		gold->quan = 300L;
		gold->owt = weight(gold);
		(void) addinv(gold);
		u.umoney0 += 300;
	    }
#endif
	    break;
	case BL_FLEET:
	    if (!(HFast & INTRINSIC)) {
		HFast |= FROMOUTSIDE;
		lrun.fleet_until = moves + 1500;
	    }
	    break;
	case BL_KEENEYE:
	    incr_itimeout(&HSearching, 2000);
	    break;
	case BL_SCHOLAR:
	    legacy_know_random(SCROLL_CLASS);
	    break;
	case BL_PETBOND:
	    if ((pet = first_pet()) != 0) {
		(void) grow_up(pet, (struct monst *) 0);
		pet->mhp = pet->mhpmax;
	    }
	    break;
	case BL_PACKRAT:
	    (void) legacy_item(SACK, 1, 0, 0);
	    break;
	case BL_TORCH:
	    (void) legacy_item(OIL_LAMP, 1, 1, 0);
	    break;
	case BL_HERBALIST:
	    (void) legacy_item(SPRIG_OF_WOLFSBANE, 2, 0, 0);
	    (void) legacy_item(CLOVE_OF_GARLIC, 2, 0, 0);
	    break;
	case BL_LIGHTSTEP:
	    HStealth |= FROMOUTSIDE;
	    break;
	case BL_HOLYWATER:
	    (void) legacy_item(POT_WATER, 2, 0, 1);
	    break;
	case BL_NIGHTEYES:
	    HInfravision |= FROMOUTSIDE;
	    break;
	case BL_DEVOUT:
	    if (u.ualign.record < 10)
		u.ualign.record = 10;
	    break;
	case BL_TINNER:
	    (void) legacy_item(TINNING_KIT, 1, rn1(30, 30), 0);
	    break;
	case BL_POISONWARD:
	    incr_itimeout(&HPoison_resistance, 1000);
	    break;
	case BL_REMOVECURSE:
	    (void) legacy_item(SCR_REMOVE_CURSE, 1, 0, 1);
	    break;
	case BL_STONEWARD:
	    {
		struct obj *obj = mksobj(CORPSE, FALSE, FALSE);

		obj_stop_timers(obj);
		obj->corpsenm = PM_LIZARD;
		obj->dknown = obj->bknown = 1;
		obj->cursed = obj->blessed = 0;
		obj->owt = weight(obj);
		(void) addinv(obj);
	    }
	    break;
	case BL_SPAREWAND:
	    for (n = 0; n < 200; n++) {
		otyp = rnd_class(bases[WAND_CLASS], bases[WAND_CLASS + 1] - 1);
		if (otyp != WAN_WISHING && objects[otyp].oc_prob > 0)
		    break;
	    }
	    if (otyp == WAN_WISHING)
		otyp = WAN_STRIKING;
	    (void) legacy_item(otyp, 1, 3, 0);
	    break;
	case BL_TRAINED:
	    u.weapon_slots++;
	    break;
	}
	flags.botl = 1;
}

/* the draw at the start of a Legacy game: Favor I keeps 1 of 2,
   Favor II 1 of 3 and Favor III 2 of 4 */
void
fate_draw()
{
	winid win;
	menu_item *pick = 0;
	int pool[NUM_BLESSINGS], drawn[NUM_BLESSINGS];
	int rank = lrun.lineage[LIN_FAVOR], npool = 0, ndraw, keep, b, i, n;
	char buf[BUFSZ];

	if (!LINEAGE_RULES() || rank <= 0)
	    return;
	ndraw = rank + 1;
	keep = (rank >= 3) ? 2 : 1;
	if (fate_bless_owned(BL_HEIRSLUCK))
	    ndraw++;
	for (b = 0; b < NUM_BLESSINGS; b++)
	    if (fate_bless_owned(b) && !bless_useless(b))
		pool[npool++] = b;
	if (!npool)
	    return;
	if (ndraw > npool)
	    ndraw = npool;
	for (i = 0; i < ndraw; i++) {
	    n = rn2(npool);
	    drawn[i] = pool[n];
	    pool[n] = pool[--npool];
	}
	if (keep > ndraw)
	    keep = ndraw;

	for (;;) {
	    win = create_nhwindow(NHW_MENU);
	    start_menu(win);
	    Sprintf(buf, "Your Favor draws %d blessing%s. Keep %s.", ndraw,
		    plur(ndraw), keep == 1 ? "one" : "two");
	    add_text(win, buf);
	    add_text(win, "");
	    for (i = 0; i < ndraw; i++) {
		Sprintf(buf, "%s: %s", blessings[drawn[i]].name,
			blessings[drawn[i]].desc);
		add_choice(win, i + 1, 'a' + i, buf, FALSE);
	    }
	    end_menu(win, "Blessings");
	    n = select_menu(win, PICK_ANY, &pick);
	    destroy_nhwindow(win);
	    if (n > keep) {
		free((genericptr_t) pick), pick = 0;
		pline("You may keep only %s.", keep == 1 ? "one" : "two");
		continue;
	    }
	    break;
	}
	if (n <= 0) {
	    /* backed out: keep the first ones drawn */
	    for (i = 0; i < keep; i++)
		bless_apply(drawn[i]);
	} else {
	    for (i = 0; i < n; i++)
		bless_apply(drawn[pick[i].item.a_int - 1]);
	    free((genericptr_t) pick);
	}
	for (*buf = '\0', b = 0; b < NUM_BLESSINGS; b++)
	    if (lrun.blessings & (1L << b))
		Sprintf(eos(buf), "%s%s", *buf ? " and " : "",
			blessings[b].name);
	if (*buf)
	    pline("Blessed: %s.", buf);
}

/* ------------------------------------------------------------------ */
/* vows */

STATIC_OVL long
vow_count(v)
int v;
{
	switch (v) {
	case VOW_VEGETARIAN:	return u.uconduct.unvegetarian;
	case VOW_ATHEIST:	return u.uconduct.gnostic;
	case VOW_ILLITERATE:	return u.uconduct.literate;
	case VOW_WISHLESS:	return u.uconduct.wishes;
	case VOW_POLYSELF:	return u.uconduct.polyselfs;
	case VOW_CELIBATE:	return u.uconduct.celibacy;
	case VOW_ARTIWISH:	return u.uconduct.wisharti;
	case VOW_POLYPILE:	return u.uconduct.polypiles;
	case VOW_VEGAN:		return u.uconduct.unvegan;
	case VOW_WEAPONLESS:	return u.uconduct.weaphit;
	case VOW_FOODLESS:	return u.uconduct.food;
	case VOW_PACIFIST:	return u.uconduct.killer;
	}
	return 0L;
}

/* may this vow be taken?  The four hard ones open with the feat "Keep a
   vow through Act I" */
STATIC_OVL boolean
vow_open(v)
int v;
{
	return (boolean) (!vows[v].hard || vow_hard_open(v));
}

/* the Renown bonus, in percent, from vows kept so far */
int
fate_vow_bonus()
{
	int v, pct = 0;

	if (!LINEAGE_RULES())
	    return 0;
	for (v = 0; v < NUM_VOWS; v++)
	    if ((lrun.vows & (1 << v)) && !(lrun.vows_broken & (1 << v))
		    && vow_count(v) <= lrun.vow_start[v])
		pct += vows[v].bonus;
	return pct;
}

/* "vegetarian and atheist (+35%)", or "none" */
void
fate_vow_text(buf, prepare)
char *buf;
boolean prepare;	/* from the profile, for the next game */
{
	int v, mask = prepare ? lprof.vows : lrun.vows, pct = 0;

	*buf = '\0';
	for (v = 0; v < NUM_VOWS; v++)
	    if (mask & (1 << v)) {
		Sprintf(eos(buf), "%s%s", *buf ? " and " : "", vows[v].key);
		pct += vows[v].bonus;
	    }
	if (*buf)
	    Sprintf(eos(buf), " (+%d%% Renown if kept)", pct);
	else
	    Strcpy(buf, "none");
}

/* the Prepare screen's vow list */
void
fate_vow_menu()
{
	winid win;
	menu_item *pick = 0;
	char buf[BUFSZ];
	int v, n, i, mask;

	for (;;) {
	    win = create_nhwindow(NHW_MENU);
	    start_menu(win);
	    add_text(win, "Take up to two vows. Each one kept to the end of");
	    add_text(win, "the game, even one that ends in death, adds its");
	    add_text(win, "bonus to the Renown multiplier. Breaking one only");
	    add_text(win, "loses its bonus. They carry over to Play again.");
	    add_text(win, "");
	    for (v = 0; v < NUM_VOWS; v++) {
		Sprintf(buf, "%s (+%d%%): never %s", vows[v].name,
			vows[v].bonus, vows[v].never);
		if (!vow_open(v)) {
		    Sprintf(eos(buf), " [locked: keep a vow through Act I]");
		    add_text(win, buf);
		} else
		    add_choice(win, v + 1, 'a' + v, buf,
			       (lprof.vows & (1 << v)) != 0);
	    }
	    end_menu(win, "Vows");
	    n = select_menu(win, PICK_ANY, &pick);
	    destroy_nhwindow(win);
	    if (n < 0)
		return;
	    if (n > MAX_VOWS) {
		free((genericptr_t) pick), pick = 0;
		pline("You may take at most two vows.");
		continue;
	    }
	    break;
	}
	for (mask = 0, i = 0; i < n; i++)
	    mask |= 1 << (pick[i].item.a_int - 1);
	if (pick)
	    free((genericptr_t) pick);
	lprof.vows = mask;
	lprof.dirty = TRUE;
	legacy_write(FALSE);
}

/* ------------------------------------------------------------------ */
/* the run: setting up, the start and every turn */

/* called by legacy_setup_run(): the choices made in the Prepare screen */
void
fate_setup_run()
{
	int v, r = flags.initrole;

	if (!LINEAGE_RULES())
	    return;
	for (v = 0; v < NUM_VOWS; v++)
	    if ((lprof.vows & (1 << v)) && vow_open(v))
		lrun.vows |= 1 << v;
	lrun.kit = (r >= 0 && r < MAX_ROLES && kit_ok(r, lprof.kit[r]))
		   ? lprof.kit[r] : KIT_A;
	if (lprof.heir_pick > 0 && lprof.heir_pick <= lprof.nvault) {
	    struct legacy_heirloom *h = &lprof.vault[lprof.heir_pick - 1];

	    if (h->role < 0 || h->role == r || r < 0) {
		lrun.heir_pick = lprof.heir_pick;
		lrun.unaided = FALSE;	/* an heirloom is help */
	    }
	}
}

/* called from legacy_newgame() once the hero, level and pet exist */
void
fate_newgame(pet)
struct monst *pet;
{
	struct legacy_heirloom *h;
	struct obj *obj, dummy;
	char name[BUFSZ];
	int v;

	for (v = 0; v < NUM_VOWS; v++)
	    lrun.vow_start[v] = vow_count(v);

	/* the Necromancer's kit B: a second ghoul */
	if (Role_if(PM_NECROMANCER) && lrun.kit == KIT_B && pet)
	    (void) makedog();

	/* the heirloom leaves the vault */
	if (lrun.heir_pick > 0 && lrun.heir_pick <= lprof.nvault) {
	    h = &lprof.vault[lrun.heir_pick - 1];
	    dummy.otyp = h->otyp;	/* a container comes empty */
	    obj = mksobj(h->otyp, !Is_container(&dummy), FALSE);
	    obj->quan = (long) h->quan;
	    obj->cursed = obj->blessed = 0;
	    obj->oeroded = obj->oeroded2 = 0;
	    obj->oerodeproof = h->erodeproof;
	    if (objects[h->otyp].oc_charged)
		obj->spe = h->spe;
	    obj->dknown = obj->bknown = obj->rknown = 1;
	    if (objects[h->otyp].oc_uses_known)
		obj->known = 1;
	    obj->owt = weight(obj);
	    discover_object(h->otyp, TRUE, FALSE);
	    heir_name(h, name);
	    obj = oname(obj, name);
	    obj = addinv(obj);
	    lrun.heir_oid = obj->o_id;
	    lrun.heir_runs = h->runs;
	    legacy_clean_field(lrun.heir_owner, h->owner, PL_NSIZ);
	    vault_remove(lrun.heir_pick - 1);
	    lprof.heir_pick = 0;
	    lprof.dirty = TRUE;
	}
	lrun.heir_pick = 0;
}

/* after the welcome message: the draw, and what the game carries */
void
fate_welcome()
{
	struct obj *obj;
	const struct kit_def *k;
	char buf[BUFSZ];
	int b;

	if (!LEGACY_RULES())
	    return;
	if (lrun.mode == LMODE_DAILY)	/* the Daily's blessing */
	    for (b = 0; b < NUM_BLESSINGS; b++)
		if (lrun.blessings & (1L << b)) {
		    lrun.blessings &= ~(1L << b);
		    bless_apply(b);
		    pline("Today's blessing: %s.", fate_bless_name(b));
		}
	if (lrun.kit != KIT_A && (k = find_kit(flags.initrole)) != 0)
	    pline("You set out with kit %c, the %s.",
		  lrun.kit == KIT_B ? 'B' : 'C', k->name[min(lrun.kit, 2)]);
	if (lrun.heir_oid)
	    for (obj = invent; obj; obj = obj->nobj)
		if (obj->o_id == lrun.heir_oid) {
		    You("carry %s's heirloom: %s.", lrun.heir_owner,
			doname(obj));
		    break;
		}
	if (lrun.vows) {
	    fate_vow_text(buf, FALSE);
	    You("have vowed: %s.", buf);
	}
	if (LINEAGE_RULES())
	    mastery_skill();
	fate_draw();
	legacy_write(FALSE);
}

/* once per turn */
void
fate_turn()
{
	int v;

	if (!LEGACY_RULES())
	    return;
	for (v = 0; v < NUM_VOWS; v++)
	    if ((lrun.vows & (1 << v)) && !(lrun.vows_broken & (1 << v))
		    && vow_count(v) > lrun.vow_start[v]) {
		lrun.vows_broken |= 1 << v;
		You("have broken your %s vow; it will no longer pay.",
		    vows[v].key);
	    }
	if (lrun.fleet_until && moves >= lrun.fleet_until) {
	    lrun.fleet_until = 0;
	    if (HFast & FROMOUTSIDE) {
		HFast &= ~FROMOUTSIDE;
		if (!Fast)
		    You("slow down as the Fleet blessing fades.");
	    }
	}
	if (fate_blessed(BL_KEENEYE) && moves == 2001 && !Searching)
	    pline("The Keen eye blessing fades.");
	if (fate_blessed(BL_POISONWARD) && moves == 1001
		&& !Poison_resistance)
	    pline("The Poison ward blessing fades.");
}

/* the hero arrived on a level for the first time */
void
fate_arrived()
{
	int dep = depth(&u.uz);

	if (!In_endgame(&u.uz)) {
	    if (dep >= 5)
		fate_mastery(1);
	    if (dep >= 10)
		fate_mastery(2);
	}
	if (fate_blessed(BL_CARTOGRAPHER) && u.uz.dnum == 0
		&& u.uz.dlevel <= 2) {
	    do_mapping();
	    pline("The Cartographer blessing shows you this level.");
	}
}

/* the hero picked something up */
void
fate_got_object(obj)
struct obj *obj;
{
	if (lrun.anc_ledger && lrun.anc_ledger == ledger_no(&u.uz)
		&& obj->o_id != lrun.heir_oid && !program_state.gameover
		&& moves > 1)
	    legacy_feat(F_BELONGINGS);
}

/* "Vigor I, Favor I" goes on with "kit B, Hearty meal, ..." */
STATIC_OVL void
append_part(buf, part, maxlen)
char *buf;
const char *part;
int maxlen;
{
	if ((int) (strlen(buf) + strlen(part) + 2) >= maxlen)
	    return;
	if (*buf)
	    Strcat(buf, ", ");
	Strcat(buf, part);
}

void
fate_help_text(buf, maxlen)
char *buf;
int maxlen;
{
	char part[BUFSZ];
	int b, v;

	if (lrun.kit != KIT_A) {
	    Sprintf(part, "kit %c", lrun.kit == KIT_B ? 'B' : 'C');
	    append_part(buf, part, maxlen);
	}
	for (b = 0; b < NUM_BLESSINGS; b++)
	    if (lrun.blessings & (1L << b))
		append_part(buf, blessings[b].name, maxlen);
	if (lrun.heir_oid || lrun.heir_pick)
	    append_part(buf, "an heirloom", maxlen);
	for (v = 0; v < NUM_VOWS; v++)
	    if (lrun.vows & (1 << v)) {
		Sprintf(part, "%s vow%s", vows[v].key,
			(lrun.vows_broken & (1 << v)) ? " (broken)" : "");
		append_part(buf, part, maxlen);
	    }
}

/* ------------------------------------------------------------------ */
/* role mastery and kits */

STATIC_OVL const struct kit_def *
find_kit(r)
int r;
{
	int i;

	if (r < 0 || !roles[r].name.m)
	    return 0;
	for (i = 0; kits[i].role; i++)
	    if (!strcmp(kits[i].role, roles[r].name.m))
		return &kits[i];
	return 0;
}

/* may role r use kit k? */
STATIC_OVL boolean
kit_ok(r, k)
int r, k;
{
	int m = (r >= 0 && r < MAX_ROLES) ? lprof.mastery[r] : 0;

	switch (k) {
	case KIT_A:
	    return TRUE;
	case KIT_B:
	    return (boolean) (m >= 1 && find_kit(r));
	case KIT_C:
	    return (boolean) (m >= 3 && find_kit(r));
	case KIT_C2:
	    return (boolean) (m >= 3 && roles[r].malenum == PM_UNDEAD_SLAYER);
	}
	return FALSE;
}

/* the hero reached mastery level n with the current role */
void
fate_mastery(n)
int n;
{
	int r = flags.initrole;

	legacy_init();
	if (!legacy_counts() || r < 0 || r >= MAX_ROLES
		|| lprof.mastery[r] >= n)
	    return;
	lprof.mastery[r] = n;
	lprof.dirty = TRUE;
	if (!program_state.gameover)
	    pline("Your mastery of the %s rises to %d: %s.", roles[r].name.m, n,
		  mastery_reward[n]);
	if (n >= MASTERY_MAX)
	    legacy_feat(F_MASTERY5);
	legacy_write(FALSE);
}

/* the Prepare screen, once the character is chosen: which kit */
void
fate_kit_menu()
{
	const struct kit_def *k;
	winid win;
	menu_item *pick = 0;
	char buf[BUFSZ];
	int r = flags.initrole, n, i, cur;
	static const char *const letter[] = { "A", "B", "C", "C" };

	if (r < 0 || r >= MAX_ROLES || !(k = find_kit(r)) || !kit_ok(r, KIT_B))
	    return;
	cur = kit_ok(r, lprof.kit[r]) ? lprof.kit[r] : KIT_A;
	win = create_nhwindow(NHW_MENU);
	start_menu(win);
	Sprintf(buf, "Your mastery of the %s is %d.", roles[r].name.m,
		lprof.mastery[r]);
	add_text(win, buf);
	add_text(win, "");
	for (i = KIT_A; i <= KIT_C2; i++) {
	    if (!kit_ok(r, i))
		continue;
	    if (i == KIT_C2)
		Sprintf(buf, "Kit C, the Whip hunter: always the bullwhip kit");
	    else
		Sprintf(buf, "Kit %s, the %s: %s", letter[i], k->name[i],
			k->desc[i]);
	    add_choice(win, i + 1, 'a' + i, buf, i == cur);
	}
	if (!kit_ok(r, KIT_C)) {
	    Sprintf(buf, "Kit C, the %s: opens at mastery 3 (%s)", k->name[2],
		    mastery_reach[3]);
	    add_text(win, buf);
	}
	end_menu(win, "Which kit?");
	n = select_menu(win, PICK_ONE, &pick);
	destroy_nhwindow(win);
	if (n > 0) {
	    /* a preselected entry picked again comes back as two */
	    i = pick[n - 1].item.a_int - 1;
	    if (n > 1 && i == cur)
		i = pick[0].item.a_int - 1;
	    free((genericptr_t) pick);
	    lprof.kit[r] = i;
	    lprof.dirty = TRUE;
	    legacy_write(FALSE);
	}
}

STATIC_OVL struct obj *
kit_find(otyp)
int otyp;
{
	struct obj *obj;

	for (obj = invent; obj; obj = obj->nobj)
	    if (obj->otyp == otyp)
		return obj;
	return 0;
}

/* a weapon (or with 'ammo', ammunition) using skill 'skill'; races swap
   in their own kinds of some weapons, so look by skill */
STATIC_OVL struct obj *
kit_find_skill(skill, ammo)
int skill;
boolean ammo;
{
	struct obj *obj;

	for (obj = invent; obj; obj = obj->nobj)
	    if ((obj->oclass == WEAPON_CLASS || is_weptool(obj))
		    && objects[obj->otyp].oc_skill == (ammo ? -skill : skill))
		return obj;
	return 0;
}

STATIC_OVL void
kit_drop(obj)
struct obj *obj;
{
	if (!obj)
	    return;
	if (obj->owornmask)
	    setnotworn(obj);
	useupall(obj);
}

STATIC_OVL void
kit_drop_all(otyp)
int otyp;
{
	struct obj *obj;

	while ((obj = kit_find(otyp)) != 0)
	    kit_drop(obj);
}

/* the hero can use a weapon from the kit at Basic, as with the usual kit */
STATIC_OVL void
kit_skill(obj)
struct obj *obj;
{
	int skill = weapon_type(obj);

	if (skill == P_NONE || P_RESTRICTED(skill) || P_SKILL(skill) >= P_BASIC
		|| P_MAX_SKILL(skill) < P_BASIC)
	    return;
	P_SKILL(skill) = P_BASIC;
	P_ADVANCE(skill) = practice_needed_to_advance(P_BASIC - 1, skill);
}

STATIC_OVL struct obj *
kit_give(otyp, quan, spe, bless)
int otyp, quan, spe, bless;
{
	struct obj *obj = legacy_item(otyp, quan, spe, bless);

	if (obj->oclass == WEAPON_CLASS || is_weptool(obj))
	    kit_skill(obj);
	return obj;
}

STATIC_OVL void
kit_wield(obj)
struct obj *obj;
{
	if (obj == uswapwep)
	    setuswapwep((struct obj *) 0, FALSE);
	if (obj == uquiver)
	    setuqwep((struct obj *) 0);
	if (uwep && uwep != obj && !uswapwep)
	    setuswapwep(uwep, FALSE);
	setuwep(obj, FALSE);
}

STATIC_OVL void
kit_quiver(obj)
struct obj *obj;
{
	setuqwep(obj);
}

STATIC_OVL void
kit_wear(obj)
struct obj *obj;
{
	if (is_shield(obj) && !uarms)
	    setworn(obj, W_ARMS);
	else if (is_gloves(obj) && !uarmg)
	    setworn(obj, W_ARMG);
	else if (is_cloak(obj) && !uarmc)
	    setworn(obj, W_ARMC);
	else if (is_suit(obj) && !uarm)
	    setworn(obj, W_ARM);
	find_ac();
}

/* riding one step better than the role starts with */
STATIC_OVL void
kit_ride()
{
#ifdef STEED
	int to = min(P_SKILL(P_RIDING) + 1, P_MAX_SKILL(P_RIDING));

	if (P_RESTRICTED(P_RIDING) || to <= P_SKILL(P_RIDING))
	    return;
	P_SKILL(P_RIDING) = to;
	P_ADVANCE(P_RIDING) = practice_needed_to_advance(to - 1, P_RIDING);
#endif
}

STATIC_OVL void
kit_halve_gold(dummy)
int dummy;
{
#ifndef GOLDOBJ
	u.ugold /= 2;
	u.ugold0 = u.ugold;
#else
	struct obj *g;

	if ((g = kit_find(GOLD_PIECE)) != 0) {
	    g->quan /= 2;
	    g->owt = weight(g);
	    u.umoney0 = g->quan;
	}
#endif
}

/* change the usual kit into kit B or C; called from legacy_newgame()
   before Arms.  The Rogue's and Undead Slayer's guns are chosen in
   u_init() */
void
fate_kit()
{
	struct obj *obj;
	int kit = lrun.kit, n;

	if (!LEGACY_RULES() || kit == KIT_A)
	    return;
	switch (Role_switch) {
	case PM_ARCHEOLOGIST:
	    kit_drop(kit_find(TINNING_KIT));
	    if (kit == KIT_B) {
		kit_drop(kit_find(TOUCHSTONE));
		(void) kit_give(SCR_MAGIC_MAPPING, 2, 0, 0);
	    } else {
		kit_drop(kit_find(BULLWHIP));
		kit_wield(kit_give(RAPIER, 1, 0, 0));
	    }
	    break;
	case PM_BARBARIAN:
	    if (kit == KIT_B) {
		kit_drop(kit_find(TWO_HANDED_SWORD));
		kit_drop(kit_find(BATTLE_AXE));
		if ((obj = kit_find_skill(P_AXE, FALSE)) != 0)
		    obj->spe = 1;
		else
		    obj = kit_give(AXE, 1, 1, 0);
		kit_wield(obj);
		kit_wear(kit_give(LARGE_SHIELD, 1, 0, 0));
	    } else {
		for (;;) {
		    for (obj = invent; obj; obj = obj->nobj)
			if (obj->oclass == WEAPON_CLASS)
			    break;
		    if (!obj)
			break;
		    kit_drop(obj);
		}
		kit_wear(kit_give(LEATHER_GLOVES, 1, 2, 0));
		(void) kit_give(FOOD_RATION, 1, 0, 0);
	    }
	    break;
	case PM_CAVEMAN:
	    if (kit == KIT_B) {
		kit_drop(kit_find_skill(P_CLUB, FALSE));
		kit_wield(kit_give(SPEAR, 1, 1, 0));
	    } else {
		kit_drop(kit_find_skill(P_SLING, FALSE));
		kit_drop_all(FLINT);
		obj = kit_give(BOW, 1, 0, 0);
		if (!uswapwep)
		    setuswapwep(obj, FALSE);
		kit_quiver(kit_give(ARROW, 30, 0, 0));
	    }
	    break;
	case PM_FLAME_MAGE:
	case PM_ICE_MAGE:
	    if (kit == KIT_B) {
		kit_drop(kit_find(QUARTERSTAFF));
		kit_wield(kit_give(DAGGER, 3, 0, 0));
	    } else {
		for (obj = invent; obj; obj = obj->nobj)
		    if (obj->oclass == RING_CLASS)
			break;
		kit_drop(obj);
		if (Role_if(PM_FLAME_MAGE)) {
		    (void) kit_give(OIL_LAMP, 1, 1, 0);
		    (void) kit_give(POT_OIL, 2, 0, 0);
		} else
		    kit_wear(kit_give(ELVEN_CLOAK, 1, 0, 0));
	    }
	    break;
	case PM_HEALER:
	    if (kit == KIT_B) {
		kit_drop(kit_find(SCALPEL));
		kit_wield(kit_give(QUARTERSTAFF, 1, 1, 0));
		kit_halve_gold(0);
	    } else {
		kit_drop(kit_find(WAN_SLEEP));
		(void) kit_give(POT_HEALING, 4, 0, 0);
	    }
	    break;
	case PM_KNIGHT:
	    if (kit == KIT_B) {
		kit_drop(kit_find(PLATE_MAIL));
		kit_wear(kit_give(RING_MAIL, 1, 0, 0));
		(void) kit_give(FOOD_RATION, 2, 0, 0);
	    } else {
		if ((obj = kit_find(LANCE)) != 0)
		    obj->spe = 1;
		kit_ride();
		kit_drop_all(APPLE);
		kit_drop_all(CARROT);
	    }
	    break;
	case PM_MONK:
	    if (kit == KIT_B) {
		kit_drop(kit_find(ROBE));
		kit_wear(kit_give(LEATHER_CLOAK, 1, 1, 0));
	    } else {
		kit_drop_all(ORANGE);
		kit_drop_all(FORTUNE_COOKIE);
		(void) kit_give(POT_WATER, 4, 0, 1);
	    }
	    break;
	case PM_NECROMANCER:
	    if (kit == KIT_B)
		kit_drop(kit_find(WAN_DRAINING));	/* the ghoul comes later */
	    else {
		kit_drop(kit_find(QUARTERSTAFF));
		for (obj = invent; obj; obj = obj->nobj)
		    if (obj->oclass == RING_CLASS)
			break;
		kit_drop(obj);
		kit_wield(kit_give(DAGGER, 6, 0, 0));
	    }
	    break;
	case PM_PRIEST:
	    if (kit == KIT_B) {
		kit_drop(kit_find(MACE));
		kit_wield(kit_give(FLAIL, 1, 1, 1));
		if ((obj = kit_find(POT_WATER)) != 0) {
		    if (obj->quan > 2) {
			obj->quan -= 2;
			obj->owt = weight(obj);
		    } else
			kit_drop(obj);
		}
	    } else {
		for (obj = invent; obj; obj = obj->nobj)
		    if (obj->oclass == SPBOOK_CLASS && obj->otyp != SPE_HEALING)
			break;
		kit_drop(obj);
		(void) kit_give(POT_WATER, 4, 0, 1);
	    }
	    break;
	case PM_RANGER:
	    while ((obj = kit_find_skill(P_BOW, FALSE)) != 0)
		kit_drop(obj);
	    while ((obj = kit_find_skill(P_BOW, TRUE)) != 0)
		kit_drop(obj);
	    if (kit == KIT_B) {
		obj = kit_give(SLING, 1, 1, 0);
		if (!uswapwep)
		    setuswapwep(obj, FALSE);
		kit_quiver(kit_give(FLINT, 40, 0, 0));
		(void) kit_give(CRAM_RATION, 2, 0, 0);
	    } else if (!kit_find_skill(P_CROSSBOW, FALSE)) {
		obj = kit_give(CROSSBOW, 1, 0, 0);
		if (!uswapwep)
		    setuswapwep(obj, FALSE);
		kit_quiver(kit_give(CROSSBOW_BOLT, 40, 0, 0));
	    }
	    break;
	case PM_ROGUE:
	    if (kit == KIT_C) {
		kit_drop_all(DART);
		(void) kit_give(SCR_MAGIC_MAPPING, 2, 0, 0);
	    }
	    break;
	case PM_SAMURAI:
	    if (kit == KIT_B) {
		if ((obj = kit_find(KATANA)) != 0)
		    obj->spe = 1;
		kit_drop_all(YUMI);
		kit_drop_all(YA);
	    } else {
		kit_drop(kit_find(SHORT_SWORD));
		if ((obj = kit_find(YUMI)) != 0)
		    obj->spe = 1;
		if ((obj = kit_find(YA)) != 0) {
		    obj->quan = 60L;
		    obj->owt = weight(obj);
		}
	    }
	    break;
#ifdef TOURIST
	case PM_TOURIST:
	    if (kit == KIT_B) {
		kit_halve_gold(0);
		(void) kit_give(DAGGER, 1, 1, 0);
		(void) kit_give(POT_EXTRA_HEALING, 2, 0, 0);
	    } else {
		if ((obj = kit_find(EXPENSIVE_CAMERA)) != 0)
		    obj->spe *= 2;
		if ((obj = kit_find(DART)) != 0) {
		    obj->quan = (obj->quan + 1) / 2;
		    obj->owt = weight(obj);
		}
	    }
	    break;
#endif
	case PM_VALKYRIE:
	    if (kit == KIT_B) {
		kit_drop(kit_find_skill(P_SPEAR, FALSE));
		kit_wield(kit_give(AXE, 1, 1, 0));
		if (uarms)
		    uarms->spe = 1;
	    } else {
		kit_drop(uarms);
		if ((obj = kit_find_skill(P_DAGGER, FALSE)) != 0) {
		    obj->quan = 8L;
		    obj->spe = 2;
		    obj->owt = weight(obj);
		}
	    }
	    break;
	case PM_WIZARD:
	    if (kit == KIT_B) {
		kit_drop(kit_find(QUARTERSTAFF));
		kit_wield(kit_give(DAGGER, 3, 0, 0));
	    } else {
		for (obj = invent; obj; obj = obj->nobj)
		    if (obj->oclass == WAND_CLASS)
			break;
		kit_drop(obj);
		for (n = 0; n < 500; n++) {
		    int otyp = rnd_class(bases[SPBOOK_CLASS],
					 SPE_BLANK_PAPER - 1);

		    if (objects[otyp].oc_level <= 2 && !kit_find(otyp)
			    && otyp != SPE_BLANK_PAPER
			    && objects[otyp].oc_prob > 0) {
			(void) kit_give(otyp, 1, 0, 1);
			break;
		    }
		}
	    }
	    break;
#ifdef YEOMAN
	case PM_YEOMAN:
	    if (kit == KIT_B) {
		kit_drop(kit_find(PARTISAN));
		obj = kit_give(BOW, 1, 0, 0);
		if (!uswapwep)
		    setuswapwep(obj, FALSE);
		kit_quiver(kit_give(ARROW, 30, 0, 0));
	    } else {
		if ((obj = kit_find(PARTISAN)) != 0)
		    obj->spe = 2;
		kit_ride();
		kit_drop(kit_find(FISHING_POLE));
	    }
	    break;
#endif
	}
	find_ac();
	update_inventory();
}

/* mastery 2: one weapon skill, chosen once per role, starts at Basic */
STATIC_OVL void
mastery_skill()
{
	winid win;
	menu_item *pick = 0;
	char buf[BUFSZ];
	int r = flags.initrole, sk, n;

	if (r < 0 || r >= MAX_ROLES || lprof.mastery[r] < 2)
	    return;
	if (!lprof.mskill[r]) {
	    win = create_nhwindow(NHW_MENU);
	    start_menu(win);
	    Sprintf(buf, "Your mastery of the %s lets one weapon skill",
		    roles[r].name.m);
	    add_text(win, buf);
	    add_text(win, "start at Basic in every game. Which one?");
	    add_text(win, "(You can change it later under Role mastery.)");
	    add_text(win, "");
	    for (n = 0, sk = P_FIRST_WEAPON; sk <= P_LAST_WEAPON; sk++)
		if (!P_RESTRICTED(sk) && P_MAX_SKILL(sk) >= P_BASIC
			&& P_SKILL(sk) < P_BASIC) {
		    add_choice(win, sk, 0, legacy_skill_name(sk), FALSE);
		    n++;
		}
	    end_menu(win, "Mastery: a weapon skill");
	    if (!n) {
		destroy_nhwindow(win);
		return;
	    }
	    n = select_menu(win, PICK_ONE, &pick);
	    destroy_nhwindow(win);
	    if (n <= 0)
		return;		/* asked again next game */
	    lprof.mskill[r] = pick[0].item.a_int;
	    free((genericptr_t) pick);
	    lprof.dirty = TRUE;
	}
	sk = lprof.mskill[r];
	if (sk >= P_FIRST_WEAPON && sk <= P_LAST_WEAPON && !P_RESTRICTED(sk)
		&& P_MAX_SKILL(sk) >= P_BASIC && P_SKILL(sk) < P_BASIC) {
	    P_SKILL(sk) = P_BASIC;
	    P_ADVANCE(sk) = practice_needed_to_advance(P_BASIC - 1, sk);
	    Your("mastery gives you Basic skill in %s.",
		 legacy_skill_name(sk));
	}
}

/* the Hall's Role mastery screen */
void
fate_mastery_menu()
{
	const struct kit_def *k;
	winid win;
	menu_item *pick = 0;
	char buf[BUFSZ];
	int r, m, n;

	for (;;) {
	    win = create_nhwindow(NHW_MENU);
	    start_menu(win);
	    add_text(win, "Mastery is earned by playing each role, in any mode:");
	    for (m = 1; m <= MASTERY_MAX; m++) {
		Sprintf(buf, "  %d: %s; gives %s", m, mastery_reach[m],
			mastery_reward[m]);
		add_text(win, buf);
	    }
	    add_text(win, "Kits and the skill apply to Legacy games only.");
	    add_text(win, "Pick a role to forget its mastery skill choice.");
	    add_text(win, "");
	    for (r = 0; roles[r].name.m && r < MAX_ROLES; r++) {
		m = lprof.mastery[r];
		k = find_kit(r);
		Sprintf(buf, "%s%s: mastery %d", roles[r].name.m,
			m >= MASTERY_MAX ? " (gold)" : "", m);
		if (m >= 1 && k)
		    Sprintf(eos(buf), "; kit %s", lprof.kit[r] == KIT_A ? "A"
			    : lprof.kit[r] == KIT_B ? "B" : "C");
		if (lprof.mskill[r])
		    Sprintf(eos(buf), "; %s at Basic",
			    legacy_skill_name(lprof.mskill[r]));
		if (m < MASTERY_MAX)
		    Sprintf(eos(buf), "; next: %s", mastery_reach[m + 1]);
		if (lprof.mskill[r])
		    add_choice(win, r + 1, 0, buf, FALSE);
		else {
		    anything any;

		    any.a_void = 0;
		    add_menu(win, NO_GLYPH, &any, 0, 0,
			     m >= MASTERY_MAX ? ATR_BOLD : ATR_NONE, buf,
			     MENU_UNSELECTED);
		}
	    }
	    end_menu(win, "Role mastery");
	    n = select_menu(win, PICK_ONE, &pick);
	    destroy_nhwindow(win);
	    if (n <= 0)
		return;
	    r = pick[0].item.a_int - 1;
	    free((genericptr_t) pick), pick = 0;
	    Sprintf(buf, "Choose a new mastery skill for the %s next game?",
		    roles[r].name.m);
	    if (yn(buf) == 'y') {
		lprof.mskill[r] = 0;
		lprof.dirty = TRUE;
		legacy_write(FALSE);
	    }
	}
}

/* ------------------------------------------------------------------ */
/* the heirloom vault */

/* general slots: one, plus Inheritance II and III */
STATIC_OVL int
vault_slots()
{
	int r = lprof.lineage[LIN_INHERITANCE];

	return 1 + (r >= 2) + (r >= 3);
}

/* heirlooms held in general slots (role < 0) or in role r's own slot */
STATIC_OVL int
vault_used(role)
int role;
{
	int i, n = 0;

	for (i = 0; i < lprof.nvault; i++)
	    if (lprof.vault[i].role == role)
		n++;
	return n;
}

STATIC_OVL void
heir_name(h, buf)
struct legacy_heirloom *h;
char *buf;
{
	Sprintf(buf, "%s's %s", h->owner, legacy_type_name(h->otyp));
}

STATIC_OVL void
vault_remove(i)
int i;
{
	if (i < 0 || i >= lprof.nvault)
	    return;
	for (; i < lprof.nvault - 1; i++)
	    lprof.vault[i] = lprof.vault[i + 1];
	lprof.nvault--;
	lprof.dirty = TRUE;
}

/* "Brunhild's long sword (+1, Valkyrie only)" */
void
fate_heir_text(buf)
char *buf;
{
	struct legacy_heirloom *h;
	char name[BUFSZ];

	if (lprof.heir_pick <= 0 || lprof.heir_pick > lprof.nvault) {
	    Strcpy(buf, "none");
	    return;
	}
	h = &lprof.vault[lprof.heir_pick - 1];
	heir_name(h, name);
	Strcpy(buf, name);
	if (h->role >= 0)
	    Sprintf(eos(buf), " (%s only)", roles[h->role].name.m);
}

/* the Prepare screen: which heirloom to take */
void
fate_heir_menu()
{
	winid win;
	menu_item *pick = 0;
	char buf[BUFSZ], name[BUFSZ];
	struct legacy_heirloom *h;
	int i, n;

	if (!lprof.nvault) {
	    pline("The vault is empty. A Legacy game that reaches dungeon level 5 may enshrine an item when it ends.");
	    return;
	}
	win = create_nhwindow(NHW_MENU);
	start_menu(win);
	add_text(win, "An heirloom starts in your inventory, identified and");
	add_text(win, "uncursed, and leaves the vault. Enshrine it again at");
	add_text(win, "the end of the game or it is lost. Taking one means");
	add_text(win, "the game is not Unaided.");
	add_text(win, "");
	add_choice(win, -1, 'a', "Take nothing", lprof.heir_pick == 0);
	for (i = 0; i < lprof.nvault; i++) {
	    h = &lprof.vault[i];
	    heir_name(h, name);
	    Sprintf(buf, "%s", name);
	    if (objects[h->otyp].oc_charged)
		Sprintf(eos(buf), " (%s%d)", h->spe >= 0 ? "+" : "", h->spe);
	    if (h->quan > 1)
		Sprintf(eos(buf), " x%d", h->quan);
	    if (h->role >= 0)
		Sprintf(eos(buf), ", %s only", roles[h->role].name.m);
	    if (h->runs)
		Sprintf(eos(buf), ", carried %d time%s", h->runs,
			plur(h->runs));
	    add_choice(win, i + 1, 'b' + i, buf, lprof.heir_pick == i + 1);
	}
	end_menu(win, "Take an heirloom?");
	n = select_menu(win, PICK_ONE, &pick);
	destroy_nhwindow(win);
	if (n <= 0)
	    return;
	i = pick[n - 1].item.a_int;
	if (n > 1 && i == (lprof.heir_pick ? lprof.heir_pick : -1))
	    i = pick[0].item.a_int;
	free((genericptr_t) pick);
	lprof.heir_pick = (i < 0) ? 0 : i;
	lprof.dirty = TRUE;
	legacy_write(FALSE);
}

/* the Hall's view of the vault */
void
fate_vault_menu()
{
	winid win;
	char buf[BUFSZ], name[BUFSZ];
	struct legacy_heirloom *h;
	int i, r = lprof.lineage[LIN_INHERITANCE];

	win = create_nhwindow(NHW_TEXT);
	Sprintf(buf, "The heirloom vault: %d of %d general slot%s used.",
		vault_used(-1), vault_slots(), plur(vault_slots()));
	putstr(win, ATR_BOLD, buf);
	putstr(win, 0, "");
	putstr(win, 0, "At the end of a Legacy game that reached dungeon");
	putstr(win, 0, "level 5 you may enshrine one item. Before a game");
	putstr(win, 0, "you may take one along, from the Prepare screen.");
	Sprintf(buf, "Heirlooms keep up to +%d and %d charges.",
		r >= 3 ? 3 : r >= 1 ? 2 : 1, r >= 3 ? 4 : r >= 1 ? 3 : 2);
	putstr(win, 0, buf);
	putstr(win, 0, "Mastery 4 with a role adds a slot only it can use.");
	putstr(win, 0, "");
	if (!lprof.nvault)
	    putstr(win, 0, "The vault is empty.");
	for (i = 0; i < lprof.nvault; i++) {
	    h = &lprof.vault[i];
	    heir_name(h, name);
	    Sprintf(buf, "%s", name);
	    if (objects[h->otyp].oc_charged)
		Sprintf(eos(buf), " (%s%d)", h->spe >= 0 ? "+" : "", h->spe);
	    if (h->quan > 1)
		Sprintf(eos(buf), " x%d", h->quan);
	    if (h->erodeproof)
		Strcat(buf, ", fixed");
	    if (h->role >= 0)
		Sprintf(eos(buf), ", %s slot", roles[h->role].name.m);
	    if (h->runs)
		Sprintf(eos(buf), ", carried %d time%s", h->runs,
			plur(h->runs));
	    if (lprof.heir_pick == i + 1)
		Strcat(buf, " [to be taken]");
	    putstr(win, 0, buf);
	}
	display_nhwindow(win, TRUE);
	destroy_nhwindow(win);
}

/* may this object go in the vault? */
STATIC_OVL boolean
heir_ok(obj)
struct obj *obj;
{
	if (obj->oartifact || obj->unpaid || obj->owornmask & (W_BALL | W_CHAIN))
	    return FALSE;
	switch (obj->otyp) {
	case WAN_WISHING:
	case MAGIC_LAMP:
	case AMULET_OF_YENDOR:
	case CANDELABRUM_OF_INVOCATION:
	case BELL_OF_OPENING:
	case SPE_BOOK_OF_THE_DEAD:
	case BOULDER:
	case STATUE:
	case FIGURINE:
	case CORPSE:
	case EGG:
	case TIN:
	case HEAVY_IRON_BALL:
	case IRON_CHAIN:
	    return FALSE;
	}
	if (obj->oclass == COIN_CLASS)
	    return FALSE;
	return TRUE;
}

/* the end of a game: one item may go to the vault */
void
fate_enshrine(how)
int how;
{
	struct legacy_heirloom *h, nh;
	struct obj *obj, *o;
	winid win;
	menu_item *pick = 0;
	anything any;
	char buf[BUFSZ], name[BUFSZ];
	int n, i, slot = -2, r = flags.initrole, ench, chg, ir;

	if (!LINEAGE_RULES() || discover || how == PANICKED
		|| how == TRICKED || deepest_lev_reached(FALSE) < 5)
	    return;
	for (obj = invent; obj; obj = obj->nobj)
	    if (heir_ok(obj))
		break;
	if (!obj)
	    return;

	win = create_nhwindow(NHW_MENU);
	start_menu(win);
	any.a_void = 0;
	add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE,
		 "Enshrine one item for a later character to take along.",
		 MENU_UNSELECTED);
	ir = lprof.lineage[LIN_INHERITANCE];
	ench = ir >= 3 ? 3 : ir >= 1 ? 2 : 1;
	chg = ir >= 3 ? 4 : ir >= 1 ? 3 : 2;
	Sprintf(buf, "It keeps up to +%d or %d charges; a stack keeps one,", ench,
		chg);
	add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE, buf, MENU_UNSELECTED);
	add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE,
		 "or up to 20 for weapons; containers go in empty.",
		 MENU_UNSELECTED);
	add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE, "", MENU_UNSELECTED);
	for (obj = invent; obj; obj = obj->nobj)
	    if (heir_ok(obj)) {
		any.a_obj = obj;
		add_menu(win, obj_to_glyph(obj), &any, obj->invlet, 0,
			 ATR_NONE, doname(obj), MENU_UNSELECTED);
	    }
	end_menu(win, "Enshrine an heirloom?");
	n = select_menu(win, PICK_ONE, &pick);
	destroy_nhwindow(win);
	if (n <= 0)
	    return;
	obj = pick[0].item.a_obj;
	free((genericptr_t) pick), pick = 0;

	/* a free slot: general first, then this role's own */
	if (vault_used(-1) < vault_slots())
	    slot = -1;
	else if (r >= 0 && r < MAX_ROLES && lprof.mastery[r] >= 4
		 && !vault_used(r))
	    slot = r;
	else {
	    win = create_nhwindow(NHW_MENU);
	    start_menu(win);
	    any.a_void = 0;
	    add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE,
		     "The vault is full. Replace which heirloom?",
		     MENU_UNSELECTED);
	    for (i = 0; i < lprof.nvault; i++) {
		h = &lprof.vault[i];
		if (h->role >= 0 && h->role != r)
		    continue;
		heir_name(h, name);
		any.a_int = i + 1;
		add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE, name,
			 MENU_UNSELECTED);
	    }
	    end_menu(win, "The vault is full");
	    n = select_menu(win, PICK_ONE, &pick);
	    destroy_nhwindow(win);
	    if (n <= 0)
		return;
	    i = pick[0].item.a_int - 1;
	    free((genericptr_t) pick);
	    slot = lprof.vault[i].role;
	    if (lprof.heir_pick == i + 1)
		lprof.heir_pick = 0;
	    else if (lprof.heir_pick > i + 1)
		lprof.heir_pick--;
	    vault_remove(i);
	}
	if (lprof.nvault >= MAX_VAULT)
	    return;

	(void) memset((genericptr_t) &nh, 0, sizeof nh);
	nh.otyp = obj->otyp;
	nh.role = slot;
	nh.erodeproof = obj->oerodeproof;
	nh.quan = (obj->oclass == WEAPON_CLASS) ? (short) min(obj->quan, 20L)
						 : 1;
	nh.spe = 0;
	if (objects[obj->otyp].oc_charged) {
	    int cap = (obj->oclass == WAND_CLASS || obj->oclass == TOOL_CLASS)
		      ? chg : ench;

	    nh.spe = (schar) min((int) obj->spe, cap);
	}
	if (lrun.heir_oid && obj->o_id == lrun.heir_oid) {
	    nh.runs = lrun.heir_runs + 1;
	    legacy_clean_field(nh.owner, lrun.heir_owner, PL_NSIZ);
	} else
	    legacy_clean_field(nh.owner, plname, PL_NSIZ);
	lprof.vault[lprof.nvault++] = nh;
	heir_name(&nh, name);

	/* the contents stay behind, then the item leaves the game */
	while ((o = obj->cobj) != 0) {
	    obj_extract_self(o);
	    place_object(o, u.ux, u.uy);
	}
	if (obj->owornmask)
	    setnotworn(obj);
	useupall(obj);
	pline("%s is in the vault.", upstart(name));
	if (nh.runs >= 3) {
	    long before = lrun.feat_renown;

	    legacy_feat(F_HEIRLOOM3);
	    if (lprof.ancestors)
		lprof.ancestors->renown += lrun.feat_renown - before;
	}
	lprof.dirty = TRUE;
	legacy_write(TRUE);
}

/* ------------------------------------------------------------------ */
/* shops */

/* the Charisma shopkeepers see */
int
fate_shop_cha()
{
	int cha = ACURR(A_CHA);

	if (fate_blessed(BL_MERCHANT) && cha < 18)
	    cha = 18;
	return cha;
}

/*fate.c*/
