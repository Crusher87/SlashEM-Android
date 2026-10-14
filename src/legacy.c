/*	SCCS Id: @(#)legacy.c	0.0.7E7F3	2026/10/13	*/
/* NetHack may be freely redistributed.  See license for details. */

/*
 * Slash'EM Legacy: the profile that carries over from game to game.
 *
 * Every game ends by paying Renown into the profile.  Renown buys Lineage
 * upgrades, which make the start of later Legacy games a little easier,
 * and roles and races that feats have not opened yet.  The profile is a
 * plain text file, legacy.dat, in the game's directory; the current
 * game's part (struct legacy_run) is saved with the game.
 */

#include "hack.h"
#include "dlb.h"

struct legacy_profile lprof;
struct legacy_run lrun;
struct legacy_result lres;
boolean legacy_selecting = FALSE;
boolean legacy_throwing = FALSE;

static boolean legacy_loaded = FALSE;

/* rank II and III upgrades open once this much Renown was earned in all */
#define RANK2_GATE	1500L
#define RANK3_GATE	5000L

#define MAX_ANCESTORS	300	/* older ones are dropped from the file */
#define RENOWN_PER_LEVEL 5
#define RENOWN_LESSON	10
#define RENOWN_MINIMUM	10

static const struct lin_track {
	const char *name, *key;
	short cost[LIN_MAXRANK];
	const char *desc[LIN_MAXRANK];
} lineage_tracks[NUM_LINEAGE] = {
	{ "Vigor", "vigor", { 60, 250, 700 },
	  { "+2 maximum hit points at the start",
	    "+4 maximum hit points at the start",
	    "+6 maximum hit points at the start" } },
	{ "Provisions", "provisions", { 40, 150, 400 },
	  { "a spare food ration",
	    "two spare food rations",
	    "two food rations and a lembas wafer" } },
	{ "Remedies", "remedies", { 100, 350, 900 },
	  { "a potion of healing",
	    "two potions of healing",
	    "two potions of healing and one of extra healing" } },
	{ "Escape", "escape", { 150, 450, 1000 },
	  { "a scroll of teleportation",
	    "two scrolls of teleportation",
	    "two scrolls of teleportation and a wand of digging" } },
	{ "Patron", "patron", { 200, 600, 1500 },
	  { "your first prayer can work from turn 1",
	    "a shorter wait after each successful prayer",
	    "+1 Luck that never fades" } },
	{ "Lore", "lore", { 120, 400, 1000 },
	  { "one potion type known at the start",
	    "plus one scroll type",
	    "plus one wand type and one ring type" } },
	{ "Companion", "companion", { 80, 300, 800 },
	  { "your pet starts a level higher",
	    "your pet starts two levels higher",
	    "a second pet" } },
	{ "Arms", "arms", { 150, 450, 1000 },
	  { "starting weapon +1",
	    "starting body armor +1",
	    "starting gear can't rust, burn or rot; weapon +1 more" } },
	{ "Ward", "ward", { 1000, 2500, 5000 },
	  { "once per game, survive a death on levels 1 to 4",
	    "the Ward works on levels 1 to 10",
	    "the Ward works anywhere" } },
	{ "Favor", "favor", { 80, 400, 1200 },
	  { "keep 1 of 2 blessings", "keep 1 of 3 blessings",
	    "keep 2 of 4 blessings" } },
	{ "Fortune", "fortune", { 250, 800, 1800 },
	  { "fate drafts offer 3 choices", "drafts at all eight milestones",
	    "one reroll per draft" } },
	{ "Inheritance", "inheritance", { 150, 600, 1500 },
	  { "heirlooms keep up to +2 and 3 charges", "a second vault slot",
	    "a third slot; up to +3 and 4 charges" } },
};

/* the feat each Ward rank needs, besides Renown */
static const int ward_feat[LIN_MAXRANK] = { F_LESSONS10, F_MEDUSA, F_GEHENNOM };

#define FG_DESCENT	0
/* names used for Insight in legacy.dat */
static const char *const icounter_keys[NUM_ICOUNTERS] = {
	"corpses", "prayers", "wands", "elbereth"
};
static const char *const insight_keys[NUM_INSIGHT] = {
	"facts", "threat", "warnings", "corpse", "prayer", "price", "wand",
	"elbereth", "sokoban"
};

#define FG_BRANCHES	1
#define FG_SKILL	2
#define FG_KNOWLEDGE	3
#define FG_ANCESTRY	4
#define FG_EXPEDITION	5
#define FG_DAILY	6

static const struct feat_def {
	const char *key, *name;
	short renown;
	xchar group;
	const char *unlocks;	/* what it opens, or 0 */
} feats[NUM_FEATS] = {
	{ "dl3", "Reach dungeon level 3", 10, FG_DESCENT, 0 },
	{ "mines", "Enter the Gnomish Mines", 15, FG_DESCENT, 0 },
	{ "minetown", "Reach Minetown", 25, FG_DESCENT, "the Caveman" },
	{ "oracle", "Consult the Oracle", 25, FG_DESCENT, "the Healer" },
	{ "soko1", "Finish one Sokoban level", 30, FG_DESCENT, "the Knight" },
	{ "sokoban", "Finish Sokoban", 75, FG_DESCENT, "the hobbit race" },
	{ "luckstone", "Claim the Mines' End luckstone", 100, FG_DESCENT, 0 },
	{ "dl10", "Reach dungeon level 10", 50, FG_DESCENT, "the Tourist" },
	{ "bigroom", "Enter the Big Room", 30, FG_DESCENT, 0 },
	{ "qportal", "Reach the Quest portal level", 75, FG_DESCENT, 0 },
	{ "medusa", "Kill Medusa", 150, FG_DESCENT, "rank II of the Ward" },
	{ "castle", "Enter the Castle", 150, FG_DESCENT, 0 },
	{ "wishwand", "Take the wand of wishing", 200, FG_DESCENT, 0 },
	{ "gehennom", "Enter Gehennom", 250, FG_DESCENT, "rank III of the Ward" },
	{ "vlad", "Enter Vlad's Tower", 150, FG_DESCENT, 0 },
	{ "quest", "Complete your Quest", 300, FG_DESCENT, 0 },
	{ "amulet", "Hold the Amulet of Yendor", 400, FG_DESCENT, 0 },
	{ "ascend", "Ascend", 1000, FG_DESCENT, 0 },
	{ "grund", "Visit Grund's Stronghold", 50, FG_BRANCHES, 0 },
	{ "wyrm", "Enter the Wyrm Caves", 75, FG_BRANCHES, 0 },
	{ "sam", "Enter One-eyed Sam's Market", 75, FG_BRANCHES, 0 },
	{ "tomb", "Enter the Lost Tomb", 75, FG_BRANCHES, 0 },
	{ "spiders", "Enter the Spider Caves", 75, FG_BRANCHES, 0 },
	{ "sunless", "Enter the Sunless Sea", 100, FG_BRANCHES, 0 },
	{ "moloch", "Enter the Temple of Moloch", 100, FG_BRANCHES, 0 },
	{ "ludios", "Enter Fort Ludios", 150, FG_BRANCHES, 0 },
	{ "franken", "Enter Frankenstein's Lab", 150, FG_BRANCHES, 0 },
	{ "dig", "Dig through a floor", 20, FG_SKILL, "the Archeologist" },
	{ "thrown20", "20 thrown or fired kills in one game", 40, FG_SKILL,
	  "the Rogue" },
	{ "xl8", "Reach experience level 8", 40, FG_SKILL, "the Monk" },
	{ "petdl8", "Reach dungeon level 8 with your first pet alive", 50,
	  FG_SKILL, "the Yeoman" },
	{ "wizxl10", "Reach experience level 10 as a Wizard", 60, FG_SKILL,
	  "the Flame Mage" },
	{ "spells100", "Cast 100 spells, across games", 60, FG_SKILL,
	  "the Ice Mage" },
	{ "undead25", "Destroy 25 undead, across games", 50, FG_SKILL,
	  "the Undead Slayer" },
	{ "elfdl10", "Reach dungeon level 10 as an elf", 50, FG_SKILL,
	  "the drow race" },
	{ "poly5", "Polymorph yourself five times in one game", 50, FG_SKILL,
	  "the doppelganger race" },
	{ "lycan", "Catch lycanthropy and cure it", 50, FG_SKILL,
	  "the lycanthrope race" },
	{ "vampire", "Kill a vampire", 50, FG_SKILL, "the vampire race" },
	{ "protection", "Buy protection from a temple priest", 40, FG_SKILL, 0 },
	{ "gift", "Receive a gift from sacrificing", 75, FG_SKILL, 0 },
	{ "crowned", "Be crowned", 150, FG_SKILL, 0 },
	{ "lessons10", "Learn 10 Lessons", 25, FG_KNOWLEDGE,
	  "rank I of the Ward" },
	{ "ghost", "Meet an ancestor's ghost", 50, FG_ANCESTRY,
	  "the Necromancer" },
	{ "ancestors25", "Record 25 ancestors", 25, FG_ANCESTRY, 0 },
	{ "id50", "Identify 50 object types, across games", 50, FG_KNOWLEDGE,
	  0 },
	{ "bestiary100", "Fill 100 Bestiary entries", 50, FG_KNOWLEDGE, 0 },
	{ "insight", "Earn every Insight item", 100, FG_KNOWLEDGE, 0 },
	{ "belongings", "Recover an ancestor's belongings", 30, FG_ANCESTRY,
	  "the Heir's luck blessing" },
	{ "heirloom3", "Carry one heirloom through three games", 75,
	  FG_ANCESTRY, 0 },
	{ "mastery5", "Reach mastery 5 with any role", 200, FG_ANCESTRY, 0 },
	{ "act1", "Clear Expedition Act I", 50, FG_EXPEDITION,
	  "the Hexed and Fragile pacts" },
	{ "act2", "Clear Expedition Act II", 100, FG_EXPEDITION,
	  "the Swift foes and Unwritten pacts" },
	{ "act3", "Clear Expedition Act III", 300, FG_EXPEDITION,
	  "the Starless pact" },
	{ "retire250", "Retire with a pot of 250 or more", 50, FG_EXPEDITION, 0 },
	{ "vowact1", "Keep a vow through Act I", 50, FG_EXPEDITION,
	  "the vegan, weaponless, foodless and pacifist vows" },
	{ "act1pact5", "Clear Act I at pact level 5 or higher", 100,
	  FG_EXPEDITION, 0 },
	{ "act1unaided", "Clear Act I Unaided", 100, FG_EXPEDITION, 0 },
	{ "streak7", "A seven-day Daily streak", 50, FG_DAILY, 0 },
	{ "streak30", "A thirty-day Daily streak", 150, FG_DAILY, 0 },
	{ "gauntlet10", "Reach dungeon level 10 in a Sunday Gauntlet", 75,
	  FG_DAILY, 0 },
};

