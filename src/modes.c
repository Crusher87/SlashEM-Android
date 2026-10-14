/*	SCCS Id: @(#)modes.c	0.0.7E7F3	2026/10/14	*/
/* NetHack may be freely redistributed.  See license for details. */

/*
 * Slash'EM Legacy: modes and pacts.  Pacts make a Legacy game harder
 * for more Renown.  An Expedition cuts the game into three acts, each
 * adding to a pot the player can bank by retiring.  The Daily is the
 * same seed for everyone each day, and a custom seed replays any seed
 * for practice.  Seeded games build every level from its own seed, so
 * the maps match while the fights do not.
 */

#include "hack.h"

/* ------------------------------------------------------------------ */
/* the tables */

static const struct pact_def {
	const char *key, *name;
	xchar ranks;
	short bonus;		/* percent per rank */
	short feat;		/* the feat that opens it, or -1 */
	const char *desc[3];
} pacts[NUM_PACTS] = {
	{ "famine", "Famine", 3, 5, -1,
	  { "hunger 25% faster", "hunger 50% faster", "hunger 75% faster" } },
	{ "horde", "Horde", 3, 8, -1,
	  { "monsters appear 25% more often", "monsters appear 50% more often",
	    "monsters appear 75% more often" } },
	{ "darkness", "Darkness", 2, 5, -1,
	  { "half the lit rooms are dark", "no room is lit", 0 } },
	{ "lonely", "Lonely", 1, 5, -1,
	  { "no starting pet", 0, 0 } },
	{ "dear", "Dear prices", 1, 4, -1,
	  { "shops charge a third more", 0, 0 } },
	{ "hexed", "Hexed", 2, 6, F_ACT1,
	  { "more items come cursed", "far more items come cursed", 0 } },
	{ "fragile", "Fragile", 2, 8, F_ACT1,
	  { "15% fewer HP per level", "30% fewer HP per level", 0 } },
	{ "swift", "Swift foes", 2, 8, F_ACT2,
	  { "hostile monsters 2 faster", "hostile monsters 4 faster", 0 } },
	{ "unwritten", "Unwritten", 1, 10, F_ACT2,
	  { "Elbereth has no effect", 0, 0 } },
	{ "elite", "Elite", 2, 10, F_QPORTAL,
	  { "monsters as if 2 levels deeper", "monsters as if 4 levels deeper",
	    0 } },
	{ "silent", "Silent gods", 2, 8, F_MEDUSA,
	  { "prayer wait 50% longer", "prayer wait doubled", 0 } },
	{ "starless", "Starless", 1, 10, F_ACT3,
	  { "no fate drafts", 0, 0 } },
};

static const struct act_def {
	const char *name, *goal;
	short bonus;
	short mult;		/* percent, applied to the whole pot */
} acts[NUM_ACTS] = {
	{ "The Delve", "pick up the luckstone at Mines' End", 50, 100 },
	{ "The Gorgon", "kill Medusa", 150, 125 },
	{ "The Siege", "take the wand of wishing from the Castle", 300, 150 },
};

static const char *const roman[] = { "", "I", "II", "III" };
static const char *const act_roman[] = { "", "I", "II", "III" };
static const char *const months[] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct",
	"Nov", "Dec"
};

STATIC_DCL void FDECL(add_text, (winid, const char *));
STATIC_DCL void FDECL(add_choice, (winid, int, CHAR_P, const char *));
STATIC_DCL boolean FDECL(pact_open, (int));
STATIC_DCL long FDECL(day_number, (long));
STATIC_DCL long FDECL(day_date, (long));
STATIC_DCL void FDECL(date_text, (long, char *));
STATIC_DCL long NDECL(act_pot);
STATIC_DCL void NDECL(act_choice);

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
add_choice(win, val, accel, str)
winid win;
int val;
char accel;
const char *str;
{
	anything any;

	any.a_void = 0;
	any.a_int = val;
	add_menu(win, NO_GLYPH, &any, accel, 0, ATR_NONE, str, MENU_UNSELECTED);
}

const char *
legacy_mode_name(mode)
int mode;
{
	switch (mode) {
	case LMODE_LEGACY:	return "Legacy";
	case LMODE_EXPEDITION:	return "Expedition";
	case LMODE_DAILY:	return "Daily";
	}
	return "Classic";
}

/* ------------------------------------------------------------------ */
/* the profile */

