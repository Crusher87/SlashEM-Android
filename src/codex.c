/*	SCCS Id: @(#)codex.c	0.0.7E7F3	2026/10/13	*/
/* NetHack may be freely redistributed.  See license for details. */

/*
 * Slash'EM Legacy: the Codex, a record of what the player has learned.
 *
 * Four volumes fill in as the player plays, in any mode:
 *   the Bestiary	monsters seen, and how many were killed;
 *   the Armory		object types formally identified;
 *   the Atlas		special levels and branches entered, each with a tip;
 *   Lessons		causes of death met, each with a tip.
 * Nothing appears that the player hasn't met.  The Codex lives in the
 * profile (legacy.dat); monsters and objects are stored by name, so new
 * monsters or objects can't scramble it.
 */

#include "hack.h"

extern const int monstr[];

#define WHERE_SZ	48

struct cx_mon {
	long kills;
	boolean seen;
	char where[WHERE_SZ];	/* where it was first seen */
};

static struct cx_mon *bestiary = 0;	/* NUMMONS entries */
static boolean *armory = 0;		/* NUM_OBJECTS entries */

/* ------------------------------------------------------------------ */
/* the Atlas */

#define PL_SHOP		0
#define PL_TEMPLE	1
#define PL_SOKOBAN	2

static const struct place {
	const char *key;	/* special level, dungeon name or tag */
	const char *name;
	const char *tip;
} places[] = {
	{ "shop", "A shop",
	  "Pay before you leave. Never dig, teleport or zap your way out with unpaid items; drop them or pay first. Prices quoted here can help identify things." },
	{ "temple", "A temple",
	  "A priest tends the altar. Donating gold can buy protection, and a coaligned priest's temple is safe to pray in. Never attack the priest." },
	{ "Sokoban", "Sokoban",
	  "Boulders can't be pushed diagonally, and you can't squeeze diagonally between boulders. Breaking boulders or squeezing past them costs Luck. Fill every hole to finish a level." },
	{ "The Gnomish Mines", "The Gnomish Mines",
	  "Dwarves carry picks and mattocks that hit hard. If you are a gnome or dwarf, most residents are peaceful. The Mines are dark; carry a light if you can." },
	{ "minetn", "Minetown",
	  "The temple priest grants protection for a donation of 400 gold times your experience level. Check the altar's alignment before you sacrifice. Don't anger the watch." },
	{ "minend", "Mines' End",
	  "A luckstone is hidden somewhere on this level. Gray stones can be loadstones that you can't drop: kick one before you pick it up." },
	{ "oracle", "The Oracle",
	  "A minor consultation is cheap and counts for a feat. The centaur statues are only statues, and the Oracle is peaceful and tough: leave her be." },
	{ "bigrm", "The Big Room",
	  "Wide open with many monsters. Stay near the stairs, or engrave Elbereth before resting." },
	{ "rats", "A rat nest",
	  "Rats swarm here. Fight in a corridor, and kill rabid rats before they drain your Constitution." },
	{ "kobold", "A kobold lair",
	  "Kobolds are weak alone but come in numbers, and their corpses are poisonous to eat." },
	{ "nymph", "A nymph grove",
	  "Nymphs steal an item and teleport away. Kill them from a distance, or carry fewer valuables in the open." },
	{ "mall", "A town of shops",
	  "Many shops in one place. Pay before you leave each one, and don't anger the watch." },
	{ "Grund's Stronghold", "Grund's Stronghold",
	  "An orc fortress. Expect large groups; doorways are your friend." },
	{ "qportal", "The Quest portal",
	  "The magic portal to your Quest is somewhere on this level. Your leader will only send you on once you reach experience level 14." },
	{ "-strt", "Your Quest home",
	  "Your leader won't let you go on below experience level 14. Never anger them, and keep your alignment record good." },
	{ "-goal", "Your Quest nemesis",
	  "Your nemesis guards the Quest artifact here. Come prepared, with escape items ready." },
	{ "The Wyrm Caves", "The Wyrm Caves",
	  "Dragons live here. Reflection protects you from their breath; fight them one at a time." },
	{ "One-eyed Sam's Market", "One-eyed Sam's Market",
	  "Prices here are many times higher, and One-eyed Sam is deadly if angered. Never steal." },
	{ "The Lost Tomb", "The Lost Tomb",
	  "Mummies and other undead guard this tomb. Expect curses on the treasure." },
	{ "The Spider Caves", "The Spider Caves",
	  "Giant spiders are fast and poisonous. Poison resistance and a corridor help." },
	{ "The Sunless Sea", "The Sunless Sea",
	  "Most of this level is water with sea monsters that can drown you. Keep away from the edges unless you can breathe water." },
	{ "The Temple of Moloch", "The Temple of Moloch",
	  "A temple of Moloch with hostile priests. Don't pray or sacrifice here." },
	{ "The Giant Caverns", "The Giant Caverns",
	  "Giants roam here and throw boulders. Eating a giant's corpse can raise your Strength." },
	{ "Fort Ludios", "Fort Ludios",
	  "Soldiers guard a great treasure. Fight them from a doorway, a few at a time; Elbereth doesn't scare soldiers." },
	{ "medusa", "Medusa's island",
	  "Her gaze turns you to stone. Reflection or a blindfold protects you; cross the water with levitation or by other means." },
	{ "castle", "The Castle",
	  "The drawbridge opens to a five-note tune: play notes near it and the gears and tumblers tell you how close you are. A wand of wishing waits in one of the corner towers. Trapdoors beyond the castle lead to Gehennom." },
	{ "valley", "The Valley of the Dead",
	  "You can't pray for help anywhere in Gehennom. The temple here belongs to Moloch, and undead are everywhere." },
	{ "juiblex", "Juiblex's swamp",
	  "Juiblex can engulf and sicken you. Much of the level is water." },
	{ "orcus", "Orcus Town",
	  "A ruined town full of undead. Orcus carries a wand of death; magic resistance protects you from it." },
	{ "lair", "A demon lord's lair",
	  "A demon lord rules here. Bring magic resistance and reflection, and don't linger." },
	{ "wizard1", "The Wizard's Tower",
	  "The Wizard of Yendor holds the Book of the Dead. Expect him to come back for you again and again." },
	{ "Vlad's Tower", "Vlad's Tower",
	  "Vlad the Impaler waits at the top with the Candelabrum of Invocation. Vampires here drain levels." },
	{ "Frankenstein's Lab", "Frankenstein's Lab",
	  "The doctor and his creations wait here. Bring ranged attacks." },
	{ "sanctum", "Moloch's Sanctum",
	  "The high priest of Moloch holds the Amulet. You can only get here by performing the invocation." },
	{ "earth", "The Plane of Earth",
	  "Dig through the rock to find the magic portal. Earth elementals move through stone." },
	{ "air", "The Plane of Air",
	  "Clouds block your sight and air elementals engulf you quickly. Speed helps." },
	{ "fire", "The Plane of Fire",
	  "Fire resistance is essential here." },
	{ "water", "The Plane of Water",
	  "Water everywhere, travelling in air bubbles. Magical breathing helps." },
	{ "astral", "The Astral Plane",
	  "Find your own god's altar among the three temples and offer the Amulet there." },
	{ 0, 0, 0 }
};

#define NUM_PLACES (SIZE(places) - 1)
static boolean atlas[SIZE(places)];

/* Sokoban memory: each layout got through, with the pushes that did it */
#define MAX_SOKOMEM	40
struct soko_mem {
	char name[12];		/* the layout, such as "soko4-2" */
	int quality;		/* SQ_xxx */
	int nmap, nsteps;
	char *map[SOKO_ROWS];
	char **steps;
};
static struct soko_mem sokomem[MAX_SOKOMEM];
static int nsokomem = 0;

STATIC_DCL char *FDECL(cx_strdup, (const char *));
STATIC_DCL void FDECL(soko_forget, (struct soko_mem *));
STATIC_DCL struct soko_mem *FDECL(soko_find, (const char *));
STATIC_DCL void FDECL(soko_add_step, (struct soko_mem *, const char *));

/* ------------------------------------------------------------------ */
/* the profile */