/* roles and races that are not open from the start of a Legacy profile */
static const struct unlock_def {
	const char *name;	/* roles[].name.m or races[].noun */
	int feat;		/* the feat that opens it, or -1: always open */
	short cost;		/* or this much Renown */
} role_unlocks[] = {
	{ "Archeologist", F_DIG, 150 },
	{ "Caveman", F_MINETOWN, 100 },
	{ "Flame Mage", F_WIZXL10, 200 },
	{ "Healer", F_ORACLE, 200 },
	{ "Ice Mage", F_SPELLS100, 200 },
	{ "Knight", F_SOKO1, 150 },
	{ "Monk", F_XL8, 150 },
	{ "Necromancer", F_GHOST, 250 },
	{ "Rogue", F_THROWN20, 150 },
	{ "Tourist", F_DL10, 300 },
	{ "Undead Slayer", F_UNDEAD25, 200 },
	{ "Yeoman", F_PETDL8, 150 },
	{ 0, -1, 0 }
}, race_unlocks[] = {
	{ "doppelganger", F_POLY5, 200 },
	{ "drow", F_ELFDL10, 150 },
	{ "hobbit", F_SOKOBAN, 100 },
	{ "lycanthrope", F_LYCAN, 200 },
	{ "vampire", F_VAMPIRE, 250 },
	{ 0, -1, 0 }
};

static const struct landmark_def {
	const char *name;
	short renown;
} landmarks[NUM_LANDMARKS] = {
	{ "the Mines", 5 },
	{ "the Oracle", 10 },
	{ "Minetown", 10 },
	{ "Sokoban finished", 20 },
	{ "Mines' End", 20 },
	{ "the Big Room", 10 },
	{ "the Quest portal level", 15 },
	{ "Medusa killed", 50 },
	{ "the Castle", 50 },
	{ "the Valley", 75 },
	{ "ascension", 500 },
};

/* the feats a new player is pointed at first, in order */
static const int goal_order[] = {
	F_MINETOWN, F_DIG, F_ORACLE, F_SOKO1, F_XL8, F_PETDL8, F_THROWN20,
	F_DL10, F_LESSONS10, F_SOKOBAN, F_VAMPIRE, F_WIZXL10, F_UNDEAD25,
	F_ELFDL10, F_SPELLS100, F_POLY5, F_LYCAN, F_GHOST, F_MEDUSA,
	F_GEHENNOM, -1
};

STATIC_DCL const struct unlock_def *FDECL(find_unlock, (BOOLEAN_P, int));
STATIC_DCL boolean FDECL(unlock_open, (BOOLEAN_P, int));
STATIC_DCL boolean FDECL(read_profile, (const char *));
STATIC_DCL void FDECL(read_ancestor, (char *));
STATIC_DCL void FDECL(clean_field, (char *, const char *, int));
STATIC_DCL char *FDECL(next_field, (char **));
STATIC_DCL void FDECL(add_lesson, (const char *));
STATIC_DCL boolean FDECL(know_lesson, (const char *));
STATIC_DCL void FDECL(lesson_key, (char *, const char *));
STATIC_DCL void FDECL(landmark, (int));
STATIC_DCL char FDECL(soko_glyph, (int, int));
STATIC_DCL void FDECL(soko_start, (int));
STATIC_DCL void FDECL(soko_finish, (int, BOOLEAN_P));
STATIC_DCL boolean FDECL(lore_ok, (int));
STATIC_DCL boolean NDECL(first_pet_alive);
STATIC_DCL boolean NDECL(on_quest_portal_level);

/* ------------------------------------------------------------------ */
/* the profile file */

/* load the profile once per process */
void
legacy_init()
{
	if (legacy_loaded)
	    return;
	legacy_loaded = TRUE;
	(void) memset((genericptr_t) &lprof, 0, sizeof lprof);
	lprof.last_mode = LMODE_LEGACY;
	codex_init();
	if (!read_profile(LEGACY_PROFILE))
	    (void) read_profile(LEGACY_BACKUP);
	lprof.dirty = FALSE;
}

STATIC_OVL char *
next_field(p)
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

/* copy a string into a fixed-size field, dropping tabs and newlines */
STATIC_OVL void
clean_field(dst, src, size)
char *dst;
const char *src;
int size;
{
	int i;

	for (i = 0; *src && i < size - 1; src++)
	    if (*src != '\t' && *src != '\n' && *src != '\r')
		dst[i++] = *src;
	dst[i] = '\0';
}

STATIC_OVL void
read_ancestor(line)
char *line;
{
	struct legacy_ancestor *a;
	char *p = line;

	a = (struct legacy_ancestor *) alloc(sizeof (struct legacy_ancestor));
	(void) memset((genericptr_t) a, 0, sizeof *a);
	a->gen = atoi(next_field(&p));
	a->mode = atoi(next_field(&p));
	a->deepest = atoi(next_field(&p));
	a->xl = atoi(next_field(&p));
	a->turns = atol(next_field(&p));
	a->renown = atol(next_field(&p));
	a->ward_fired = !!atoi(next_field(&p));
	a->dnum = atoi(next_field(&p));
	a->dlevel = atoi(next_field(&p));
	a->when = atol(next_field(&p));
	clean_field(a->name, next_field(&p), sizeof a->name);
	clean_field(a->role, next_field(&p), sizeof a->role);
	clean_field(a->race, next_field(&p), sizeof a->race);
	clean_field(a->gender, next_field(&p), sizeof a->gender);
	clean_field(a->align, next_field(&p), sizeof a->align);
	clean_field(a->death, next_field(&p), sizeof a->death);
	clean_field(a->help, next_field(&p), sizeof a->help);
	clean_field(a->epitaph, next_field(&p), sizeof a->epitaph);
	/* the file lists the oldest first; the list is newest first */
	a->next = lprof.ancestors;
	lprof.ancestors = a;
}