boolean
modes_read(key, rest)
const char *key;
char *rest;
{
	int i;

	if (!strcmp(key, "pact")) {
	    char pname[BUFSZ];
	    int r = 0;

	    if (sscanf(rest, "%s %d", pname, &r) == 2)
		for (i = 0; i < NUM_PACTS; i++)
		    if (!strcmp(pname, pacts[i].key))
			lprof.pacts[i] = (xchar) max(0, min(r, pacts[i].ranks));
	    return TRUE;
	}
	if (!strcmp(key, "daily")) {
	    (void) sscanf(rest, "%ld %d %d", &lprof.daily_last,
			  &lprof.daily_streak, &lprof.daily_best);
	    return TRUE;
	}
	return FALSE;
}

void
modes_write(fp)
FILE *fp;
{
	int i;

	for (i = 0; i < NUM_PACTS; i++)
	    if (lprof.pacts[i])
		fprintf(fp, "pact %s %d\n", pacts[i].key, lprof.pacts[i]);
	if (lprof.daily_last)
	    fprintf(fp, "daily %ld %d %d\n", lprof.daily_last,
		    lprof.daily_streak, lprof.daily_best);
}

/* ------------------------------------------------------------------ */
/* pacts */

/* the rank of pact p in this game */
int
pact_rank(p)
int p;
{
	return LEGACY_RULES() ? lrun.pact[p] : 0;
}

int
pact_level()
{
	int p, n = 0;

	for (p = 0; p < NUM_PACTS; p++)
	    n += pact_rank(p);
	return n;
}

/* the Renown bonus, in percent */
int
pact_bonus()
{
	int p, n = 0;

	for (p = 0; p < NUM_PACTS; p++)
	    n += pact_rank(p) * pacts[p].bonus;
	return n;
}

STATIC_OVL boolean
pact_open(p)
int p;
{
	return (boolean) (pacts[p].feat < 0 || lprof.feat_done[pacts[p].feat]);
}

/* "Famine I and Darkness II (+15%)", or "none" */
void
pact_text(buf, prepare)
char *buf;
boolean prepare;	/* the profile's choice for the next game */
{
	int p, r, pct = 0;

	*buf = '\0';
	for (p = 0; p < NUM_PACTS; p++) {
	    r = prepare ? lprof.pacts[p] : lrun.pact[p];
	    if (!r)
		continue;
	    Sprintf(eos(buf), "%s%s", *buf ? ", " : "", pacts[p].name);
	    if (pacts[p].ranks > 1)
		Sprintf(eos(buf), " %s", roman[r]);
	    pct += r * pacts[p].bonus;
	}
	if (*buf)
	    Sprintf(eos(buf), " (+%d%%)", pct);
	else
	    Strcpy(buf, "none");
}

/* the Prepare screen's pact list: each pick raises a pact one rank,
   and past its top rank it goes back to none */
void
pact_menu()
{
	winid win;
	menu_item *pick = 0;
	char buf[BUFSZ];
	int p, n, r, lvl, pct;

	for (;;) {
	    for (lvl = pct = p = 0; p < NUM_PACTS; p++) {
		lvl += lprof.pacts[p];
		pct += lprof.pacts[p] * pacts[p].bonus;
	    }
	    win = create_nhwindow(NHW_MENU);
	    start_menu(win);
	    add_text(win, "Pacts make the game harder for more Renown.");
	    add_text(win, "Pick one to raise it a rank; past the top rank it");
	    add_text(win, "goes back to none. The multiplier is capped at x3.");
	    Sprintf(buf, "Pact level %d: +%d%% Renown.", lvl, pct);
	    add_text(win, buf);
	    add_text(win, "");
	    for (p = 0; p < NUM_PACTS; p++) {
		r = lprof.pacts[p];
		if (!pact_open(p)) {
		    Sprintf(buf, "    %s, locked: %s", pacts[p].name,
			    legacy_feat_name(pacts[p].feat));
		    add_text(win, buf);
		    continue;
		}
		if (r)
		    Sprintf(buf, "[%s] %s +%d%%: %s", roman[r], pacts[p].name,
			    r * pacts[p].bonus, pacts[p].desc[r - 1]);
		else
		    Sprintf(buf, "[ ] %s, +%d%% a rank (%d): %s", pacts[p].name,
			    pacts[p].bonus, pacts[p].ranks, pacts[p].desc[0]);
		add_choice(win, p + 1, 'a' + p, buf);
	    }
	    end_menu(win, "Pacts");
	    n = select_menu(win, PICK_ONE, &pick);
	    destroy_nhwindow(win);
	    if (n <= 0)
		break;
	    p = pick[0].item.a_int - 1;
	    free((genericptr_t) pick), pick = 0;
	    lprof.pacts[p] = (lprof.pacts[p] + 1) % (pacts[p].ranks + 1);
	    lprof.dirty = TRUE;
	}
	legacy_write(FALSE);
}