void
codex_init()
{
	int i;

	if (!bestiary) {
	    bestiary = (struct cx_mon *)
		alloc((unsigned) (NUMMONS * sizeof (struct cx_mon)));
	    armory = (boolean *) alloc((unsigned) (NUM_OBJECTS * sizeof (boolean)));
	}
	(void) memset((genericptr_t) bestiary, 0,
		      NUMMONS * sizeof (struct cx_mon));
	(void) memset((genericptr_t) armory, 0, NUM_OBJECTS * sizeof (boolean));
	/* The Hall reads the profile before init_objects() has run, when
	   OBJ_NAME() doesn't work yet.  Names are never shuffled (only
	   descriptions are), so this is what init_objects() will set. */
	for (i = 0; i < NUM_OBJECTS; i++)
	    objects[i].oc_name_idx = i;
	(void) memset((genericptr_t) atlas, 0, sizeof atlas);
	while (nsokomem > 0)
	    soko_forget(&sokomem[--nsokomem]);
}

STATIC_OVL char *
cx_strdup(s)
const char *s;
{
	char *t = (char *) alloc((unsigned) strlen(s) + 1);

	Strcpy(t, s);
	return t;
}

STATIC_OVL void
soko_forget(m)
struct soko_mem *m;
{
	int i;

	for (i = 0; i < m->nmap; i++)
	    free((genericptr_t) m->map[i]);
	for (i = 0; i < m->nsteps; i++)
	    free((genericptr_t) m->steps[i]);
	if (m->steps)
	    free((genericptr_t) m->steps);
	(void) memset((genericptr_t) m, 0, sizeof *m);
}

STATIC_OVL struct soko_mem *
soko_find(name)
const char *name;
{
	int i;

	for (i = 0; i < nsokomem; i++)
	    if (!strcmp(sokomem[i].name, name))
		return &sokomem[i];
	return (struct soko_mem *) 0;
}

STATIC_OVL void
soko_add_step(m, text)
struct soko_mem *m;
const char *text;
{
	/* grow by 32 at a time */
	if (!(m->nsteps % 32)) {
	    char **n = (char **) alloc((unsigned) ((m->nsteps + 32)
						   * sizeof (char *)));

	    if (m->nsteps)
		(void) memcpy((genericptr_t) n, (genericptr_t) m->steps,
			      m->nsteps * sizeof (char *));
	    if (m->steps)
		free((genericptr_t) m->steps);
	    m->steps = n;
	}
	m->steps[m->nsteps++] = cx_strdup(text);
}

STATIC_OVL char *
tab_field(p)
char **p;
{
	char *s = *p, *t;

	if (!s)
	    return "";
	if ((t = index(s, '\t')) != 0) {
	    *t = '\0';
	    *p = t + 1;
	} else
	    *p = 0;
	return s;
}

/* a profile line legacy.c doesn't know; TRUE if it was ours */
boolean
codex_read(key, rest)
const char *key;
char *rest;
{
	int i;

	if (!strcmp(key, "mon")) {
	    /* mon kills<TAB>symbol<TAB>where<TAB>name */
	    char *p = rest, *name, *where, *sym;
	    long kills = atol(tab_field(&p));

	    sym = tab_field(&p);
	    where = tab_field(&p);
	    name = tab_field(&p);
	    for (i = LOW_PM; i < NUMMONS; i++)
		if (!strcmp(mons[i].mname, name)
			&& def_monsyms[(int) mons[i].mlet] == *sym) {
		    bestiary[i].seen = TRUE;
		    bestiary[i].kills = kills;
		    legacy_clean_field(bestiary[i].where, where, WHERE_SZ);
		    break;
		}
	    return TRUE;
	}
	if (!strcmp(key, "obj")) {
	    /* obj class<TAB>name */
	    char *p = rest, *name;
	    int oc = atoi(tab_field(&p));

	    name = tab_field(&p);
	    for (i = 0; i < NUM_OBJECTS; i++)
		if (objects[i].oc_class == oc && OBJ_NAME(objects[i])
			&& !strcmp(OBJ_NAME(objects[i]), name)) {
		    armory[i] = TRUE;
		    break;
		}
	    return TRUE;
	}
	if (!strcmp(key, "place")) {
	    for (i = 0; i < NUM_PLACES; i++)
		if (!strcmp(places[i].key, rest))
		    atlas[i] = TRUE;
	    return TRUE;
	}
	if (!strcmp(key, "sokoban")) {	/* "sokoban soko4-2[\tpartial]" */
	    char *name = tab_field(&rest);
	    struct soko_mem *m;

	    if (!soko_find(name) && nsokomem < MAX_SOKOMEM && *name) {
		m = &sokomem[nsokomem++];
		(void) strncpy(m->name, name, sizeof m->name - 1);
		m->quality = !rest ? SQ_SOLVED
			     : !strcmp(rest, "partial") ? SQ_PARTIAL : SQ_LOST;
	    }
	    return TRUE;
	}
	if (!strcmp(key, "sokomap") || !strcmp(key, "sokostep")) {
	    char *name = tab_field(&rest);
	    struct soko_mem *m = soko_find(name);

	    if (!rest)
		return TRUE;
	    if (m && key[4] == 'm') {
		if (m->nmap < SOKO_ROWS)
		    m->map[m->nmap++] = cx_strdup(rest);
	    } else if (m)
		soko_add_step(m, rest);
	    return TRUE;
	}
	return FALSE;
}

void
codex_write(fp)
FILE *fp;
{
	int i;

	for (i = LOW_PM; i < NUMMONS; i++)
	    if (bestiary[i].seen)
		fprintf(fp, "mon %ld\t%c\t%s\t%s\n", bestiary[i].kills,
			def_monsyms[(int) mons[i].mlet], bestiary[i].where,
			mons[i].mname);
	for (i = 0; i < NUM_OBJECTS; i++)
	    if (armory[i])
		fprintf(fp, "obj %d\t%s\n", (int) objects[i].oc_class,
			OBJ_NAME(objects[i]));
	for (i = 0; i < NUM_PLACES; i++)
	    if (atlas[i])
		fprintf(fp, "place %s\n", places[i].key);
	for (i = 0; i < nsokomem; i++) {
	    struct soko_mem *m = &sokomem[i];
	    int j;

	    fprintf(fp, "sokoban %s%s\n", m->name,
		    m->quality == SQ_SOLVED ? ""
		    : m->quality == SQ_PARTIAL ? "\tpartial" : "\tlost");
	    for (j = 0; j < m->nmap; j++)
		fprintf(fp, "sokomap %s\t%s\n", m->name, m->map[j]);
	    for (j = 0; j < m->nsteps; j++)
		fprintf(fp, "sokostep %s\t%s\n", m->name, m->steps[j]);
	}
}

/* ------------------------------------------------------------------ */
/* filling the Codex */

/* "level 4 of the Gnomish Mines" */
STATIC_OVL void
where_now(buf)
char *buf;
{
	if (In_endgame(&u.uz))
	    Strcpy(buf, "the Elemental Planes");
	else if (u.uz.dnum == 0)
	    Sprintf(buf, "dungeon level %d", depth(&u.uz));
	else
	    Sprintf(buf, "%s, level %d", dungeons[u.uz.dnum].dname,
		    depth(&u.uz));
}

void
codex_saw(ptr)
struct permonst *ptr;
{
	int i = monsndx(ptr), n;

	if (!bestiary || i < LOW_PM || i >= NUMMONS || bestiary[i].seen)
	    return;
	bestiary[i].seen = TRUE;
	where_now(bestiary[i].where);
	lrun.cx_new[CX_BESTIARY]++;
	lprof.dirty = TRUE;
	n = codex_count(CX_BESTIARY);
	if (n >= 100)
	    legacy_feat(F_BESTIARY100);
}

void
codex_killed(ptr)
struct permonst *ptr;
{
	int i = monsndx(ptr);

	if (!bestiary || i < LOW_PM || i >= NUMMONS)
	    return;
	codex_saw(ptr);
	bestiary[i].kills++;
	lprof.dirty = TRUE;
	if (bestiary[i].kills == 1L && insight_on(INS_FACTS)
		&& program_state.something_worth_saving)
	    pline("You add a fact sheet for %s to your Bestiary.",
		  makeplural(ptr->mname));
	insight_check();
}