STATIC_OVL boolean
read_profile(name)
const char *name;
{
	FILE *fp;
	char line[BUFSZ * 4], *key, *rest, *p;
	int i, version = 0;

	fp = fopen(fqname(name, SCOREPREFIX, 0), "r");
	if (!fp)
	    return FALSE;
	if (!fgets(line, sizeof line, fp)
		|| sscanf(line, "SlashEM-Legacy %d", &version) != 1) {
	    (void) fclose(fp);
	    return FALSE;
	}
	while (fgets(line, sizeof line, fp)) {
	    if ((p = index(line, '\n')) != 0) *p = '\0';
	    if ((p = index(line, '\r')) != 0) *p = '\0';
	    key = line;
	    if ((rest = index(line, ' ')) != 0)
		*rest++ = '\0';
	    else
		rest = "";
	    if (!strcmp(key, "renown")) {
		(void) sscanf(rest, "%ld %ld", &lprof.renown_earned,
			      &lprof.renown_spent);
	    } else if (!strcmp(key, "generation")) {
		lprof.generation = atoi(rest);
	    } else if (!strcmp(key, "lastmode")) {
		lprof.last_mode = max(0, min(atoi(rest), NUM_LMODES - 1));
	    } else if (!strcmp(key, "setting")) {
		int val = 0;
		char sname[BUFSZ];

		if (sscanf(rest, "%s %d", sname, &val) == 2) {
		    if (!strcmp(sname, "unlock_all"))
			lprof.unlock_all = !!val;
		    else if (!strcmp(sname, "skip_hall"))
			lprof.skip_hall = !!val;
		}
	    } else if (!strcmp(key, "lineage")) {
		char tname[BUFSZ];
		int owned = 0, off = 0;

		if (sscanf(rest, "%s %d %d", tname, &owned, &off) >= 2)
		    for (i = 0; i < NUM_LINEAGE; i++)
			if (!strcmp(tname, lineage_tracks[i].key)) {
			    lprof.lineage[i] = (xchar) max(0,
					min(owned, LIN_MAXRANK));
			    lprof.lin_off[i] = !!off;
			}
	    } else if (!strcmp(key, "feat")) {
		char fname[BUFSZ];
		long when = 0;

		if (sscanf(rest, "%s %ld", fname, &when) == 2)
		    for (i = 0; i < NUM_FEATS; i++)
			if (!strcmp(fname, feats[i].key))
			    lprof.feat_done[i] = when ? when : 1L;
	    } else if (!strcmp(key, "counter")) {
		char cname[BUFSZ];
		long n = 0;

		if (sscanf(rest, "%s %ld", cname, &n) == 2) {
		    if (!strcmp(cname, "spells"))
			lprof.counter[LC_SPELLS] = n;
		    else if (!strcmp(cname, "undead"))
			lprof.counter[LC_UNDEAD] = n;
		}
	    } else if (!strcmp(key, "role")) {
		for (i = 0; roles[i].name.m; i++)
		    if (!strcmp(rest, roles[i].name.m))
			lprof.roles_bought |= 1UL << i;
	    } else if (!strcmp(key, "race")) {
		for (i = 0; races[i].noun; i++)
		    if (!strcmp(rest, races[i].noun))
			lprof.races_bought |= 1UL << i;
	    } else if (!strcmp(key, "lesson")) {
		if (*rest)
		    add_lesson(rest);
	    } else if (!strcmp(key, "ancestor")) {
		read_ancestor(rest);
	    } else if (!strcmp(key, "insight")) {
		char iname[BUFSZ];
		long n = 0;

		if (sscanf(rest, "%s %ld", iname, &n) == 2)
		    for (i = 0; i < NUM_ICOUNTERS; i++)
			if (!strcmp(iname, icounter_keys[i]))
			    lprof.icount[i] = n;
	    } else if (!strcmp(key, "insightoff")) {
		for (i = 0; i < NUM_INSIGHT; i++)
		    if (!strcmp(rest, insight_keys[i]))
			lprof.ins_off[i] = TRUE;
	    } else if (!fate_read(key, rest) && !modes_read(key, rest))
		(void) codex_read(key, rest);
	    /* anything else was written by a newer version: skip it */
	}
	(void) fclose(fp);
	return TRUE;
}

/* write the profile if it changed (or always, if 'force'); the old file
   is kept as legacy.bak, so a crash while writing can't lose it */
void
legacy_write(force)
boolean force;
{
	FILE *fp;
	struct legacy_ancestor *a, **list;
	int i, n;
	char tmpname[BUFSZ], datname[BUFSZ], bakname[BUFSZ];

	if (!legacy_loaded || (!force && !lprof.dirty))
	    return;
	Strcpy(tmpname, fqname(LEGACY_TMP, SCOREPREFIX, 0));
	Strcpy(datname, fqname(LEGACY_PROFILE, SCOREPREFIX, 0));
	Strcpy(bakname, fqname(LEGACY_BACKUP, SCOREPREFIX, 0));

	fp = fopen(tmpname, "w");
	if (!fp)
	    return;
	fprintf(fp, "SlashEM-Legacy %d\n", LEGACY_VERSION);
	fprintf(fp, "renown %ld %ld\n", lprof.renown_earned,
		lprof.renown_spent);
	fprintf(fp, "generation %d\n", lprof.generation);
	fprintf(fp, "lastmode %d\n", lprof.last_mode);
	fprintf(fp, "setting unlock_all %d\n", lprof.unlock_all ? 1 : 0);
	fprintf(fp, "setting skip_hall %d\n", lprof.skip_hall ? 1 : 0);
	for (i = 0; i < NUM_LINEAGE; i++)
	    if (lprof.lineage[i] || lprof.lin_off[i])
		fprintf(fp, "lineage %s %d %d\n", lineage_tracks[i].key,
			(int) lprof.lineage[i], lprof.lin_off[i] ? 1 : 0);
	for (i = 0; i < NUM_FEATS; i++)
	    if (lprof.feat_done[i])
		fprintf(fp, "feat %s %ld\n", feats[i].key, lprof.feat_done[i]);
	fprintf(fp, "counter spells %ld\n", lprof.counter[LC_SPELLS]);
	fprintf(fp, "counter undead %ld\n", lprof.counter[LC_UNDEAD]);
	for (i = 0; roles[i].name.m; i++)
	    if (lprof.roles_bought & (1UL << i))
		fprintf(fp, "role %s\n", roles[i].name.m);
	for (i = 0; races[i].noun; i++)
	    if (lprof.races_bought & (1UL << i))
		fprintf(fp, "race %s\n", races[i].noun);
	for (i = 0; i < lprof.nlessons; i++)
	    fprintf(fp, "lesson %s\n", lprof.lessons[i]);
	for (i = 0; i < NUM_ICOUNTERS; i++)
	    if (lprof.icount[i])
		fprintf(fp, "insight %s %ld\n", icounter_keys[i],
			lprof.icount[i]);
	for (i = 0; i < NUM_INSIGHT; i++)
	    if (lprof.ins_off[i])
		fprintf(fp, "insightoff %s\n", insight_keys[i]);
	codex_write(fp);
	fate_write(fp);
	modes_write(fp);

	/* ancestors, oldest first, at most MAX_ANCESTORS of them */
	for (n = 0, a = lprof.ancestors; a; a = a->next)
	    n++;
	if (n) {
	    list = (struct legacy_ancestor **)
		alloc((unsigned) (n * sizeof (struct legacy_ancestor *)));
	    for (i = 0, a = lprof.ancestors; a; a = a->next)
		list[i++] = a;
	    for (i = min(n, MAX_ANCESTORS) - 1; i >= 0; i--) {
		a = list[i];
		fprintf(fp, "ancestor %d\t%d\t%d\t%d\t%ld\t%ld\t%d\t%d\t%d\t%ld"
			"\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
			a->gen, a->mode, a->deepest, a->xl, a->turns,
			a->renown, a->ward_fired ? 1 : 0, a->dnum, a->dlevel,
			a->when, a->name, a->role, a->race, a->gender,
			a->align, a->death, a->help, a->epitaph);
	    }
	    free((genericptr_t) list);
	}
	if (fclose(fp) != 0) {
	    (void) unlink(tmpname);
	    return;
	}
	(void) unlink(bakname);
	(void) rename(datname, bakname);
	if (rename(tmpname, datname) == 0)
	    lprof.dirty = FALSE;
}

/* ------------------------------------------------------------------ */
/* lessons: the causes of death met so far */