/* the four hard vows open with "Keep a vow through Act I" */
boolean
vow_hard_open(v)
int v;
{
	return (boolean) (lprof.feat_done[F_VOWACT1] != 0);
}

/* the pact effects, called where the game computes each number */

/* Horde: 1 in 'base' becomes more often */
int
legacy_spawn_rate(base)
int base;
{
	int r = pact_rank(PACT_HORDE);

	return r ? max(1, base * 4 / (4 + r)) : base;
}

/* Darkness: should a room that would be lit stay dark? */
boolean
legacy_dark_room(lit)
boolean lit;
{
	int r = pact_rank(PACT_DARKNESS);

	if (!lit || !r)
	    return lit;
	return (boolean) (r == 1 && rn2(2));
}

/* Fragile: hit points from a new level */
int
legacy_hp_gain(num)
int num;
{
	int r = pact_rank(PACT_FRAGILE);

	if (r && num > 1)
	    num = max(1, num - (num * 15 * r + 50) / 100);
	return num;
}

/* Dear prices */
long
legacy_shop_price(tmp)
long tmp;
{
	if (pact_rank(PACT_DEAR))
	    tmp += tmp / 3L;
	return tmp;
}

/* Hexed: an object made after the starting kit may be cursed */
void
legacy_hex(otmp)
struct obj *otmp;
{
	int r = pact_rank(PACT_HEXED);

	if (!r || !lrun.inv_made || otmp->cursed || otmp->oartifact)
	    return;
	switch (otmp->oclass) {
	case WEAPON_CLASS:
	case ARMOR_CLASS:
	case RING_CLASS:
	case AMULET_CLASS:
	case WAND_CLASS:
	case TOOL_CLASS:
	case POTION_CLASS:
	case SCROLL_CLASS:
	    if (rn2(100) < 15 * r) {
		otmp->blessed = 0;
		otmp->cursed = 1;
	    }
	    break;
	}
}

/* ------------------------------------------------------------------ */
/* seeded levels: a second generator, used only while a level is built */

static boolean seed_on = FALSE;
static unsigned long long seed_state = 0;

/* every random number in the game comes through here (rnd.c) */
long
legacy_rand()
{
	if (!seed_on)
	    return (long) Rand();
	/* xorshift64 */
	seed_state ^= seed_state << 13;
	seed_state ^= seed_state >> 7;
	seed_state ^= seed_state << 17;
	return (long) ((seed_state >> 33) & 0x7fffffffUL);
}

/* use the seeded generator, from a seed built from the run's seed and
   (a, b): a level, or a step of setting up the game */
void
legacy_seed_push(a, b)
long a, b;
{
	unsigned long long z;

	if (!lrun.seed)
	    return;
	/* splitmix64 */
	z = (unsigned long long) lrun.seed * 0x9E3779B97F4A7C15ULL
	    + (unsigned long long) (a + 1000) * 0xBF58476D1CE4E5B9ULL
	    + (unsigned long long) (b + 1000) * 0x94D049BB133111EBULL;
	z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
	z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
	z ^= z >> 31;
	seed_state = z ? z : 0x2545F4914F6CDD1DULL;
	seed_on = TRUE;
}

void
legacy_seed_pop()
{
	seed_on = FALSE;
}

/* a seed from typed text; a plain number is used as it is.  Seeds are
   kept to 32 bits so they are the same on every device */
unsigned long
legacy_hash_seed(s)
const char *s;
{
	unsigned long long h = 0;
	const char *p;

	for (p = s; *p && digit(*p) && h <= 0xffffffffULL; p++)
	    h = h * 10 + (*p - '0');
	if (!*p && p != s && h && h <= 0xffffffffULL)
	    return (unsigned long) h;
	h = 5381;
	for (p = s; *p; p++)
	    h = (h * 33 + (unsigned char) lowc(*p)) & 0xffffffffULL;
	return h ? (unsigned long) h : 1UL;
}

/* ------------------------------------------------------------------ */
/* dates */

/* days since 1970-01-01 for a yyyymmdd date */
STATIC_OVL long
day_number(date)
long date;
{
	long y = date / 10000, m = (date / 100) % 100, d = date % 100;
	long era, yoe, doy, doe;

	y -= (m <= 2);
	era = (y >= 0 ? y : y - 399) / 400;
	yoe = y - era * 400;
	doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
	doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
	return era * 146097 + doe - 719468;
}