void
codex_identified(otyp)
int otyp;
{
	if (!armory || otyp < 0 || otyp >= NUM_OBJECTS || armory[otyp]
		|| !OBJ_NAME(objects[otyp]) || !program_state.something_worth_saving)
	    return;
	armory[otyp] = TRUE;
	lrun.cx_new[CX_ARMORY]++;
	lprof.dirty = TRUE;
	if (codex_armory_count() >= 50)
	    legacy_feat(F_ID50);
	insight_check();
}

/* the hero entered place i of the Atlas */
void
codex_place(i)
int i;
{
	if (i < 0 || i >= NUM_PLACES || atlas[i])
	    return;
	atlas[i] = TRUE;
	lrun.cx_new[CX_ATLAS]++;
	lprof.dirty = TRUE;
	pline("You add %s to your Atlas.", places[i].name);
	pline("%s", places[i].tip);
}

STATIC_OVL int
find_place(key)
const char *key;
{
	int i;

	for (i = 0; i < NUM_PLACES; i++)
	    if (!strcmp(places[i].key, key))
		return i;
	return -1;
}

STATIC_OVL boolean
quest_portal_here()
{
	branch *br = Is_branchlev(&u.uz);

	if (!br || br->type != BR_PORTAL)
	    return FALSE;
	return (on_level(&br->end1, &u.uz) && br->end2.dnum == quest_dnum)
	    || (on_level(&br->end2, &u.uz) && br->end1.dnum == quest_dnum);
}

/* the hero arrived on a level for the first time */
void
codex_arrived()
{
	static const char *const lairs[] = {
	    "yeenoghu", "demogorg", "baalz", "geryon", "dispater", "asmodeus",
	    0
	};
	s_level *slev = Is_special(&u.uz);
	const char *dname = dungeons[u.uz.dnum].dname;
	int i;

	/* the branch first, then the level within it */
	if (In_sokoban(&u.uz))
	    codex_place(PL_SOKOBAN);
	else if (u.uz.dnum != 0 && !In_hell(&u.uz) && !In_endgame(&u.uz))
	    codex_place(find_place(dname));
	if (quest_portal_here())
	    codex_place(find_place("qportal"));
	if (slev) {
	    const char *proto = slev->proto, *dash = index(proto, '-');

	    if (dash && In_quest(&u.uz))
		codex_place(find_place(dash));
	    for (i = 0; lairs[i]; i++)
		if (!strcmp(proto, lairs[i]))
		    codex_place(find_place("lair"));
	    codex_place(find_place(proto));
	}
	if (In_V_tower(&u.uz))
	    codex_place(find_place("Vlad's Tower"));
	/* Sokoban memory: this layout's solution is known */
	if (In_sokoban(&u.uz) && insight_on(INS_SOKOBAN)) {
	    int lev = dunlev(&u.uz);
	    char name[20];

	    if (lev >= 1 && lev < SOKO_LEVS && lrun.soko_layout[lev]
		    && lrun.soko_state[lev] == SOKO_UNSEEN) {
		Sprintf(name, "soko%d-%d", lev, (int) lrun.soko_layout[lev]);
		if (soko_find(name))
		    pline("Sokoban memory: you remember solving this layout. Its solution is in your Codex (#codex, then Atlas).");
	    }
	}
}

void
codex_entered_shop()
{
	codex_place(PL_SHOP);
}

void
codex_entered_temple()
{
	codex_place(PL_TEMPLE);
}

/* a Sokoban layout, such as "soko4-2", was got through with these pushes;
   keep the shortest complete record of each layout */
void
codex_soko_remember(name, map, nmap, steps, nsteps, quality)
const char *name;
char **map, **steps;
int nmap, nsteps, quality;
{
	struct soko_mem *m = soko_find(name);
	boolean isnew = !m, had = insight_on(INS_SOKOBAN);
	int i;

	/* keep the best record: every hole filled beats got past, which
	   beats one that missed pushes; then the fewest steps */
	if (m && (quality > m->quality
		  || (quality == m->quality && m->nsteps <= nsteps)))
	    return;
	if (!m) {
	    if (nsokomem >= MAX_SOKOMEM)
		return;
	    m = &sokomem[nsokomem++];
	} else
	    soko_forget(m);
	(void) strncpy(m->name, name, sizeof m->name - 1);
	m->quality = quality;
	for (i = 0; i < nmap && i < SOKO_ROWS; i++)
	    m->map[m->nmap++] = cx_strdup(map[i]);
	for (i = 0; i < nsteps; i++)
	    soko_add_step(m, steps[i]);
	lprof.dirty = TRUE;
	if (isnew) {
	    lrun.cx_new[CX_ATLAS]++;
	    if (had)
		pline("Your Codex remembers how you got through this Sokoban layout.");
	    insight_check();
	}
}

/* ------------------------------------------------------------------ */
/* counting */

long
codex_total_kills()
{
	long n = 0L;
	int i;

	if (!bestiary)
	    return 0L;
	for (i = LOW_PM; i < NUMMONS; i++)
	    n += bestiary[i].kills;
	return n;
}

int
codex_armory_count()
{
	int i, n = 0;

	if (!armory)
	    return 0;
	for (i = 0; i < NUM_OBJECTS; i++)
	    if (armory[i])
		n++;
	return n;
}

long
codex_kills(ptr)
struct permonst *ptr;
{
	int i = monsndx(ptr);

	return (bestiary && i >= LOW_PM && i < NUMMONS) ? bestiary[i].kills
							  : 0L;
}

/* entries in a volume; -1: Sokoban layouts done, -2: kinds killed */
int
codex_count(vol)
int vol;
{
	int i, n = 0;

	if (!bestiary)
	    return 0;
	switch (vol) {
	case CX_BESTIARY:
	    for (i = LOW_PM; i < NUMMONS; i++)
		if (bestiary[i].seen) n++;
	    break;
	case CX_ARMORY:
	    n = codex_armory_count();
	    break;
	case CX_ATLAS:
	    for (i = 0; i < NUM_PLACES; i++)
		if (atlas[i]) n++;
	    break;
	case CX_LESSONS:
	    n = lprof.nlessons;
	    break;
	case -1:
	    n = nsokomem;
	    break;
	case -2:
	    for (i = LOW_PM; i < NUMMONS; i++)
		if (bestiary[i].kills) n++;
	    break;
	}
	return n;
}

/* the real name of an object type, such as "potion of healing", whether
   or not the hero knows it */
char *
legacy_type_name(otyp)
int otyp;
{
	struct objclass *oc = &objects[otyp];
	char *save_uname = oc->oc_uname, *buf, *p;
	boolean save_known = oc->oc_name_known;

	oc->oc_uname = 0;
	oc->oc_name_known = 1;
	buf = obj_typename(otyp);
	oc->oc_uname = save_uname;
	oc->oc_name_known = save_known;
	if ((p = strstri(buf, " (")) != 0)
	    *p = '\0';
	return buf;
}

/* ------------------------------------------------------------------ */
/* Lessons */