STATIC_OVL void
add_lesson(s)
const char *s;
{
	if (lprof.nlessons >= lprof.maxlessons) {
	    int newmax = lprof.maxlessons ? lprof.maxlessons * 2 : 32;
	    char **nl = (char **) alloc((unsigned) (newmax * sizeof (char *)));

	    if (lprof.nlessons)
		(void) memcpy((genericptr_t) nl, (genericptr_t) lprof.lessons,
			      lprof.nlessons * sizeof (char *));
	    if (lprof.lessons)
		free((genericptr_t) lprof.lessons);
	    lprof.lessons = nl;
	    lprof.maxlessons = newmax;
	}
	lprof.lessons[lprof.nlessons] = (char *) alloc((unsigned) strlen(s) + 1);
	clean_field(lprof.lessons[lprof.nlessons], s, (int) strlen(s) + 1);
	lprof.nlessons++;
}

STATIC_OVL boolean
know_lesson(s)
const char *s;
{
	int i;

	for (i = 0; i < lprof.nlessons; i++)
	    if (!strcmp(lprof.lessons[i], s))
		return TRUE;
	return FALSE;
}

/* reduce "killed by an invisible soldier ant, while helpless" to
   "killed by soldier ant", so the same cause is one Lesson */
STATIC_OVL void
lesson_key(key, death)
char *key;
const char *death;
{
	static const char *const drop[] = {
	    "a ", "an ", "the ", "invisible ", "hallucinogen-distorted ", 0
	};
	char buf[BUFSZ], *p;
	int i;
	boolean again;

	clean_field(buf, death, BUFSZ);
	/* being frozen by a floating eye is the real cause, whatever then
	   did the killing */
	if (strstri(buf, "frozen by a floating eye")) {
	    Strcpy(key, "frozen by a floating eye");
	    return;
	}
	if ((p = strstr(buf, ", while")) != 0) *p = '\0';
	if ((p = strstr(buf, " (with")) != 0) *p = '\0';
	if ((p = strstr(buf, " (in")) != 0) *p = '\0';
	for (p = buf; *p; p++)
	    *p = lowc(*p);
	p = strstr(buf, " by ");
	if (p) {
	    p += 4;
	    do {
		again = FALSE;
		for (i = 0; drop[i]; i++)
		    if (!strncmp(p, drop[i], strlen(drop[i]))) {
			(void) memmove(p, p + strlen(drop[i]),
				       strlen(p + strlen(drop[i])) + 1);
			again = TRUE;
		    }
	    } while (again);
	}
	Strcpy(key, buf);
}

void
legacy_lesson_key(key, death)
char *key;
const char *death;
{
	lesson_key(key, death);
}

/* has any Lesson so far contained this text? */
boolean
legacy_know_lesson_like(s)
const char *s;
{
	int i;

	legacy_init();
	for (i = 0; i < lprof.nlessons; i++)
	    if (strstri(lprof.lessons[i], s))
		return TRUE;
	return FALSE;
}

void
legacy_clean_field(dst, src, size)
char *dst;
const char *src;
int size;
{
	clean_field(dst, src, size);
}

/* ------------------------------------------------------------------ */
/* unlocks */

STATIC_OVL const struct unlock_def *
find_unlock(isrole, idx)
boolean isrole;
int idx;
{
	const struct unlock_def *u;
	const char *name = isrole ? roles[idx].name.m : races[idx].noun;

	if (!name)
	    return 0;
	for (u = isrole ? role_unlocks : race_unlocks; u->name; u++)
	    if (!strcmp(u->name, name))
		return u;
	return 0;
}

/* is this role (or race) open in Legacy, ignoring unlock_all? */
STATIC_OVL boolean
unlock_open(isrole, idx)
boolean isrole;
int idx;
{
	const struct unlock_def *u = find_unlock(isrole, idx);

	if (!u || u->feat < 0 || lprof.feat_done[u->feat])
	    return TRUE;
	return ((isrole ? lprof.roles_bought : lprof.races_bought)
		& (1UL << idx)) != 0;
}

/* used by role.c while a Legacy character is being chosen */
boolean
legacy_role_locked(idx)
int idx;
{
	if (!legacy_selecting || !LINEAGE_RULES() || lprof.unlock_all)
	    return FALSE;
	return !unlock_open(TRUE, idx);
}

boolean
legacy_race_locked(idx)
int idx;
{
	if (!legacy_selecting || !LINEAGE_RULES() || lprof.unlock_all)
	    return FALSE;
	return !unlock_open(FALSE, idx);
}

/* the feat that opens a locked role or race, or 0 if it is open */
const char *
legacy_unlock_text(isrole, idx)
boolean isrole;
int idx;
{
	const struct unlock_def *u = find_unlock(isrole, idx);

	if (unlock_open(isrole, idx))
	    return 0;
	return feats[u->feat].name;
}

int
legacy_unlock_cost(isrole, idx)
boolean isrole;
int idx;
{
	const struct unlock_def *u = find_unlock(isrole, idx);

	return u ? u->cost : 0;
}

boolean
legacy_buy_unlock(isrole, idx)
boolean isrole;
int idx;
{
	int cost = legacy_unlock_cost(isrole, idx);

	if (unlock_open(isrole, idx) || legacy_renown() < cost)
	    return FALSE;
	lprof.renown_spent += cost;
	if (isrole)
	    lprof.roles_bought |= 1UL << idx;
	else
	    lprof.races_bought |= 1UL << idx;
	lprof.dirty = TRUE;
	legacy_write(FALSE);
	return TRUE;
}

/* ------------------------------------------------------------------ */
/* Renown and the Lineage */

boolean
legacy_mode()
{
	return (boolean) LEGACY_RULES();
}

/* does this game pay Renown and feats?  not in Explore mode or from a
   typed-in seed */
boolean
legacy_counts()
{
	return (boolean) (!discover && !lrun.custom);
}

long
legacy_renown()
{
	return lprof.renown_earned - lprof.renown_spent;
}

const char *
legacy_lineage_name(t)
int t;
{
	return lineage_tracks[t].name;
}

/* what rank r (1..3) of track t gives */
const char *
legacy_lineage_desc(t, r)
int t, r;
{
	return lineage_tracks[t].desc[r - 1];
}

/* the price of rank r (1..3) of track t */
int
legacy_lineage_cost(t, r)
int t, r;
{
	return lineage_tracks[t].cost[r - 1];
}

/* why the next rank of a track can't be bought yet, or 0 if it can
   (Renown aside) */
const char *
legacy_lineage_blocked(t)
int t;
{
	static char buf[BUFSZ];
	int next = lprof.lineage[t] + 1;

	if (next > LIN_MAXRANK)
	    return "complete";
	if (next == 2 && lprof.renown_earned < RANK2_GATE) {
	    Sprintf(buf, "opens at %ld Renown earned", RANK2_GATE);
	    return buf;
	}
	if (next == 3 && lprof.renown_earned < RANK3_GATE) {
	    Sprintf(buf, "opens at %ld Renown earned", RANK3_GATE);
	    return buf;
	}
	if (t == LIN_WARD && !lprof.feat_done[ward_feat[next - 1]]) {
	    Sprintf(buf, "needs the feat \"%s\"", feats[ward_feat[next - 1]].name);
	    return buf;
	}
	return 0;
}

boolean
legacy_buy_lineage(t)
int t;
{
	int next = lprof.lineage[t] + 1;

	if (legacy_lineage_blocked(t)
		|| legacy_renown() < legacy_lineage_cost(t, next))
	    return FALSE;
	lprof.renown_spent += legacy_lineage_cost(t, next);
	lprof.lineage[t] = next;
	lprof.dirty = TRUE;
	legacy_write(FALSE);
	return TRUE;
}

/* can the next game be Unaided?  only if there is something to give up */
boolean
legacy_unaided_possible()
{
	int t;

	for (t = 0; t < NUM_LINEAGE; t++)
	    if (LIN_OPEN(t) && lprof.lineage[t])
		return TRUE;
	return FALSE;
}

/* fill in the run state for a game about to start; lrun.mode is set */
void
legacy_setup_run()
{
	int t, mode = lrun.mode;
	unsigned long seed = lrun.seed;
	boolean any_owned = FALSE, any_on = FALSE, custom = lrun.custom;

	(void) memset((genericptr_t) &lrun, 0, sizeof lrun);
	lrun.mode = mode;
	lrun.seed = seed;
	lrun.custom = custom;
	if (!LINEAGE_RULES())
	    return;
	for (t = 0; t < NUM_LINEAGE; t++) {
	    if (!LIN_OPEN(t) || !lprof.lineage[t])
		continue;
	    any_owned = TRUE;
	    if (!lprof.lin_off[t]) {
		lrun.lineage[t] = lprof.lineage[t];
		any_on = TRUE;
	    }
	}
	lrun.unaided = (any_owned && !any_on);
	fate_setup_run();	/* vows, kit and heirloom; may clear unaided */
	modes_setup_run();	/* pacts */
}