/* and back */
STATIC_OVL long
day_date(z)
long z;
{
	long era, doe, yoe, y, doy, mp, d, m;

	z += 719468;
	era = (z >= 0 ? z : z - 146096) / 146097;
	doe = z - era * 146097;
	yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
	y = yoe + era * 400;
	doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
	mp = (5 * doy + 2) / 153;
	d = doy - (153 * mp + 2) / 5 + 1;
	m = mp + (mp < 10 ? 3 : -9);
	return (y + (m <= 2)) * 10000 + m * 100 + d;
}

STATIC_OVL void
date_text(date, buf)
long date;
char *buf;
{
	int m = (int) ((date / 100) % 100);

	Sprintf(buf, "%s %ld", (m >= 1 && m <= 12) ? months[m - 1] : "?",
		date % 100);
}

/* ------------------------------------------------------------------ */
/* the run */

/* called by legacy_setup_run() for Legacy and Expedition games */
void
modes_setup_run()
{
	int p;

	for (p = 0; p < NUM_PACTS; p++)
	    if (pact_open(p))
		lrun.pact[p] = lprof.pacts[p];
}

/* the Daily: today's seed picks the character, kit, blessing and the
   week's pacts.  With 'ask', show it and let the player go back */
boolean
daily_prepare(ask)
boolean ask;
{
	static const char *const days[] = { "Sunday", "Monday", "Tuesday",
		"Wednesday", "Thursday", "Friday", "Saturday" };
	winid win;
	menu_item *pick = 0;
	char buf[BUFSZ], desc[BUFSZ], date[BUFSZ];
	long today = yyyymmdd((time_t) 0), dn = day_number(today);
	int h[4], tries, i, n, p, npacts, nroles, dow = (int) ((dn + 4) % 7);
	boolean counts = (lprof.daily_last != today);
	int bless, kit;

	lrun.mode = LMODE_DAILY;
	/* kept to 32 bits, so every device gets the same seed */
	lrun.seed = (unsigned long) ((legacy_hash_seed("daily")
			^ (unsigned long long) today * 2654435761ULL) & 0xffffffffULL);
	if (!lrun.seed)
	    lrun.seed = 1;
	lrun.custom = FALSE;
	legacy_selecting = FALSE;

	/* the character */
	for (nroles = 0; roles[nroles].name.m; nroles++)
	    ;
	legacy_seed_push(-10L, 0L);
	for (tries = 0; tries < 5000; tries++) {
	    h[0] = rn2(nroles);
	    for (n = 0; races[n].noun; n++)
		;
	    h[1] = rn2(n);
	    h[2] = rn2(ROLE_GENDERS);
	    h[3] = rn2(ROLE_ALIGNS);
	    if (ok_role(h[0], h[1], h[2], h[3])
		    && ok_race(h[0], h[1], h[2], h[3])
		    && ok_gend(h[0], h[1], h[2], h[3])
		    && ok_align(h[0], h[1], h[2], h[3]))
		break;
	}
	kit = rn2(roles[h[0]].malenum == PM_UNDEAD_SLAYER ? 4 : 3);
	do {
	    bless = rn2(NUM_BLESSINGS);
	} while (bless == BL_HEIRSLUCK);
	legacy_seed_pop();
	flags.initrole = h[0];
	flags.initrace = h[1];
	flags.initgend = h[2];
	flags.initalign = h[3];

	legacy_setup_run();	/* keeps the mode and seed */
	lrun.daily_date = today;
	lrun.daily_counts = counts;
	lrun.gauntlet = (dow == 0);
	lrun.kit = kit;
	lrun.blessings = 1L << bless;
	/* the week's pacts; Sunday adds two more */
	legacy_seed_push(-11L, (dn + 3) / 7);
	npacts = lrun.gauntlet ? 4 : 2;
	for (i = 0; i < npacts; ) {
	    p = rn2(NUM_PACTS);
	    if (p == PACT_STARLESS || lrun.pact[p])
		continue;
	    lrun.pact[p] = 1;
	    i++;
	}
	legacy_seed_pop();

	if (!ask)
	    return TRUE;
	date_text(today, date);
	legacy_hero_desc(desc, h[0], h[1], h[2], h[3]);
	win = create_nhwindow(NHW_MENU);
	start_menu(win);
	Sprintf(buf, "The Daily for %s %s%s", days[dow], date,
		lrun.gauntlet ? ": the Sunday Gauntlet" : "");
	add_text(win, buf);
	add_text(win, "Everyone gets the same character and the same maps.");
	add_text(win, "Lineage and heirlooms are off; Insight stays on.");
	add_text(win, "");
	Sprintf(buf, "Character: %s, kit %c", desc,
		kit == KIT_A ? 'A' : kit == KIT_B ? 'B' : 'C');
	add_text(win, buf);
	Sprintf(buf, "Blessing: %s", fate_bless_name(bless));
	add_text(win, buf);
	pact_text(buf, FALSE);
	Sprintf(desc, "Pacts: %s", buf);
	add_text(win, desc);
	add_text(win, "");
	if (counts) {
	    long streak = (lprof.daily_last
			   && day_number(lprof.daily_last) == dn - 1)
			  ? lprof.daily_streak + 1 : 1;

	    Sprintf(buf, "This is today's attempt: it pays %ld Renown and makes your streak %ld.",
		    (20L + min(5L * streak, 50L)) * (lrun.gauntlet ? 2 : 1),
		    streak);
	} else
	    Strcpy(buf, "You have played today's Daily; this one is practice.");
	add_text(win, buf);
	if (lprof.daily_streak) {
	    Sprintf(buf, "Streak: %d day%s. Deepest Daily: level %d.",
		    lprof.daily_streak, plur(lprof.daily_streak),
		    lprof.daily_best);
	    add_text(win, buf);
	}
	add_text(win, "");
	add_choice(win, 1, 'a', "Start the Daily");
	add_choice(win, 2, '<', "Back to the Hall");
	end_menu(win, "The Daily");
	n = select_menu(win, PICK_ONE, &pick);
	destroy_nhwindow(win);
	i = (n > 0) ? pick[0].item.a_int : 2;
	if (pick)
	    free((genericptr_t) pick);
	return (boolean) (i == 1);
}