static const struct lesson_tip {
	const char *match;	/* found in the Lesson, which is lower case */
	const char *tip;
} lesson_tips[] = {
	{ "floating eye",
	  "Never hit a floating eye in melee unless you are blind or it cannot see you. Throw things at it instead, or leave it alone." },
	{ "soldier ant",
	  "Soldier ants are fast and hit twice. Fight them in a corridor, or engrave Elbereth and let them come to you." },
	{ "fire ant",
	  "Fire ants are fast and burn your things. Fight them in a corridor, and keep scrolls and potions in a bag." },
	{ "killer bee",
	  "Bees come in swarms and poison. Retreat to a corridor so only one can reach you." },
	{ "touching",
	  "Wield a cockatrice corpse only while wearing gloves, and never fall down stairs while holding one." },
	{ "turned to stone",
	  "Eating a lizard corpse or anything acidic cures stoning. Carry one once you have it, and never fight a cockatrice bare-handed." },
	{ "cockatrice",
	  "Never touch a cockatrice bare-handed. Its hissing can start to turn you to stone: eat a lizard corpse or something acidic at once." },
	{ "chickatrice",
	  "Never touch a chickatrice bare-handed. Its hissing can start to turn you to stone: eat a lizard corpse or something acidic at once." },
	{ "rabid",
	  "Rabid animals drain Constitution. Kill them from range or retreat; prayer cures the illness." },
	{ "homunculus",
	  "Sleep resistance stops its bite. Until then, fight sleep-biters at range." },
	{ "starvation",
	  "Eat when the status line says Hungry, and pray when it says Weak if your prayer is likely safe." },
	{ "exhaustion",
	  "Casting spells or using techniques while Weak from hunger can kill you. Eat first." },
	{ "choked",
	  "Stop eating when the game says you're having a hard time getting it all down." },
	{ "rotted",
	  "Eat corpses fresh: anything older than about 50 turns can be tainted. Lichen and lizard corpses never rot." },
	{ "tainted",
	  "Eat corpses fresh: anything older than about 50 turns can be tainted. Lichen and lizard corpses never rot." },
	{ "poisonous corpse",
	  "Some corpses are poisonous; without poison resistance they can kill you. Check a monster's fact sheet before you eat it." },
	{ "watchman",
	  "Do not dip in or quaff from Minetown fountains, and do not break shop doors while the watch can see." },
	{ "watch captain",
	  "Do not dip in or quaff from Minetown fountains, and do not break shop doors while the watch can see." },
	{ "shopkeeper",
	  "Never dig, teleport or zap out of a shop with unpaid items. Pay first, or drop them." },
	{ "priest",
	  "Temple priests are very tough. Never attack one, and don't desecrate their altar." },
	{ "drown",
	  "Stay away from water when something can grab you: eels can drown you in a single turn. Fight them away from the shore." },
	{ "lava",
	  "Lava kills without fire resistance. Never walk next to it while confused or stunned." },
	{ "slime",
	  "Green slime turns you into slime. Pray, or burn it off with fire (a scroll, wand or spell of fire)." },
	{ "illness",
	  "Illness kills on a timer. Pray, apply a unicorn horn, or eat a eucalyptus leaf." },
	{ "wrath",
	  "Pray only when you are in trouble, and roughly a thousand turns apart. An angry god will smite you." },
	{ "lightning bolt",
	  "Pray only when you are in trouble, and roughly a thousand turns apart. An angry god will smite you." },
	{ "gas spore",
	  "A gas spore explodes when it dies. Kill it from a distance, never in melee." },
	{ "mold",
	  "Molds never move; they hurt you when you touch them. Leave them alone or attack from range." },
	{ "jelly",
	  "Jellies never move and punish melee. Leave them alone or attack from range." },
	{ "eel",
	  "Stay away from water when eels are about: they can drown you in a single turn." },
	{ "water moccasin",
	  "Quaffing from a fountain can release water moccasins. Avoid fountains early on." },
	{ "water demon",
	  "Quaffing from a fountain can release a water demon, usually hostile early in the game. Avoid fountains early on." },
	{ "dwarf",
	  "Dwarves hit hard with mattocks and axes. Fight them at range or behind Elbereth until your armor is good." },
	{ "gnome",
	  "Gnomes in the Mines often carry crossbows, arrows and wands. Fight them one at a time." },
	{ "orc",
	  "Orcs come in groups. Fight in a corridor so only one can reach you." },
	{ "jackal",
	  "Jackals come in packs. Back into a corridor so they reach you one at a time." },
	{ "coyote",
	  "Small animals come in packs. Back into a corridor so they reach you one at a time." },
	{ "giant beetle",
	  "Giant beetles bite very hard. Avoid melee at low level; use Elbereth or attack from range." },
	{ "were",
	  "Lycanthropes summon help and their bite gives lycanthropy. Pray, or eat a sprig of wolfsbane, to cure it." },
	{ "kobold",
	  "Kobolds are weak alone but throw things. Close the distance or use a doorway." },
	{ "soldier",
	  "Soldiers come in groups with good weapons, and Elbereth doesn't scare them. Fight them in a doorway." },
	{ "mind flayer",
	  "Mind flayers eat your brain. Wear a helmet, keep away, and kill them at range." },
	{ "owlbear",
	  "Owlbears grab and crush you. Kill them at range or keep away." },
	{ "troll",
	  "Trolls come back to life. Eat or tin the corpse, or stand on it." },
	{ "leocrotta",
	  "Leocrottas are fast and hit three times. Use Elbereth or fight at range." },
	{ "giant",
	  "Giants throw boulders and hit hard. Fight them at range or in tight corridors." },
	{ "vampire",
	  "Vampires drain experience levels. Keep away and fight with ranged attacks." },
	{ "chameleon",
	  "Chameleons change into other monsters. Treat each new form as dangerous." },
	{ "falling rock",
	  "Any helmet softens falling rocks." },
	{ "land mine",
	  "Search where traps might be, and watch for the ^ marks of traps you have found." },
	{ "trap",
	  "Search where traps might be, and watch for the ^ marks of traps you have found." },
	{ "boulder",
	  "Search where traps might be, and watch for the ^ marks of traps you have found." },
	{ "kicking",
	  "Kicking walls and heavy things hurts. Don't kick when your hit points are low." },
	{ "drawbridge",
	  "Stay off a drawbridge's square when it might close." },
	{ "wand",
	  "Engrave-test unknown wands before zapping them, and never zap one at yourself." },
	{ "poison",
	  "Poison resistance prevents death by poison. Until you have it, avoid poisoned attacks and poisonous corpses." },
	{ "invisible",
	  "You can't fight what you can't see. Find a way to see invisible, or fight with your back to a wall." },
	{ 0, 0 }
};

/* a tip for a Lesson, from the table or from what killed you */
const char *
codex_lesson_tip(lesson)
const char *lesson;
{
	static char buf[BUFSZ];
	const char *name;
	char nm[BUFSZ], *p;
	int i;

	for (i = 0; lesson_tips[i].match; i++)
	    if (strstri(lesson, lesson_tips[i].match))
		return lesson_tips[i].tip;
	/* "killed by <monster>": say something about that monster */
	if ((name = strstri(lesson, "killed by ")) != 0) {
	    name += 10;
	    if (!strncmpi(name, "a ", 2))
		name += 2;
	    else if (!strncmpi(name, "an ", 3))
		name += 3;
	    else if (!strncmpi(name, "the ", 4))
		name += 4;
	    Strcpy(nm, name);
	    if ((p = strstri(nm, " called ")) != 0 || (p = index(nm, ',')) != 0)
		*p = '\0';
	    for (i = LOW_PM; i < NUMMONS; i++)
		if (!strcmpi(mons[i].mname, nm)) {
		    struct permonst *ptr = &mons[i];
		    boolean fast = ptr->mmove > 12,
			ranged = (attacktype(ptr, AT_WEAP) || attacktype(ptr, AT_MAGC)
				  || attacktype(ptr, AT_BREA) || attacktype(ptr, AT_SPIT)
				  || attacktype(ptr, AT_GAZE));

		    Sprintf(buf, "%s %s%s%s. Look at monsters with ; before fighting, and retreat to a corridor or engrave Elbereth when one outclasses you.%s",
			    upstart(makeplural(strcpy(nm, ptr->mname))),
			    (fast || ranged) ? "" : "were too strong for you",
			    fast ? "are fast" : "",
			    ranged ? (fast ? " and attack from a distance"
					   : "attack from a distance") : "",
			    (ptr->mlet == S_HUMAN || ptr == &mons[PM_MINOTAUR])
				? " Elbereth won't scare this one." : "");
		    return buf;
		}
	}
	return "Most deaths can be avoided by retreating early, praying when your hit points are very low, or using an escape item.";
}

/* ------------------------------------------------------------------ */
/* fact sheets and threat sense */