/* the help a game has, such as "Vigor II, Ward I" */
void
legacy_help_text(buf, shortform)
char *buf;
boolean shortform;
{
	static const char *const roman[] = { "", "I", "II", "III" };
	int t;

	*buf = '\0';
	if (!LEGACY_RULES()) {
	    Strcpy(buf, discover ? "Explore mode" : "Classic");
	    return;
	}
	if (lrun.mode == LMODE_DAILY)
	    Strcpy(buf, lrun.gauntlet ? "Daily Gauntlet" : "Daily");
	else if (lrun.unaided)
	    Strcpy(buf, shortform ? "Unaided" : "Unaided (+25% Renown)");
	else
	    for (t = 0; t < NUM_LINEAGE; t++)
		if (lrun.lineage[t])
		    Sprintf(eos(buf), "%s%s %s", *buf ? ", " : "",
			    lineage_tracks[t].name, roman[lrun.lineage[t]]);
	/* blessings, kit, heirloom and vows; kept short enough for BUFSZ */
	fate_help_text(buf, BUFSZ - 40);
	if (pact_level() && strlen(buf) < BUFSZ - 60)
	    Sprintf(eos(buf), "%spact level %d", *buf ? ", " : "",
		    pact_level());
	if (lrun.custom)
	    Sprintf(eos(buf), "%scustom seed", *buf ? ", " : "");
	if (!*buf)
	    Strcpy(buf, "no Lineage yet");
	if (discover)
	    Strcat(buf, " (Explore mode)");
}

/* a "lawful female dwarven Valkyrie" description */
void
legacy_hero_desc(buf, role, race, gend, algn)
char *buf;
int role, race, gend, algn;
{
	Sprintf(buf, "%s %s %s %s",
		(algn >= 0 && algn < ROLE_ALIGNS) ? aligns[algn].adj : "?",
		(gend >= 0 && gend < ROLE_GENDERS) ? genders[gend].adj : "?",
		(race >= 0 && races[race].noun) ? races[race].adj : "?",
		(role < 0 || !roles[role].name.m) ? "?" :
		(gend == 1 && roles[role].name.f) ? roles[role].name.f :
		roles[role].name.m);
}

/* ------------------------------------------------------------------ */
/* the start of a game */

/* may Lore make this object type known? */
STATIC_OVL boolean
lore_ok(otyp)
int otyp;
{
	if (!OBJ_DESCR(objects[otyp]) || objects[otyp].oc_name_known
		|| objects[otyp].oc_prob <= 0)
	    return FALSE;
	switch (otyp) {
	case POT_WATER:
	case SCR_BLANK_PAPER:
#ifdef MAIL
	case SCR_MAIL:
#endif
	case WAN_NOTHING:
	    return FALSE;
	}
	return TRUE;
}

/* make a random object type of one class known, one not known yet */
void
legacy_know_random(oclass)
int oclass;
{
	int otyp, first = bases[oclass], last = bases[oclass + 1] - 1;
	int n = 0;

	for (otyp = first; otyp <= last; otyp++)
	    if (objects[otyp].oc_class == oclass && lore_ok(otyp))
		n++;
	if (!n)
	    return;
	n = rn2(n);
	for (otyp = first; otyp <= last; otyp++)
	    if (objects[otyp].oc_class == oclass && lore_ok(otyp) && !n--) {
		discover_object(otyp, TRUE, FALSE);
		break;
	    }
}

/* called from newgame() once the hero, the first level and the pet exist */
void
legacy_newgame(pet)
struct monst *pet;
{
	struct obj *otmp;
	int r;

	legacy_init();
	legacy_selecting = FALSE;
	lrun.gen = ++lprof.generation;
	lprof.last_mode = lrun.mode;
	lprof.dirty = TRUE;
	/* where Insight and the Codex stood, for the recap */
	lrun.ins_had = insight_mask();
	lrun.ins_told = lrun.ins_had;
	for (r = 0; r < NUM_ICOUNTERS; r++)
	    lrun.ic_start[r] = lprof.icount[r];
	lrun.kills_start = codex_total_kills();
	lrun.ids_start = codex_armory_count();
	if (pet)
	    lrun.first_pet = pet->m_id;
	lrun.inv_made = FALSE;	/* what Legacy adds is never Hexed */
	if (LEGACY_RULES()) {
	    fate_kit();		/* kit B or C, before Arms improves it */
	    if ((r = lrun.lineage[LIN_VIGOR]) > 0) {
		u.uhpmax += 2 * r;
		u.uhp = u.uhpmax;
	    }
	    lineage_inv();	/* Provisions, Remedies and Escape */
	    if (lrun.lineage[LIN_PATRON] >= 1)
		u.ublesscnt = 0;
	    if (lrun.lineage[LIN_PATRON] >= 3)
		u.uluck++;	/* and timeout.c keeps it from fading */
	    if ((r = lrun.lineage[LIN_LORE]) > 0) {
		legacy_know_random(POTION_CLASS);
		if (r >= 2) legacy_know_random(SCROLL_CLASS);
		if (r >= 3) {
		    legacy_know_random(WAND_CLASS);
		    legacy_know_random(RING_CLASS);
		}
	    }
	    if ((r = lrun.lineage[LIN_COMPANION]) > 0 && pet) {
		int i;

		for (i = 0; i < min(r, 2); i++)
		    (void) grow_up(pet, (struct monst *) 0);
		pet->mhp = pet->mhpmax;
		if (r >= 3)
		    (void) makedog();
	    }
	    if ((r = lrun.lineage[LIN_ARMS]) > 0) {
		if (uwep && (uwep->oclass == WEAPON_CLASS || is_weptool(uwep)))
		    uwep->spe += (r >= 3) ? 2 : 1;
		if (r >= 2) {
		    otmp = uarm ? uarm : uarmc;
		    if (otmp) {
			otmp->spe++;
			find_ac();
		    }
		}
		if (r >= 3)
		    for (otmp = invent; otmp; otmp = otmp->nobj)
			if (otmp->oclass == WEAPON_CLASS
				|| otmp->oclass == ARMOR_CLASS
				|| is_weptool(otmp))
			    otmp->oerodeproof = 1;
	    }
	    fate_newgame(pet);	/* blessings, heirloom and vows */
	    flags.botl = 1;
	}
	modes_newgame();	/* the Daily streak */
	lrun.inv_made = TRUE;
	legacy_write(FALSE);
}

/* the line after the welcome message */
void
legacy_welcome()
{
	char buf[BUFSZ];

	int t;

	modes_welcome();
	if (!LEGACY_RULES())
	    return;
	if (lrun.unaided)
	    pline("You set out unaided, for 25%% more Renown.");
	else {
	    for (*buf = '\0', t = 0; t < NUM_LINEAGE; t++)
		if (lrun.lineage[t])
		    Sprintf(eos(buf), "%s%s %s", *buf ? ", " : "",
			    lineage_tracks[t].name,
			    lrun.lineage[t] == 3 ? "III" :
			    lrun.lineage[t] == 2 ? "II" : "I");
	    if (*buf)
		pline("Your lineage gives you: %s.", buf);
	}
	lrun.inv_made = FALSE;
	fate_welcome();	/* blessings, heirloom, vows, mastery */
	lrun.inv_made = TRUE;
}

/* ------------------------------------------------------------------ */
/* feats and landmarks */

const char *
legacy_feat_name(f)
int f;
{
	return feats[f].name;
}

int
legacy_feat_renown(f)
int f;
{
	return feats[f].renown;
}

const char *
legacy_feat_unlocks(f)
int f;
{
	return feats[f].unlocks;
}

int
legacy_feat_group(f)
int f;
{
	return feats[f].group;
}

int
legacy_feats_done()
{
	int i, n = 0;

	for (i = 0; i < NUM_FEATS; i++)
	    if (lprof.feat_done[i])
		n++;
	return n;
}

int
legacy_ancestor_count()
{
	struct legacy_ancestor *a;
	int n = 0;

	for (a = lprof.ancestors; a; a = a->next)
	    n++;
	return n;
}