/* called from legacy_newgame() */
void
modes_newgame()
{
	long today, dn;

	if (lrun.mode == LMODE_DAILY && lrun.daily_counts) {
	    /* the attempt counts as soon as it starts */
	    today = lrun.daily_date;
	    dn = day_number(today);
	    if (lprof.daily_last && day_number(lprof.daily_last) == dn - 1)
		lprof.daily_streak++;
	    else
		lprof.daily_streak = 1;
	    lprof.daily_last = today;
	    lprof.daily_best = max(lprof.daily_best, 1);
	    lprof.dirty = TRUE;
	    if (lprof.daily_streak >= 7)
		legacy_feat(F_STREAK7);
	    if (lprof.daily_streak >= 30)
		legacy_feat(F_STREAK30);
	}
}

/* the pot after clearing act n: every bonus so far, times act n's
   multiplier */
STATIC_OVL long
act_pot()
{
	long sum = 0;
	int i;

	for (i = 0; i < lrun.act; i++)
	    sum += acts[i].bonus;
	return lrun.act ? sum * acts[lrun.act - 1].mult / 100 : 0L;
}

/* act n (1..3) of an Expedition is cleared; the choice waits for the
   end of the turn */
void
modes_act_cleared(n)
int n;
{
	int v;

	if (lrun.mode != LMODE_EXPEDITION || lrun.act != n - 1)
	    return;
	lrun.act = n;
	lrun.pot = act_pot();
	lrun.act_pending = n;
	draft_act(n);		/* one more fate draft */
	legacy_feat(F_ACT1 + n - 1);
	if (n == 1) {
	    if (pact_level() >= 5)
		legacy_feat(F_ACT1PACT5);
	    if (lrun.unaided)
		legacy_feat(F_ACT1UNAIDED);
	    for (v = 0; v < NUM_VOWS; v++)
		if ((lrun.vows & (1 << v)) && !(lrun.vows_broken & (1 << v)))
		    legacy_feat(F_VOWACT1);
	}
}