STATIC_OVL const char *
attack_word(at)
int at;
{
	switch (at) {
	case AT_NONE:	return "passive";
	case AT_CLAW:	return "claw";
	case AT_BITE:	return "bite";
	case AT_KICK:	return "kick";
	case AT_BUTT:	return "butt";
	case AT_TUCH:	return "touch";
	case AT_STNG:	return "sting";
	case AT_HUGS:	return "hug";
	case AT_SPIT:	return "spit";
	case AT_ENGL:	return "engulf";
	case AT_BREA:	return "breath";
	case AT_EXPL:	return "explode";
	case AT_BOOM:	return "explodes on death";
	case AT_GAZE:	return "gaze";
	case AT_TENT:	return "tentacles";
	case AT_MULTIPLY: return "multiplies";
	case AT_WEAP:	return "weapon";
	case AT_MAGC:	return "spell";
	}
	return "attack";
}

STATIC_OVL const char *
damage_word(ad)
int ad;
{
	switch (ad) {
	case AD_PHYS:	return 0;
	case AD_MAGM:	return "magic missile";
	case AD_FIRE:	return "fire";
	case AD_COLD:	return "cold";
	case AD_SLEE:	return "sleep";
	case AD_DISN:	return "disintegration";
	case AD_ELEC:	return "shock";
	case AD_DRST:	return "poison";
	case AD_ACID:	return "acid";
	case AD_LITE:	return "light";
	case AD_BLND:	return "blinding";
	case AD_STUN:	return "stun";
	case AD_SLOW:	return "slowing";
	case AD_PLYS:	return "paralysis";
	case AD_DRLI:	return "level drain";
	case AD_DREN:	return "drains energy";
	case AD_LEGS:	return "leg wound";
	case AD_STON:	return "petrification";
	case AD_STCK:	return "sticky";
	case AD_SGLD:	return "steals gold";
	case AD_SITM:
	case AD_SEDU:	return "steals items";
	case AD_TLPT:	return "teleports you";
	case AD_RUST:	return "rust";
	case AD_CONF:	return "confusion";
	case AD_DGST:	return "digests you";
	case AD_HEAL:	return "heals";
	case AD_WRAP:	return "drowns you";
	case AD_WERE:	return "lycanthropy";
	case AD_DRDX:	return "drains Dexterity";
	case AD_DRCO:	return "drains Constitution";
	case AD_DRIN:	return "eats brains";
	case AD_DISE:	return "disease";
	case AD_DCAY:	return "rots";
	case AD_SSEX:	return "seduces";
	case AD_HALU:	return "hallucination";
	case AD_DETH:	return "death";
	case AD_PEST:	return "pestilence";
	case AD_FAMN:	return "famine";
	case AD_SLIM:	return "sliming";
	case AD_CALM:	return "calming";
	case AD_ENCH:	return "disenchants";
	case AD_POLY:	return "polymorph";
	case AD_CORR:	return "corrosion";
	case AD_TCKL:	return "tickling";
	case AD_CLRC:	return "clerical spells";
	case AD_SPEL:	return "mage spells";
	case AD_RBRE:	return "random breath";
	case AD_SAMU:	return "steals the Amulet";
	case AD_CURS:	return "curses";
	}
	return 0;
}

/* append ", " and s to buf if it fits */
STATIC_OVL void
add_item(buf, s, first)
char *buf;
const char *s;
boolean *first;
{
	if (strlen(buf) + strlen(s) + 3 >= BUFSZ - 10)
	    return;
	if (!*first)
	    Strcat(buf, ", ");
	Strcat(buf, s);
	*first = FALSE;
}

/* the fact sheet for ptr, in up to three lines; returns the line count */
STATIC_OVL int
fact_sheet(ptr, lines)
struct permonst *ptr;
char lines[3][BUFSZ];
{
	long kills = codex_kills(ptr);
	char tmp[BUFSZ];
	boolean first = TRUE;
	int i, n = 0;
	struct attack *a;

	if (kills < 1L)
	    return 0;
	Sprintf(lines[n], "%s: level %d, speed %d, AC %d, magic resistance %d%%.",
		upstart(strcpy(tmp, ptr->mname)), (int) ptr->mlevel,
		(int) ptr->mmove, (int) ptr->ac, (int) ptr->mr);
	n++;
	Strcpy(lines[n], "Attacks: ");
	for (i = 0; i < NATTK; i++) {
	    const char *dw;

	    a = &ptr->mattk[i];
	    if (!a->aatyp && !a->adtyp && !a->damn && !a->damd)
		continue;
	    dw = damage_word(a->adtyp);
	    if (a->damn && a->damd)
		Sprintf(tmp, "%s %dd%d%s%s%s", attack_word(a->aatyp), a->damn,
			a->damd, dw ? " (" : "", dw ? dw : "", dw ? ")" : "");
	    else
		Sprintf(tmp, "%s%s%s%s", attack_word(a->aatyp), dw ? " (" : "",
			dw ? dw : "", dw ? ")" : "");
	    add_item(lines[n], tmp, &first);
	}
	if (first)
	    Strcat(lines[n], "none");
	Strcat(lines[n], ".");
	n++;
	if (kills >= 5L) {
	    static const struct { long bit; const char *name; } res[] = {
		{ MR_FIRE, "fire" }, { MR_COLD, "cold" }, { MR_SLEEP, "sleep" },
		{ MR_DISINT, "disintegration" }, { MR_ELEC, "shock" },
		{ MR_POISON, "poison" }, { MR_ACID, "acid" },
		{ MR_STONE, "stoning" }, { 0, 0 }
	    };
	    char res_buf[BUFSZ], eat_buf[BUFSZ], danger[BUFSZ];
	    boolean f1 = TRUE, f2 = TRUE, f3 = TRUE;

	    *res_buf = *eat_buf = *danger = '\0';
	    for (i = 0; res[i].name; i++)
		if (ptr->mresists & res[i].bit)
		    add_item(res_buf, res[i].name, &f1);
	    if (poisonous(ptr)) add_item(eat_buf, "poisonous", &f2);
	    if (acidic(ptr)) add_item(eat_buf, "acidic (cures stoning)", &f2);
	    if (touch_petrifies(ptr)) add_item(eat_buf, "petrifies", &f2);
	    if (is_were(ptr)) add_item(eat_buf, "lycanthropy", &f2);
	    for (i = 0; res[i].name; i++)
		if (ptr->mconveys & res[i].bit) {
		    Sprintf(tmp, "may give %s resistance", res[i].name);
		    add_item(eat_buf, tmp, &f2);
		}
	    if (telepathic(ptr)) add_item(eat_buf, "may give telepathy", &f2);
	    if (can_teleport(ptr)) add_item(eat_buf, "may give teleportitis", &f2);
	    if (dmgtype(ptr, AD_STON)) add_item(danger, "can turn you to stone", &f3);
	    if (dmgtype(ptr, AD_DETH) || dmgtype(ptr, AD_DISN))
		add_item(danger, "can kill outright", &f3);
	    if (dmgtype(ptr, AD_DRLI)) add_item(danger, "drains levels", &f3);
	    if (dmgtype(ptr, AD_SLIM)) add_item(danger, "turns you to slime", &f3);
	    if (dmgtype(ptr, AD_WRAP)) add_item(danger, "can drown you", &f3);
	    if (dmgtype(ptr, AD_DGST)) add_item(danger, "swallows you whole", &f3);
	    if (ptr == &mons[PM_FLOATING_EYE])
		add_item(danger, "its gaze paralyses you if you hit it", &f3);
	    else if (attacktype_fordmg(ptr, AT_NONE, AD_PLYS))
		add_item(danger, "paralyses you if you hit it", &f3);
	    if (touch_petrifies(ptr))
		add_item(danger, "deadly to touch bare-handed", &f3);
	    Sprintf(lines[n], "Resists: %s. Eating it: %s.%s%s",
		    *res_buf ? res_buf : "nothing",
		    *eat_buf ? eat_buf : "nothing special",
		    *danger ? " Danger: " : "", danger);
	    if (*danger)
		Strcat(lines[n], ".");
	    n++;
	}
	return n;
}

/* threat sense: how dangerous ptr is for the hero right now */
STATIC_OVL const char *
threat_word(ptr)
struct permonst *ptr;
{
	int d;