/* a feat happened; it pays once, ever */
void
legacy_feat(f)
int f;
{
	if (f < 0 || f >= NUM_FEATS || !legacy_counts() || !legacy_loaded)
	    return;
	if (lprof.feat_done[f])
	    return;
	lprof.feat_done[f] = yyyymmdd((time_t) 0);
	lprof.renown_earned += feats[f].renown;
	lrun.feats[f / 32] |= 1L << (f % 32);
	lrun.feat_renown += feats[f].renown;
	lprof.dirty = TRUE;
	if (!program_state.gameover && iflags.window_inited) {
	    You("earned a feat: %s (+%d Renown).", feats[f].name,
		(int) feats[f].renown);
	    if (feats[f].unlocks)
		pline("That opens %s in Legacy.", feats[f].unlocks);
	}
	livelog_printf(LL_MINORAC, "earned the feat \"%s\"", feats[f].name);
}

/* a counter kept across games went up */
void
legacy_count(c)
int c;
{
	if (discover || !legacy_loaded)
	    return;
	lprof.counter[c]++;
	lprof.dirty = TRUE;
	if (c == LC_SPELLS && lprof.counter[c] >= 100)
	    legacy_feat(F_SPELLS100);
	else if (c == LC_UNDEAD && lprof.counter[c] >= 25)
	    legacy_feat(F_UNDEAD25);
}

STATIC_OVL void
landmark(lm)
int lm;
{
	lrun.landmarks |= 1L << lm;
}

STATIC_OVL boolean
first_pet_alive()
{
	struct monst *mtmp;

	if (!lrun.first_pet)
	    return FALSE;
	for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
	    if (mtmp->m_id == lrun.first_pet && !DEADMONSTER(mtmp)
		    && mtmp->mtame)
		return TRUE;
	return (u.usteed && u.usteed->m_id == lrun.first_pet);
}

STATIC_OVL boolean
on_quest_portal_level()
{
	branch *br = Is_branchlev(&u.uz);

	if (!br || br->type != BR_PORTAL)
	    return FALSE;
	return (on_level(&br->end1, &u.uz) && br->end2.dnum == quest_dnum)
	    || (on_level(&br->end2, &u.uz) && br->end1.dnum == quest_dnum);
}

/* the hero arrived on a level for the first time */
void
legacy_arrived(newbranch)
boolean newbranch;
{
	static const struct {
	    const char *dname;
	    int feat;
	} branch_feats[] = {
	    { "Grund's Stronghold", F_GRUND },
	    { "The Wyrm Caves", F_WYRM },
	    { "One-eyed Sam's Market", F_SAM },
	    { "The Lost Tomb", F_TOMB },
	    { "The Spider Caves", F_SPIDERS },
	    { "The Sunless Sea", F_SUNLESS },
	    { "The Temple of Moloch", F_MOLOCH },
	    { "Fort Ludios", F_LUDIOS },
	    { "Frankenstein's Lab", F_FRANKEN },
	    { 0, 0 }
	};
	s_level *slev = Is_special(&u.uz);
	int dep = depth(&u.uz), i;

	if (In_mines(&u.uz)) {
	    landmark(LM_MINES);
	    legacy_feat(F_MINES);
	}
	if (Is_oracle_level(&u.uz))
	    landmark(LM_ORACLE);
	if (slev && slev->flags.town && In_mines(&u.uz)) {
	    landmark(LM_MINETOWN);
	    legacy_feat(F_MINETOWN);
	}
	if (slev && !strcmp(slev->proto, "minend"))
	    landmark(LM_MINESEND);
	if (Is_bigroom(&u.uz)) {
	    landmark(LM_BIGROOM);
	    legacy_feat(F_BIGROOM);
	}
	if (on_quest_portal_level()) {
	    landmark(LM_QPORTAL);
	    legacy_feat(F_QPORTAL);
	    fate_mastery(3);
	}
	if (Is_stronghold(&u.uz)) {
	    landmark(LM_CASTLE);
	    legacy_feat(F_CASTLE);
	}
	if (Is_valley(&u.uz))
	    landmark(LM_VALLEY);
	if (In_hell(&u.uz))
	    legacy_feat(F_GEHENNOM);
	if (In_V_tower(&u.uz))
	    legacy_feat(F_VLAD);
	if (newbranch)
	    for (i = 0; branch_feats[i].dname; i++)
		if (!strcmp(dungeons[u.uz.dnum].dname, branch_feats[i].dname))
		    legacy_feat(branch_feats[i].feat);
	if (!In_endgame(&u.uz)) {
	    if (dep >= 3)
		legacy_feat(F_DL3);
	    if (dep >= 10) {
		legacy_feat(F_DL10);
		if (Race_if(PM_ELF))
		    legacy_feat(F_ELFDL10);
	    }
	    if (dep >= 8 && first_pet_alive())
		legacy_feat(F_PETDL8);
	}

	codex_arrived();	/* the Atlas */
	fate_arrived();		/* mastery and the Cartographer */
	modes_arrived(dep);	/* the Daily's depth */
	if (In_sokoban(&u.uz))
	    legacy_soko_arrived();	/* Sokoban memory */

	/* bones left by one of this profile's own characters */
	if (lrun.ghost_ledger == -1) {
	    struct legacy_ancestor *a;

	    lrun.ghost_ledger = 0;
	    for (a = lprof.ancestors; a; a = a->next)
		if (!strcmp(a->name, lrun.ghost_name)
			&& a->dnum == u.uz.dnum && a->dlevel == u.uz.dlevel) {
		    pline("This is where %s the %s fell.", a->name, a->role);
		    lrun.ghost_ledger = ledger_no(&u.uz);
		    lrun.anc_ledger = lrun.ghost_ledger;
		    break;
		}
	}
	legacy_write(FALSE);
}

/* getbones() loaded a bones level; its ghost, if any, carries the name of
   the character who died there */
void
legacy_bones_loaded(ghostname)
const char *ghostname;
{
	lrun.ghost_ledger = ghostname ? -1 : 0;
	clean_field(lrun.ghost_name, ghostname ? ghostname : "",
		    sizeof lrun.ghost_name);
}

/* a monster died; byhero: the hero killed it */
void
legacy_mon_died(mtmp, byhero)
struct monst *mtmp;
boolean byhero;
{
	struct permonst *ptr = mtmp->data;

	if (ptr == &mons[PM_MEDUSA]) {
	    landmark(LM_MEDUSA);
	    legacy_feat(F_MEDUSA);
	    fate_mastery(4);
	    modes_act_cleared(2);
	}
	if (!byhero || mtmp->mtame)
	    return;
	codex_killed(ptr);
	if (is_undead(ptr))
	    legacy_count(LC_UNDEAD);
	if (ptr->mlet == S_VAMPIRE)
	    legacy_feat(F_VAMPIRE);
	if (legacy_throwing && ++lrun.thrown_kills >= 20)
	    legacy_feat(F_THROWN20);
}

/* ------------------------------------------------------------------ */
/* Sokoban memory: record each level as first seen and the pushes made,
   so a layout got through can be shown again in the Codex */

STATIC_OVL char
soko_glyph(x, y)
int x, y;
{
	struct trap *t = t_at(x, y);
	int glyph;

	if (t && (t->ttyp == HOLE || t->ttyp == TRAPDOOR || t->ttyp == PIT
		  || t->ttyp == SPIKED_PIT))
	    return '^';
	if (levl[x][y].typ == STONE)
	    return ' ';
	/* the default (plain text) symbol for the map feature */
	glyph = back_to_glyph(x, y);
	return glyph_is_cmap(glyph) ? defsyms[glyph_to_cmap(glyph)].sym : '.';
}

/* first arrival on Sokoban level 'lev': draw it, letter its boulders */
STATIC_OVL void
soko_start(lev)
int lev;
{
	int x, y, x0 = COLNO, x1 = 0, y0 = ROWNO, y1 = 0, nb = 0;
	struct obj *otmp;

	lrun.soko_state[lev] = SOKO_RECORDING;
	lrun.soko_nlog[lev] = 0;
	for (x = 1; x < COLNO; x++)
	    for (y = 0; y < ROWNO; y++)
		if (levl[x][y].typ != STONE) {
		    if (x < x0) x0 = x;
		    if (x > x1) x1 = x;
		    if (y < y0) y0 = y;
		    if (y > y1) y1 = y;
		}
	if (x1 < x0 || x1 - x0 + 1 >= SOKO_COLS || y1 - y0 + 1 > SOKO_ROWS) {
	    lrun.soko_state[lev] = SOKO_LOST;
	    return;
	}
	(void) memset((genericptr_t) lrun.soko_map[lev], 0,
		      sizeof lrun.soko_map[lev]);
	for (y = y0; y <= y1; y++) {
	    char *row = lrun.soko_map[lev][y - y0];

	    for (x = x0; x <= x1; x++) {
		char c = soko_glyph(x, y);

		if (x == u.ux && y == u.uy)
		    c = '@';
		else if ((otmp = sobj_at(BOULDER, x, y)) != 0) {
		    if (nb < SOKO_MAXB) {
			lrun.soko_bx[lev][nb] = x;
			lrun.soko_by[lev][nb] = y;
			c = (nb < 26) ? 'A' + nb : 'a' + nb - 26;
		    } else
			lrun.soko_state[lev] = SOKO_LOST;
		    nb++;
		}
		row[x - x0] = c;
	    }
	    /* no trailing blanks */
	    for (x = x1 - x0; x >= 0 && row[x] == ' '; x--)
		row[x] = '\0';
	}
	lrun.soko_nb[lev] = (uchar) min(nb, SOKO_MAXB);
}