/* retire, or press on */
STATIC_OVL void
act_choice()
{
	winid win;
	menu_item *pick = 0;
	char buf[BUFSZ];
	int n = lrun.act_pending, choice;

	lrun.act_pending = 0;
	win = create_nhwindow(NHW_MENU);
	start_menu(win);
	Sprintf(buf, "Act %s, %s, is cleared! The pot holds %ld Renown.",
		act_roman[n], acts[n - 1].name, lrun.pot);
	add_text(win, buf);
	add_text(win, "");
	Sprintf(buf, "Retire: end the game as a victory and bank all %ld",
		lrun.pot);
	add_choice(win, 1, 'a', buf);
	if (n < NUM_ACTS) {
	    long next = 0;
	    int i;

	    for (i = 0; i <= n; i++)
		next += acts[i].bonus;
	    next = next * acts[n].mult / 100;
	    Sprintf(buf, "Press on to Act %s, %s: %s (the pot becomes %ld;",
		    act_roman[n + 1], acts[n].name, acts[n].goal, next);
	    add_choice(win, 2, 'b', buf);
	    Sprintf(buf, "    die first and you keep half, %ld)", lrun.pot / 2);
	    add_text(win, buf);
	} else
	    add_choice(win, 2, 'b',
		       "Press on toward ascension; the pot is banked when the game ends");
	end_menu(win, "Retire or press on?");
	n = select_menu(win, PICK_ONE, &pick);
	destroy_nhwindow(win);
	choice = (n > 0) ? pick[0].item.a_int : 2;
	if (pick)
	    free((genericptr_t) pick);
	if (choice == 1) {
	    if (yn("Really retire and end this game?") == 'y') {
		lrun.retired = TRUE;
		killer = "retired";
		killer_format = NO_KILLER_PREFIX;
		done(ESCAPED);
		return;
	    }
	}
	if (lrun.act < NUM_ACTS)
	    pline("Act %s, %s: %s.", act_roman[lrun.act + 1],
		  acts[lrun.act].name, acts[lrun.act].goal);
}

/* once per turn */
void
modes_turn()
{
	if (lrun.act_pending && !program_state.gameover)
	    act_choice();
}

/* the hero arrived on a new level at depth dep */
void
modes_arrived(dep)
int dep;
{
	if (lrun.mode == LMODE_DAILY) {
	    if (lrun.daily_counts && dep > lprof.daily_best) {
		lprof.daily_best = dep;
		lprof.dirty = TRUE;
	    }
	    if (lrun.gauntlet && lrun.daily_counts && dep >= 10)
		legacy_feat(F_GAUNTLET10);
	}
}

/* the welcome: what this mode asks */
void
modes_welcome()
{
	char buf[BUFSZ];

	if (lrun.mode == LMODE_EXPEDITION)
	    pline("Expedition, Act I, %s: %s.", acts[0].name, acts[0].goal);
	if (lrun.mode == LMODE_DAILY)
	    pline("Today's Daily%s%s.", lrun.gauntlet ? " (the Gauntlet)" : "",
		  lrun.daily_counts ? "" : ", for practice");
	if (lrun.custom)
	    pline("A custom seed: no Renown or feats, but the Codex learns.");
	if (pact_level()) {
	    pact_text(buf, FALSE);
	    pline("Pacts: %s.", buf);
	}
}

/* Renown from the mode, worked out by legacy_game_over() */
void
modes_game_over(how)
int how;
{
	lres.pact_pct = pact_bonus();
	if (lrun.mode == LMODE_EXPEDITION && lrun.act) {
	    lres.pot_pts = (lrun.retired || lrun.act >= NUM_ACTS)
			   ? lrun.pot : lrun.pot / 2;
	    if (lrun.retired && lrun.pot >= 250)
		legacy_feat(F_RETIRE250);
	}
	if (lrun.mode == LMODE_DAILY && lrun.daily_counts)
	    lres.daily_pts = (20L + min(5L * lprof.daily_streak, 50L))
			     * (lrun.gauntlet ? 2 : 1);
}

/* "Daily Oct 14: dwarven Valkyrie, level 9, killed by a soldier ant on
   turn 4,812" */
void
daily_line(buf)
char *buf;
{
	char date[BUFSZ], turns[BUFSZ], *p;
	long t = moves;
	int n;

	date_text(lrun.daily_date, date);
	/* the turn with thousands separated */
	Sprintf(turns, "%ld", t);
	n = (int) strlen(turns);
	if (n > 3) {
	    char tmp[BUFSZ];

	    Strcpy(tmp, turns);
	    p = turns;
	    for (n = 0; tmp[n]; n++) {
		*p++ = tmp[n];
		if (tmp[n + 1] && ((int) strlen(tmp) - n - 1) % 3 == 0)
		    *p++ = ',';
	    }
	    *p = '\0';
	}
	Sprintf(buf, "Daily %s: %s %s, level %d, %s on turn %s", date,
		urace.adj, (flags.female && urole.name.f) ? urole.name.f
		: urole.name.m, depth(&u.uz), lres.death, turns);
}

/*modes.c*/