	if (ptr == &mons[PM_FLOATING_EYE])
	    return "harmless unless you hit it";
	if (touch_petrifies(ptr) && !Stone_resistance)
	    return "deadly to touch";
	d = monstr[monsndx(ptr)] - (int) u.ulevel;
	if (!ptr->mattk[0].aatyp && !ptr->mattk[0].damn)
	    return "harmless";
	if (d <= -6) return "harmless";
	if (d <= -2) return "easy";
	if (d <= 2) return "an even match";
	if (d <= 6) return "dangerous";
	return "deadly";
}

/* the hero looked at a monster of type ptr (mtmp, if it is really there) */
void
codex_look(ptr, mtmp)
struct permonst *ptr;
struct monst *mtmp;
{
	char lines[3][BUFSZ];
	int i, n;

	if (mtmp && (mtmp == u.usteed || mtmp->mtame))
	    mtmp = 0;
	if (insight_on(INS_THREAT) && ptr != youmonst.data)
	    pline("Threat sense: %s looks %s to you.",
		  mtmp ? mon_nam(mtmp) : "it", threat_word(ptr));
	if (insight_on(INS_FACTS) && (n = fact_sheet(ptr, lines)) > 0)
	    for (i = 0; i < n; i++)
		pline("%s", lines[i]);
}

/* ------------------------------------------------------------------ */
/* what objects do, for the Armory */

static const struct item_note {
	const char *name;	/* the real name, as legacy_type_name() gives */
	const char *note;
} item_notes[] = {
	/* potions */
	{ "potion of booze", "Confuses you briefly and gives a little nutrition." },
	{ "potion of fruit juice", "Gives a little nutrition." },
	{ "potion of see invisible", "Lets you see invisible things for a while, or for good if blessed. Tastes like fruit juice." },
	{ "potion of sickness", "Poisons you, costing Strength and hit points." },
	{ "potion of sleeping", "Puts you to sleep unless you resist sleep." },
	{ "potion of clairvoyance", "Shows you the map around you." },
	{ "potion of confusion", "Confuses you." },
	{ "potion of hallucination", "Makes you hallucinate for a while." },
	{ "potion of healing", "Heals some hit points and cures blindness; can raise your maximum." },
	{ "potion of extra healing", "Heals many hit points and cures blindness and hallucination; can raise your maximum." },
	{ "potion of restore ability", "Restores lowered attributes; blessed restores them all." },
	{ "potion of blindness", "Blinds you for a while." },
	{ "potion of ESP", "Gives you telepathy for a while." },
	{ "potion of gain energy", "Restores and raises your magical energy." },
	{ "potion of invisibility", "Makes you invisible for a while." },
	{ "potion of monster detection", "Shows the monsters on the level." },
	{ "potion of object detection", "Shows the objects on the level." },
	{ "potion of enlightenment", "Shows your attributes and powers; blessed also raises your Intelligence and Wisdom." },
	{ "potion of full healing", "Heals all your hit points and raises your maximum." },
	{ "potion of levitation", "Makes you float for a while; you can't pick things up or go down stairs." },
	{ "potion of polymorph", "Turns you into a random monster." },
	{ "potion of speed", "Makes you very fast for a while and heals wounded legs." },
	{ "potion of acid", "Burns you unless you resist acid, but cures stoning." },
	{ "potion of oil", "Can be lit and used as a lamp, or lit and thrown to explode." },
	{ "potion of gain ability", "Raises an attribute; blessed raises them all." },
	{ "potion of gain level", "Raises your experience level; a cursed one carries you up through the ceiling." },
	{ "potion of invulnerability", "Makes you invulnerable for a short time." },
	{ "potion of paralysis", "Freezes you in place for several turns." },
	{ "potion of water", "Plain water. Holy water blesses what you dip in it; unholy water curses it." },
	{ "potion of blood", "Nourishes vampires; other creatures find it revolting." },
	{ "potion of vampire blood", "Nourishes and heals vampires." },
	{ "potion of amnesia", "Makes you forget the map and some of what you have identified." },
	/* scrolls */
	{ "scroll of create monster", "Creates a monster next to you." },
	{ "scroll of taming", "Tames the monsters next to you." },
	{ "scroll of light", "Lights up the area around you." },
	{ "scroll of food detection", "Shows the food on the level." },
	{ "scroll of gold detection", "Shows the gold on the level; confused, it shows traps." },
	{ "scroll of identify", "Identifies one or more of your items." },
	{ "scroll of magic mapping", "Maps the current level." },
	{ "scroll of confuse monster", "Your next melee hit confuses the monster." },
	{ "scroll of scare monster", "Most monsters won't step onto it while it lies on the floor. It crumbles if picked up after being dropped." },
	{ "scroll of enchant weapon", "Raises your wielded weapon's enchantment." },
	{ "scroll of enchant armor", "Raises a worn armor's enchantment; too much makes it evaporate." },
	{ "scroll of remove curse", "Uncurses your worn and wielded items; blessed uncurses everything you carry." },
	{ "scroll of teleportation", "Teleports you within the level; confused, it moves you to another level." },
	{ "scroll of fire", "Bursts into flames around you. Useful against green slime." },
	{ "scroll of earth", "Drops boulders around you." },
	{ "scroll of destroy armor", "Destroys a piece of armor you are wearing." },
	{ "scroll of amnesia", "Makes you forget the map and some of what you have identified." },
	{ "scroll of charging", "Recharges a wand or tool; confused, it restores your energy." },
	{ "scroll of genocide", "Wipes out a kind of monster; blessed, a whole class. Cursed, it creates them instead." },
	{ "scroll of punishment", "Chains a heavy iron ball to you." },
	{ "scroll of stinking cloud", "Lets you place a cloud of poison gas." },
	{ "scroll of blank paper", "Blank. A magic marker can write a scroll you know onto it." },
	/* rings */
	{ "ring of adornment", "Raises or lowers your Charisma by its enchantment." },
	{ "ring of hunger", "Makes you hungry much faster." },
	{ "ring of mood", "Lets you sense your god's mood." },
	{ "ring of protection", "Improves your armor class by its enchantment." },
	{ "ring of protection from shape changers", "Stops shape changers from changing form." },
	{ "ring of sleeping", "Makes you fall asleep at random." },
	{ "ring of stealth", "Lets you move without waking monsters." },
	{ "ring of sustain ability", "Keeps your attributes from changing." },
	{ "ring of warning", "Warns you of nearby hostile monsters." },
	{ "ring of aggravate monster", "Wakes and angers monsters." },
	{ "ring of cold resistance", "Protects you from cold." },
	{ "ring of gain constitution", "Changes your Constitution by its enchantment." },
	{ "ring of gain dexterity", "Changes your Dexterity by its enchantment." },
	{ "ring of gain intelligence", "Changes your Intelligence by its enchantment." },
	{ "ring of gain strength", "Changes your Strength by its enchantment." },
	{ "ring of gain wisdom", "Changes your Wisdom by its enchantment." },
	{ "ring of increase accuracy", "Adds its enchantment to your chance to hit." },
	{ "ring of increase damage", "Adds its enchantment to the damage you do." },
	{ "ring of slow digestion", "Makes you hunger very slowly." },
	{ "ring of invisibility", "Makes you invisible while worn." },
	{ "ring of poison resistance", "Protects you from poison." },
	{ "ring of see invisible", "Lets you see invisible things." },
	{ "ring of shock resistance", "Protects you from electricity." },
	{ "ring of fire resistance", "Protects you from fire." },
	{ "ring of free action", "Stops you from being paralysed." },
	{ "ring of levitation", "Makes you float while worn." },
	{ "ring of regeneration", "Heals you quickly, but makes you hungry faster." },
	{ "ring of searching", "Finds hidden doors and traps around you automatically." },
	{ "ring of teleportation", "Teleports you at random." },
	{ "ring of conflict", "Makes monsters fight each other, and you." },
	{ "ring of polymorph", "Polymorphs you at random." },
	{ "ring of polymorph control", "Lets you choose what you polymorph into." },
	{ "ring of teleport control", "Lets you choose where you teleport to." },
	/* amulets */
	{ "amulet of change", "Changes your sex, then crumbles." },
	{ "amulet of drain resistance", "Protects you from level drain." },
	{ "amulet of ESP", "Gives you telepathy." },
	{ "amulet of flying", "Lets you fly." },
	{ "amulet of life saving", "Saves you from death once, then crumbles." },
	{ "amulet of magical breathing", "Lets you live without air, even underwater." },
	{ "amulet of reflection", "Reflects rays and gazes." },
	{ "amulet of restful sleep", "Makes you fall asleep at random." },
	{ "amulet of strangulation", "Strangles you to death in six turns. Take it off at once." },
	{ "amulet of unchanging", "Stops you from changing form." },
	{ "amulet versus poison", "Protects you from poison." },
	{ "amulet versus stone", "Protects you from turning to stone." },
	/* wands */
	{ "wand of light", "Lights up the area." },
	{ "wand of nothing", "Does nothing." },
	{ "wand of enlightenment", "Shows your attributes and powers." },
	{ "wand of healing", "Heals the monster you zap, or you." },
	{ "wand of locking", "Locks doors and boxes, and closes holes." },
	{ "wand of make invisible", "Makes a monster or object invisible." },
	{ "wand of opening", "Opens locks, and can free you from a trap or a monster's grip." },
	{ "wand of probing", "Shows a monster's health and what it carries." },
	{ "wand of secret door detection", "Reveals hidden doors and traps nearby." },
	{ "wand of slow monster", "Slows a monster." },
	{ "wand of speed monster", "Speeds up a monster; zapped at yourself, it makes you fast for good." },
	{ "wand of striking", "Hits with a blast of force; magic resistance stops it." },
	{ "wand of undead turning", "Makes undead flee, and brings corpses back to life." },
	{ "wand of draining", "Drains a monster's levels." },
	{ "wand of cancellation", "Strips the magic from monsters and objects. Never put it in a bag of holding." },
	{ "wand of create monster", "Creates a monster." },
	{ "wand of fear", "Makes monsters flee." },
	{ "wand of polymorph", "Changes monsters and objects into others." },
	{ "wand of teleportation", "Teleports monsters and objects away." },
	{ "wand of create horde", "Creates several monsters." },
	{ "wand of extra healing", "Heals the monster you zap, or you, by a lot." },
	{ "wand of wishing", "Grants wishes." },
	{ "wand of digging", "Digs through walls and floors." },
	{ "wand of magic missile", "Fires magic missiles." },
	{ "wand of fire", "Fires a ray of fire. Engraving with it burns a lasting Elbereth." },
	{ "wand of cold", "Fires a ray of cold that freezes water." },
	{ "wand of sleep", "Fires a ray that puts monsters to sleep." },
	{ "wand of death", "Fires a ray that kills most monsters outright." },
	{ "wand of lightning", "Fires a ray of lightning; engraving with it blinds you." },
	{ "wand of fireball", "Fires exploding balls of fire." },
	{ 0, 0 }
};