/* the hero pushes the boulder at sx,sy to rx,ry on a Sokoban level */
void
legacy_soko_push(sx, sy, rx, ry)
int sx, sy, rx, ry;
{
	int lev = dunlev(&u.uz), i, dir;
	struct trap *t = t_at(rx, ry);
	boolean fills;

	if (lev < 1 || lev >= SOKO_LEVS
		|| lrun.soko_state[lev] != SOKO_RECORDING)
	    return;
	for (i = 0; i < lrun.soko_nb[lev]; i++)
	    if (lrun.soko_bx[lev][i] == sx && lrun.soko_by[lev][i] == sy)
		break;
	if (i >= lrun.soko_nb[lev] || lrun.soko_nlog[lev] >= SOKO_LOG) {
	    lrun.soko_state[lev] = SOKO_LOST;
	    return;
	}
	dir = (rx < sx) ? 0 : (rx > sx) ? 1 : (ry < sy) ? 2 : 3;
	fills = (t && (t->ttyp == HOLE || t->ttyp == TRAPDOOR
		       || t->ttyp == PIT || t->ttyp == SPIKED_PIT));
	lrun.soko_log[lev][lrun.soko_nlog[lev]++] =
	    (uchar) ((i << 3) | (dir << 1) | (fills ? 1 : 0));
	if (fills)
	    lrun.soko_bx[lev][i] = lrun.soko_by[lev][i] = -1;
	else
	    lrun.soko_bx[lev][i] = rx, lrun.soko_by[lev][i] = ry;
}

/* Sokoban level 'lev' was got through: hand its pushes to the Codex;
   'solved' if every hole was filled */
STATIC_OVL void
soko_finish(lev, solved)
int lev;
boolean solved;
{
	static const char *const dirname[4] = { "left", "right", "up", "down" };
	char name[20], buf[BUFSZ], *map[SOKO_ROWS], *steps[SOKO_LOG];
	int nmap = 0, nsteps = 0, k, n, i, j;

	if (lev < 1 || lev >= SOKO_LEVS || !lrun.soko_layout[lev]
		|| lrun.soko_state[lev] == SOKO_UNSEEN
		|| lrun.soko_state[lev] == SOKO_DONE)
	    return;
	if (!lrun.soko_nlog[lev]) {	/* got past it without a push */
	    lrun.soko_state[lev] = SOKO_DONE;
	    return;
	}
	Sprintf(name, "soko%d-%d", lev, (int) lrun.soko_layout[lev]);
	for (i = 0; i < SOKO_ROWS && lrun.soko_map[lev][i][0]; i++)
	    map[nmap++] = lrun.soko_map[lev][i];
	/* one step per run of pushes on the same boulder, such as
	   "3. B: left 2, down 1 (fills a hole)" */
	n = lrun.soko_nlog[lev];
	for (k = 0; k < n; ) {
	    int b = lrun.soko_log[lev][k] >> 3;
	    boolean filled = FALSE;

	    Sprintf(buf, "%2d. %c:", nsteps + 1, (b < 26) ? 'A' + b : 'a' + b - 26);
	    while (k < n && (lrun.soko_log[lev][k] >> 3) == b) {
		int d = (lrun.soko_log[lev][k] >> 1) & 3;

		for (j = 0; k < n && lrun.soko_log[lev][k] >> 3 == b
			    && ((lrun.soko_log[lev][k] >> 1) & 3) == d; k++, j++)
		    if (lrun.soko_log[lev][k] & 1)
			filled = TRUE;
		if (strlen(buf) < BUFSZ - 40)
		    Sprintf(eos(buf), "%s %s %d",
			    (*(eos(buf) - 1) == ':') ? "" : ",", dirname[d], j);
	    }
	    if (filled)
		Strcat(buf, " (fills a hole)");
	    steps[nsteps] = (char *) alloc((unsigned) strlen(buf) + 1);
	    Strcpy(steps[nsteps], buf);
	    nsteps++;
	}
	codex_soko_remember(name, map, nmap, steps, nsteps,
			    lrun.soko_state[lev] == SOKO_LOST ? SQ_LOST
			    : solved ? SQ_SOLVED : SQ_PARTIAL);
	for (i = 0; i < nsteps; i++)
	    free((genericptr_t) steps[i]);
	lrun.soko_state[lev] = SOKO_DONE;
}

/* the n-th Sokoban level (counted from the entrance) was finished */
void
legacy_soko_solved(n)
int n;
{
	lrun.soko_solved++;
	legacy_feat(F_SOKO1);
	soko_finish(dunlev(&u.uz), TRUE);
	if (n >= dunlevs_in_dungeon(&u.uz)) {
	    landmark(LM_SOKOBAN);
	    legacy_feat(F_SOKOBAN);
	}
}

/* mkmaze() built a Sokoban level from layout 'proto', such as "soko4-2" */
void
legacy_soko_layout(proto)
const char *proto;
{
	int lev, var;

	if (sscanf(proto, "soko%d-%d", &lev, &var) == 2 && lev >= 1
		&& lev < SOKO_LEVS)
	    lrun.soko_layout[lev] = (xchar) var;
}

/* the hero has arrived on a Sokoban level */
void
legacy_soko_arrived()
{
	int lev = dunlev(&u.uz);

	if (lev < 1 || lev >= SOKO_LEVS)
	    return;
	if (lrun.soko_state[lev] == SOKO_UNSEEN)
	    soko_start(lev);
	/* reaching it from below means the level below was got through,
	   whatever was left unfilled */
	if (lev + 1 < SOKO_LEVS)
	    soko_finish(lev + 1, FALSE);
}

/* the hero picked up an object */
void
legacy_got_object(obj)
struct obj *obj;
{
	s_level *slev;

	if (obj->otyp == LUCKSTONE && (slev = Is_special(&u.uz)) != 0
		&& !strcmp(slev->proto, "minend")) {
	    legacy_feat(F_LUCKSTONE);
	    modes_act_cleared(1);	/* Expedition Act I */
	} else if (obj->otyp == WAN_WISHING) {
	    legacy_feat(F_WISHWAND);
	    if (Is_stronghold(&u.uz))
		modes_act_cleared(3);
	}
	else if (obj->otyp == AMULET_OF_YENDOR)
	    legacy_feat(F_AMULET);
	fate_got_object(obj);
}

void
legacy_polymorphed()
{
	if (++lrun.polys >= 5)
	    legacy_feat(F_POLY5);
}

void
legacy_xlevel(lev)
int lev;
{
	if (lev >= 8)
	    legacy_feat(F_XL8);
	if (lev >= 10 && Role_if(PM_WIZARD))
	    legacy_feat(F_WIZXL10);
}

/* once per turn */
void
legacy_turn()
{
	struct monst *mtmp;

	/* the Bestiary: every monster the hero can see */
	if (!Hallucination)
	    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
		if (!DEADMONSTER(mtmp) && canspotmon(mtmp)
			&& !mtmp->m_ap_type && !mtmp->mundetected)
		    codex_saw(mtmp->data);
	insight_turn();
	fate_turn();	/* vows and timed blessings */
	modes_turn();	/* an Expedition act's choice */

	/* lycanthropy caught and later cured (the race doesn't count) */
	if (!Race_if(PM_HUMAN_WEREWOLF)) {
	    if (u.ulycn >= LOW_PM)
		lrun.lycan_caught = TRUE;
	    else if (lrun.lycan_caught) {
		lrun.lycan_caught = FALSE;
		legacy_feat(F_LYCAN);
	    }
	}
	/* an ancestor's ghost on this level */
	if (lrun.ghost_ledger > 0 && lrun.ghost_ledger == ledger_no(&u.uz))
	    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
		if (mtmp->data == &mons[PM_GHOST] && !DEADMONSTER(mtmp)
			&& mtmp->mnamelth && canspotmon(mtmp)
			&& !strcmp(NAME(mtmp), lrun.ghost_name)) {
		    legacy_feat(F_GHOST);
		    lrun.ghost_ledger = 0;
		    break;
		}
}

/* ------------------------------------------------------------------ */
/* the Ward */

/* should the Ward save the hero from this death?  If so it is used up */
boolean
legacy_ward(how)
int how;
{
	int rank = lrun.lineage[LIN_WARD];

	if (!LINEAGE_RULES() || !rank || lrun.ward_used
		|| how >= GENOCIDED)
	    return FALSE;
	if ((rank == 1 && depth(&u.uz) > 4) || (rank == 2 && depth(&u.uz) > 10))
	    return FALSE;
	lrun.ward_used = TRUE;
	lrun.ward_turn = moves;
	return TRUE;
}

/* after the Ward fired: back to the level's up staircase, unless the
   code that killed the hero is about to move them itself */
void
legacy_ward_move(how)
int how;
{
	xchar x = xupstair, y = yupstair;
	struct monst *mtmp;

	if (!x && sstairs.sx && sstairs.up) {
	    x = sstairs.sx;
	    y = sstairs.sy;
	}
	if (!x) {
	    x = xupladder;
	    y = yupladder;
	}
	if (!x || u.uswallow || how == DROWNING
		|| (how == BURNING && is_lava(u.ux, u.uy))
		|| (x == u.ux && y == u.uy))
	    return;
	if ((mtmp = m_at(x, y)) != 0) {
	    mnexto(mtmp);
	    if (m_at(x, y))
		return;
	}
	teleds(x, y, FALSE);
	You("are back at the stairs where you came in.");
}

/* ------------------------------------------------------------------ */
/* the end of a game */

/* work out what the game paid and record its ancestor */
void
legacy_game_over(how, death)
int how;
const char *death;
{
	struct legacy_ancestor *a;
	char key[BUFSZ];
	int lm, mode = lrun.mode;

	legacy_init();
	(void) memset((genericptr_t) &lres, 0, sizeof lres);
	if (how == PANICKED || how == TRICKED)
	    return;
	/* a game quit before it started isn't worth an ancestor */
	if (how == QUIT && moves < 100)
	    return;

	if (how == ASCENDED) {
	    landmark(LM_ASCENDED);
	    legacy_feat(F_ASCEND);
	}
	lres.valid = TRUE;
	clean_field(lres.death, death, sizeof lres.death);
	lres.deepest = deepest_lev_reached(FALSE);
	lres.cap = 3 * max(u.ulevel, u.ulevelmax) + 3;
	lres.credited = min(lres.deepest, lres.cap);
	lres.depth_pts = (long) lres.credited * RENOWN_PER_LEVEL;
	for (lm = 0; lm < NUM_LANDMARKS; lm++)
	    if (lrun.landmarks & (1L << lm)) {
		lres.landmark_pts += landmarks[lm].renown;
		Sprintf(eos(lres.landmarks), "%s%s",
			*lres.landmarks ? ", " : "", landmarks[lm].name);
	    }
	lres.soko_pts = (long) lrun.soko_solved * 5;

	/* the Lesson: the first death by each cause pays */
	if (how < GENOCIDED) {
	    lesson_key(key, death);
	    if (know_lesson(key))
		lres.lesson = 2;
	    else {
		lres.lesson = 1;
		lres.lesson_pts = RENOWN_LESSON;
		if (!discover) {
		    add_lesson(key);
		    if (lprof.nlessons >= 10)
			legacy_feat(F_LESSONS10);
		}
	    }
	}

	modes_game_over(how);	/* pacts, the act pot and the Daily */
	lres.subtotal = lres.depth_pts + lres.landmark_pts + lres.soko_pts
			+ lres.lesson_pts + lres.pot_pts;
	lres.vow_pct = fate_vow_bonus();
	lres.mult = 100 + (lrun.unaided ? 25 : 0) + lres.vow_pct
		    + lres.pact_pct;
	if (lres.mult > 300)
	    lres.mult = 300;
	lres.total = lres.subtotal * lres.mult / 100;
	if (mode == LMODE_DAILY)	/* a flat reward, or none for practice */
	    lres.total = lres.daily_pts;
	if (!legacy_counts() || mode == LMODE_DAILY)
	    lres.total = legacy_counts() ? lres.total : 0;
	else if (how != QUIT && lres.total < RENOWN_MINIMUM) {
	    lres.total = RENOWN_MINIMUM;
	    lres.floored = TRUE;
	}
	lprof.renown_earned += lres.total;

	/* the ancestor */
	a = (struct legacy_ancestor *) alloc(sizeof (struct legacy_ancestor));
	(void) memset((genericptr_t) a, 0, sizeof *a);
	a->gen = lrun.gen ? lrun.gen : lprof.generation;
	a->mode = mode;
	a->deepest = lres.deepest;
	a->xl = u.ulevel;
	a->turns = moves;
	a->ward_fired = lrun.ward_used;
	a->dnum = u.uz.dnum;
	a->dlevel = u.uz.dlevel;
	a->when = yyyymmdd((time_t) 0);
	clean_field(a->name, plname, sizeof a->name);
	clean_field(a->role, (flags.female && urole.name.f) ? urole.name.f
			     : urole.name.m, sizeof a->role);
	clean_field(a->race, urace.adj, sizeof a->race);
	clean_field(a->gender, genders[flags.female ? 1 : 0].adj,
		    sizeof a->gender);
	clean_field(a->align, aligns[flags.initalign >= 0 ? flags.initalign
			      : 0].adj, sizeof a->align);
	clean_field(a->death, death, sizeof a->death);
	legacy_help_text(key, TRUE);
	if (lrun.ward_used)
	    Strcat(key, "; the Ward fired");
	clean_field(a->help, key, sizeof a->help);
	a->next = lprof.ancestors;
	lprof.ancestors = a;
	if (legacy_ancestor_count() >= 25)
	    legacy_feat(F_ANCESTORS25);
	a->renown = lres.total + lrun.feat_renown;

	lprof.dirty = TRUE;
	legacy_write(TRUE);
}

/* the epitaph of the newest ancestor */
void
legacy_set_epitaph(text)
const char *text;
{
	if (!lprof.ancestors)
	    return;
	clean_field(lprof.ancestors->epitaph, text,
		    sizeof lprof.ancestors->epitaph);
	lprof.dirty = TRUE;
	legacy_write(FALSE);
}

/* up to 'max' short lines naming the nearest things to aim for */
int
legacy_next_goals(goals, maxgoals)
char goals[][BUFSZ];
int maxgoals;
{
	int n = 0, t, best = -1, i, f;
	long have = legacy_renown();

	/* the cheapest Lineage rank on offer */
	for (t = 0; t < NUM_LINEAGE; t++)
	    if (LIN_OPEN(t) && !legacy_lineage_blocked(t)
		    && (best < 0 || legacy_lineage_cost(t, lprof.lineage[t] + 1)
			< legacy_lineage_cost(best, lprof.lineage[best] + 1)))
		best = t;
	if (best >= 0 && n < maxgoals) {
	    static const char *const roman[] = { "", "I", "II", "III" };
	    int r = lprof.lineage[best] + 1, cost = legacy_lineage_cost(best, r);

	    if (have >= cost)
		Sprintf(goals[n++], "Buy %s %s in the Lineage (%d Renown)",
			lineage_tracks[best].name, roman[r], cost);
	    else
		Sprintf(goals[n++], "Earn %ld more Renown for %s %s",
			cost - have, lineage_tracks[best].name, roman[r]);
	}
	/* feats that open something */
	for (i = 0; goal_order[i] >= 0 && n < maxgoals; i++) {
	    f = goal_order[i];
	    if (lprof.feat_done[f] || !feats[f].unlocks)
		continue;
	    if (lprof.unlock_all && strncmp(feats[f].unlocks, "rank", 4))
		continue;
	    Sprintf(goals[n++], "%s to open %s", feats[f].name,
		    feats[f].unlocks);
	}
	return n;
}

/* ------------------------------------------------------------------ */
/* the game's part, saved with the game */

void
legacy_save_run_fd(fd)
int fd;
{
	bwrite(fd, (genericptr_t) &lrun, sizeof lrun);
	legacy_write(FALSE);
}

void
legacy_restore_run_fd(fd)
int fd;
{
	legacy_init();
	mread(fd, (genericptr_t) &lrun, sizeof lrun);
}

/*legacy.c*/