/* the Armory page for otyp */
STATIC_OVL void
armory_page(otyp)
int otyp;
{
	winid win = create_nhwindow(NHW_TEXT);
	struct objclass *oc = &objects[otyp];
	char buf[BUFSZ], name[BUFSZ];
	boolean first = TRUE;
	int i;

	Strcpy(name, legacy_type_name(otyp));
	putstr(win, ATR_BOLD, upstart(name));
	Sprintf(buf, "Base price %d, weight %d.", (int) oc->oc_cost,
		(int) oc->oc_weight);
	putstr(win, 0, buf);
	for (i = 0; item_notes[i].name; i++)
	    if (!strcmp(item_notes[i].name, name)) {
		putstr(win, 0, item_notes[i].note);
		break;
	    }
	if (oc->oc_class == SPBOOK_CLASS && otyp != SPE_BLANK_PAPER
		&& otyp != SPE_BOOK_OF_THE_DEAD) {
	    Sprintf(buf, "Teaches a level %d spell. Books above your level can paralyse or teleport you when you read them.",
		    (int) oc->oc_level);
	    putstr(win, 0, buf);
	} else if (oc->oc_class == ARMOR_CLASS) {
	    Sprintf(buf, "Armor class bonus %d.", (int) oc->a_ac);
	    putstr(win, 0, buf);
	} else if (oc->oc_class == WEAPON_CLASS) {
	    Sprintf(buf, "Damage d%d against small monsters, d%d against large ones.",
		    (int) oc->oc_wsdam, (int) oc->oc_wldam);
	    putstr(win, 0, buf);
	}
	/* the price group: other types of the class at the same price */
	Strcpy(buf, "Same base price: ");
	for (i = bases[(int) oc->oc_class]; i < NUM_OBJECTS
		&& objects[i].oc_class == oc->oc_class; i++)
	    if (i != otyp && OBJ_NAME(objects[i]) && objects[i].oc_prob
		    && objects[i].oc_cost == oc->oc_cost)
		add_item(buf, legacy_type_name(i), &first);
	if (!first) {
	    putstr(win, 0, "");
	    putstr(win, 0, buf);
	}
	display_nhwindow(win, TRUE);
	destroy_nhwindow(win);
}

/* ------------------------------------------------------------------ */
/* reading the Codex */

STATIC_OVL void
text_page(title, lines, n)
const char *title;
char lines[][BUFSZ];
int n;
{
	winid win = create_nhwindow(NHW_TEXT);
	int i;

	putstr(win, ATR_BOLD, title);
	for (i = 0; i < n; i++)
	    putstr(win, 0, lines[i]);
	display_nhwindow(win, TRUE);
	destroy_nhwindow(win);
}

/* a long line, broken at spaces to fit a phone screen */
void
legacy_put_wrapped(win, text)
winid win;
const char *text;
{
	char buf[BUFSZ];
	const char *p = text;
	int len, cut;

	while (*p) {
	    len = strlen(p);
	    if (len <= 60) {
		putstr(win, 0, p);
		break;
	    }
	    for (cut = 60; cut > 20 && p[cut] != ' '; cut--)
		;
	    (void) strncpy(buf, p, cut);
	    buf[cut] = '\0';
	    putstr(win, 0, buf);
	    p += cut;
	    while (*p == ' ')
		p++;
	}
}

STATIC_OVL int
pick_one(win, prompt)
winid win;
const char *prompt;
{
	menu_item *pick = 0;
	int n, choice;

	end_menu(win, prompt);
	n = select_menu(win, PICK_ONE, &pick);
	destroy_nhwindow(win);
	choice = (n > 0) ? pick[0].item.a_int : 0;
	if (pick)
	    free((genericptr_t) pick);
	return choice;
}

STATIC_OVL void
add_entry(win, val, str)
winid win;
int val;
const char *str;
{
	anything any;

	any.a_void = 0;
	any.a_int = val;
	add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE, str, MENU_UNSELECTED);
}

STATIC_OVL void
add_heading(win, str)
winid win;
const char *str;
{
	anything any;

	any.a_void = 0;
	add_menu(win, NO_GLYPH, &any, 0, 0, iflags.menu_headings, str,
		 MENU_UNSELECTED);
}

STATIC_OVL void
show_bestiary()
{
	winid win;
	char buf[BUFSZ], lines[6][BUFSZ];
	int i, c, n, choice;

	for (;;) {
	    win = create_nhwindow(NHW_MENU);
	    start_menu(win);
	    n = 0;
	    for (c = 1; c < MAXMCLASSES; c++)
		for (i = LOW_PM; i < NUMMONS; i++)
		    if (bestiary[i].seen && mons[i].mlet == c) {
			if (bestiary[i].kills)
			    Sprintf(buf, "%c  %s (%ld killed)", def_monsyms[c],
				    mons[i].mname, bestiary[i].kills);
			else
			    Sprintf(buf, "%c  %s", def_monsyms[c], mons[i].mname);
			add_entry(win, i, buf);
			n++;
		    }
	    if (!n) {
		destroy_nhwindow(win);
		pline("Your Bestiary is empty so far.");
		return;
	    }
	    Sprintf(buf, "Bestiary: %d monsters", n);
	    choice = pick_one(win, buf);
	    if (!choice)
		return;
	    n = 0;
	    Sprintf(lines[n++], "Symbol %c. First seen on %s.",
		    def_monsyms[(int) mons[choice].mlet], bestiary[choice].where);
	    Sprintf(lines[n++], "Killed: %ld.", bestiary[choice].kills);
	    n += fact_sheet(&mons[choice], &lines[n]);
	    if (!bestiary[choice].kills)
		Strcpy(lines[n++], "Kill one to see its fact sheet.");
	    else if (bestiary[choice].kills < 5L)
		Strcpy(lines[n++], "Kill five to learn more about it.");
	    Strcpy(buf, mons[choice].mname);
	    text_page(upstart(buf), lines, n);
	}
}

STATIC_OVL void
show_armory()
{
	winid win;
	int i, c, n, choice;
	char buf[BUFSZ];
	boolean head;

	for (;;) {
	    win = create_nhwindow(NHW_MENU);
	    start_menu(win);
	    n = 0;
	    for (c = 1; c < MAXOCLASSES; c++) {
		head = FALSE;
		for (i = 0; i < NUM_OBJECTS; i++)
		    if (armory[i] && objects[i].oc_class == c) {
			if (!head) {
			    add_heading(win, let_to_name(def_oc_syms[c], FALSE));
			    head = TRUE;
			}
			add_entry(win, i + 1, legacy_type_name(i));
			n++;
		    }
	    }
	    if (!n) {
		destroy_nhwindow(win);
		pline("Your Armory is empty so far. Identify something to start it.");
		return;
	    }
	    Sprintf(buf, "Armory: %d object types identified", n);
	    choice = pick_one(win, buf);
	    if (!choice)
		return;
	    armory_page(choice - 1);
	}
}

/* "soko4-2" -> "level 1, layout 2"; soko4 is the entry level */
STATIC_OVL char *
soko_title(name, buf)
const char *name;
char *buf;
{
	int lev, var;

	if (sscanf(name, "soko%d-%d", &lev, &var) == 2)
	    Sprintf(buf, "Sokoban level %d, layout %d", 5 - lev, var);
	else
	    Strcpy(buf, name);
	return buf;
}

STATIC_OVL void
show_solution(i)
int i;
{
	winid win = create_nhwindow(NHW_TEXT);
	struct soko_mem *m = &sokomem[i];
	char buf[BUFSZ];
	int j;

	putstr(win, ATR_BOLD, soko_title(m->name, buf));
	putstr(win, 0, "How you got through it. Boulders are letters, holes and");
	putstr(win, 0, "pits are ^, and you start on @.");
	if (m->quality == SQ_PARTIAL)
	    putstr(win, 0, "You left some holes unfilled, so this may not be complete.");
	else if (m->quality == SQ_LOST)
	    putstr(win, 0, "Some pushes were missed, so this may not work as written.");
	putstr(win, 0, "");
	for (j = 0; j < m->nmap; j++)
	    putstr(win, 0, m->map[j]);
	putstr(win, 0, "");
	putstr(win, 0, "Push the boulders in this order, walking round to the far");
	putstr(win, 0, "side of each first:");
	for (j = 0; j < m->nsteps; j++)
	    putstr(win, 0, m->steps[j]);
	display_nhwindow(win, TRUE);
	destroy_nhwindow(win);
}

STATIC_OVL void
show_atlas()
{
	winid win;
	int i, n, choice;
	char buf[BUFSZ];

	for (;;) {
	    win = create_nhwindow(NHW_MENU);
	    start_menu(win);
	    n = 0;
	    for (i = 0; i < NUM_PLACES; i++)
		if (atlas[i]) {
		    add_entry(win, i + 1, places[i].name);
		    n++;
		}
	    if (codex_count(-1)) {
		add_heading(win, insight_on(INS_SOKOBAN)
			    ? "Sokoban memory: solutions"
			    : "Sokoban memory (switched off in Settings)");
		for (i = 0; i < nsokomem; i++)
		    if (insight_on(INS_SOKOBAN)) {
			add_entry(win, 1000 + i, soko_title(sokomem[i].name, buf));
			n++;
		    }
	    }
	    if (!n) {
		destroy_nhwindow(win);
		pline("Your Atlas is empty so far.");
		return;
	    }
	    choice = pick_one(win, "Atlas: places you have been");
	    if (!choice)
		return;
	    if (choice >= 1000) {
		show_solution(choice - 1000);
	    } else {
		win = create_nhwindow(NHW_TEXT);
		putstr(win, ATR_BOLD, places[choice - 1].name);
		legacy_put_wrapped(win, places[choice - 1].tip);
		display_nhwindow(win, TRUE);
		destroy_nhwindow(win);
	    }
	}
}

STATIC_OVL void
show_lessons()
{
	winid win;
	int i, choice;
	char buf[BUFSZ];

	if (!lprof.nlessons) {
	    pline("You have no Lessons yet. Each new way to die teaches one.");
	    return;
	}
	for (;;) {
	    win = create_nhwindow(NHW_MENU);
	    start_menu(win);
	    for (i = 0; i < lprof.nlessons; i++) {
		Strcpy(buf, lprof.lessons[i]);
		add_entry(win, i + 1, upstart(buf));
	    }
	    Sprintf(buf, "Lessons: %d ways you have died", lprof.nlessons);
	    choice = pick_one(win, buf);
	    if (!choice)
		return;
	    win = create_nhwindow(NHW_TEXT);
	    Strcpy(buf, lprof.lessons[choice - 1]);
	    putstr(win, ATR_BOLD, upstart(buf));
	    legacy_put_wrapped(win, codex_lesson_tip(lprof.lessons[choice - 1]));
	    display_nhwindow(win, TRUE);
	    destroy_nhwindow(win);
	}
}

/* the Insight page: each item, whether it is earned, and how close */
STATIC_OVL void
show_insight()
{
	winid win = create_nhwindow(NHW_TEXT);
	char buf[BUFSZ], prog[BUFSZ];
	int i;

	putstr(win, ATR_BOLD, "Insight: help you earn by doing things");
	putstr(win, 0, "");
	for (i = 0; i < NUM_INSIGHT; i++) {
	    insight_progress(i, prog);
	    Sprintf(buf, "%s: %s (%s)", insight_name(i),
		    !insight_earned(i) ? "not yet" :
		    lprof.ins_off[i] ? "earned, switched off" : "on", prog);
	    putstr(win, insight_earned(i) ? ATR_BOLD : 0, buf);
	    legacy_put_wrapped(win, insight_desc(i));
	    putstr(win, 0, "");
	}
	display_nhwindow(win, TRUE);
	destroy_nhwindow(win);
}

/* the Codex's contents page */
void
codex_menu(from_hall)
boolean from_hall;
{
	winid win;
	char buf[BUFSZ];
	int choice, n;

	legacy_init();
	for (;;) {
	    win = create_nhwindow(NHW_MENU);
	    start_menu(win);
	    Sprintf(buf, "Bestiary (%d monsters)", codex_count(CX_BESTIARY));
	    add_entry(win, 1, buf);
	    Sprintf(buf, "Armory (%d object types)", codex_count(CX_ARMORY));
	    add_entry(win, 2, buf);
	    Sprintf(buf, "Atlas (%d places)", codex_count(CX_ATLAS));
	    add_entry(win, 3, buf);
	    Sprintf(buf, "Lessons (%d)", codex_count(CX_LESSONS));
	    add_entry(win, 4, buf);
	    for (n = 0, choice = 0; choice < NUM_INSIGHT; choice++)
		if (insight_earned(choice))
		    n++;
	    Sprintf(buf, "Insight (%d of %d earned)", n, NUM_INSIGHT);
	    add_entry(win, 5, buf);
	    choice = pick_one(win, from_hall ? "The Codex" : "Your Codex");
	    switch (choice) {
	    case 1: show_bestiary(); break;
	    case 2: show_armory(); break;
	    case 3: show_atlas(); break;
	    case 4: show_lessons(); break;
	    case 5: show_insight(); break;
	    default: return;
	    }
	}
}

/* the #codex command; takes no time */
int
docodex()
{
	codex_menu(FALSE);
	return 0;
}

/*codex.c*/
