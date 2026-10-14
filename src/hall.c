/*	SCCS Id: @(#)hall.c	0.0.7E7F3	2026/10/13	*/
/* NetHack may be freely redistributed.  See license for details. */

/*
 * Slash'EM Legacy: the Hall, the Prepare screen and the end-of-game
 * recap.  Everything here is built from the game's ordinary menus and
 * text windows, which the Android front end shows as tappable lists.
 */

#include "hack.h"

static boolean hall_active = FALSE;
static int opt_role, opt_race, opt_gend, opt_align;	/* from the options */

#define NUM_HEIRS	3

STATIC_DCL boolean FDECL(prepare_legacy, (BOOLEAN_P));
STATIC_DCL boolean NDECL(choose_own);
STATIC_DCL boolean FDECL(random_hero, (int *));
STATIC_DCL void NDECL(lineage_toggles);
STATIC_DCL void NDECL(lineage_shop);
STATIC_DCL void NDECL(unlock_shop);
STATIC_DCL void NDECL(show_feats);
STATIC_DCL void NDECL(show_ancestors);
STATIC_DCL void FDECL(show_ancestor, (struct legacy_ancestor *));
STATIC_DCL void NDECL(hall_settings);
STATIC_DCL int FDECL(read_restart, (int *));
STATIC_DCL void FDECL(write_restart, (const char *));
STATIC_DCL void FDECL(add_text, (winid, const char *));
STATIC_DCL void FDECL(add_choice, (winid, int, CHAR_P, const char *));
STATIC_DCL void FDECL(set_hero, (int *));
STATIC_DCL void FDECL(money_line, (char *, const char *, long));
STATIC_DCL void FDECL(recap_knowledge, (winid));

boolean
legacy_hall_active()
{
	return hall_active;
}

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

/* set flags.initrole etc. from a role, race, gender, alignment array */
STATIC_OVL void
set_hero(h)
int *h;
{
	flags.initrole = h[0];
	flags.initrace = h[1];
	flags.initgend = h[2];
	flags.initalign = h[3];
}

/* a random character among the open roles and races */
STATIC_OVL boolean
random_hero(h)
int *h;
{
	int tries, nroles;

	for (nroles = 0; roles[nroles].name.m; nroles++)
	    continue;
	for (tries = 0; tries < 200; tries++) {
	    h[0] = rn2(nroles);
	    if (!ok_role(h[0], ROLE_NONE, ROLE_NONE, ROLE_NONE))
		continue;
	    h[1] = pick_race(h[0], ROLE_NONE, ROLE_NONE, PICK_RANDOM);
	    if (h[1] < 0)
		continue;
	    h[2] = pick_gend(h[0], h[1], ROLE_NONE, PICK_RANDOM);
	    if (h[2] < 0)
		continue;
	    h[3] = pick_align(h[0], h[1], h[2], PICK_RANDOM);
	    if (h[3] < 0)
		continue;
	    return TRUE;
	}
	return FALSE;
}

/* let the player pick a role, race, gender and alignment as usual */
STATIC_OVL boolean
choose_own()
{
	flags.initrole = opt_role;
	flags.initrace = opt_race;
	flags.initgend = opt_gend;
	flags.initalign = opt_align;
	/* an option naming a role that isn't open yet is ignored */
	if (flags.initrole >= 0
		&& !ok_role(flags.initrole, ROLE_NONE, ROLE_NONE, ROLE_NONE))
	    flags.initrole = flags.initrace = ROLE_NONE;
	if (flags.initrace >= 0
		&& !ok_race(flags.initrole, flags.initrace, ROLE_NONE, ROLE_NONE))
	    flags.initrace = ROLE_NONE;
	player_selection();
	/* on Android, backing out of the role menu returns here */
	return (boolean) (flags.initrole >= 0 && flags.initrace >= 0
			  && flags.initgend >= 0 && flags.initalign >= 0);
}

/* switch Lineage tracks on and off for the next game */
STATIC_OVL void
lineage_toggles()
{
	static const char *const roman[] = { "", "I", "II", "III" };
	winid win;
	anything any;
	menu_item *pick = 0;
	char buf[BUFSZ];
	int t, n, i;

	win = create_nhwindow(NHW_MENU);
	start_menu(win);
	add_text(win, "Tick the upgrades to use in the next game.");
	add_text(win, "Untick them all for an Unaided game: +25% Renown.");
	add_text(win, "");
	for (t = 0; t < NUM_LINEAGE; t++) {
	    if (!LIN_OPEN(t) || !lprof.lineage[t])
		continue;
	    Sprintf(buf, "%s %s: %s", legacy_lineage_name(t),
		    roman[lprof.lineage[t]],
		    legacy_lineage_desc(t, lprof.lineage[t]));
	    any.a_void = 0;
	    any.a_int = t + 1;
	    add_menu(win, NO_GLYPH, &any, 0, 0, ATR_NONE, buf,
		     lprof.lin_off[t] ? MENU_UNSELECTED : MENU_SELECTED);
	}
	end_menu(win, "Lineage for the next game");
	n = select_menu(win, PICK_ANY, &pick);
	destroy_nhwindow(win);
	if (n < 0)
	    return;
	for (t = 0; t < NUM_LINEAGE; t++)
	    if (LIN_OPEN(t) && lprof.lineage[t])
		lprof.lin_off[t] = TRUE;
	for (i = 0; i < n; i++)
	    lprof.lin_off[pick[i].item.a_int - 1] = FALSE;
	if (pick)
	    free((genericptr_t) pick);
	lprof.dirty = TRUE;
	legacy_write(FALSE);
}

/*
 * The Prepare screen for a Legacy game.  Returns TRUE once a character
 * is chosen (flags.initrole etc. are set), FALSE to go back to the Hall.
 */
STATIC_OVL boolean
prepare_legacy(fresh_heirs)
boolean fresh_heirs;
{
	static int heirs[NUM_HEIRS][4];
	static int nheirs = 0;
	winid win;
	menu_item *pick = 0;
	char buf[BUFSZ], help[BUFSZ];
	int i, n, choice;

	lrun.mode = LMODE_LEGACY;
	legacy_selecting = TRUE;
	if (fresh_heirs || !nheirs)
	    for (nheirs = 0; nheirs < NUM_HEIRS; nheirs++)
		if (!random_hero(heirs[nheirs]))
		    break;

	for (;;) {
	    win = create_nhwindow(NHW_MENU);
	    start_menu(win);
	    Sprintf(buf, "Heirs of %s's line, ready to go:", plname);
	    add_text(win, buf);
	    for (i = 0; i < nheirs; i++) {
		legacy_hero_desc(help, heirs[i][0], heirs[i][1], heirs[i][2],
				 heirs[i][3]);
		Sprintf(buf, "Heir: %s", help);
		add_choice(win, 1 + i, 'a' + i, buf);
	    }
	    add_text(win, "");
	    add_choice(win, 10, 'd', "Choose your own character");
	    add_choice(win, 11, 'e', "New heirs");
	    add_text(win, "");
	    if (legacy_unaided_possible()) {
		lrun.mode = LMODE_LEGACY;
		legacy_setup_run();
		legacy_help_text(help, FALSE);
		Sprintf(buf, "Help for this game: %s", help);
		add_choice(win, 12, 'f', buf);
	    } else
		add_text(win,
		     "Help for this game: none yet (buy Lineage in the Hall)");
	    fate_vow_text(help, TRUE);
	    Sprintf(buf, "Vows: %s", help);
	    add_choice(win, 14, 'g', buf);
	    if (lprof.nvault) {
		fate_heir_text(help);
		Sprintf(buf, "Heirloom: %s", help);
		add_choice(win, 15, 'h', buf);
	    }
	    add_choice(win, 13, '<', "Back to the Hall");
	    end_menu(win, "Prepare a Legacy game");
	    n = select_menu(win, PICK_ONE, &pick);
	    destroy_nhwindow(win);
	    choice = (n > 0) ? pick[0].item.a_int : 13;
	    if (pick)
		free((genericptr_t) pick), pick = 0;

	    if (choice >= 1 && choice <= NUM_HEIRS) {
		set_hero(heirs[choice - 1]);
		break;
	    } else if (choice == 10) {
		if (choose_own())
		    break;
	    } else if (choice == 11) {
		for (nheirs = 0; nheirs < NUM_HEIRS; nheirs++)
		    if (!random_hero(heirs[nheirs]))
			break;
	    } else if (choice == 12) {
		lineage_toggles();
	    } else if (choice == 14) {
		fate_vow_menu();
	    } else if (choice == 15) {
		fate_heir_menu();
	    } else {
		legacy_selecting = FALSE;
		return FALSE;
	    }
	}
	nheirs = 0;	/* the next Prepare screen gets new heirs */
	fate_kit_menu();	/* kit B or C, once mastery opens them */
	lrun.mode = LMODE_LEGACY;
	legacy_setup_run();
	return TRUE;
}

/* buy Lineage upgrades */
STATIC_OVL void
lineage_shop()
{
	static const char *const roman[] = { "", "I", "II", "III" };
	winid win;
	menu_item *pick = 0;
	char buf[BUFSZ], qbuf[QBUFSZ];
	const char *why;
	int t, n, r, cost;

	for (;;) {
	    win = create_nhwindow(NHW_MENU);
	    start_menu(win);
	    Sprintf(buf, "You have %ld Renown to spend (%ld earned in all).",
		    legacy_renown(), lprof.renown_earned);
	    add_text(win, buf);
	    add_text(win, "Upgrades help Legacy games only, and each can be");
	    add_text(win, "switched off on the Prepare screen.");
	    add_text(win, "");
	    for (t = 0; t < NUM_LINEAGE; t++) {
		if (!LIN_OPEN(t))
		    continue;
		r = lprof.lineage[t];
		if (r >= LIN_MAXRANK) {
		    Sprintf(buf, "%s III (complete): %s",
			    legacy_lineage_name(t), legacy_lineage_desc(t, r));
		    add_text(win, buf);
		    continue;
		}
		cost = legacy_lineage_cost(t, r + 1);
		why = legacy_lineage_blocked(t);
		Sprintf(buf, "%s %s for %d: %s", legacy_lineage_name(t),
			roman[r + 1], cost, legacy_lineage_desc(t, r + 1));
		if (why) {
		    Sprintf(eos(buf), " [%s]", why);
		    add_text(win, buf);
		} else if (legacy_renown() < cost) {
		    Sprintf(eos(buf), " [need %ld more]",
			    cost - legacy_renown());
		    add_text(win, buf);
		} else
		    add_choice(win, t + 1, 'a' + t, buf);
	    }
	    end_menu(win, "Lineage");
	    n = select_menu(win, PICK_ONE, &pick);
	    destroy_nhwindow(win);
	    if (n <= 0)
		return;
	    t = pick[0].item.a_int - 1;
	    free((genericptr_t) pick), pick = 0;
	    r = lprof.lineage[t] + 1;
	    Sprintf(qbuf, "Spend %d Renown on %s %s?",
		    legacy_lineage_cost(t, r), legacy_lineage_name(t), roman[r]);
	    if (yn(qbuf) == 'y' && legacy_buy_lineage(t)) {
		lprof.lin_off[t] = FALSE;
		Sprintf(buf, "%s %s is yours.", legacy_lineage_name(t),
			roman[r]);
		pline("%s", buf);
	    }
	}
}

/* open roles and races with Renown */
STATIC_OVL void
unlock_shop()
{
	winid win;
	menu_item *pick = 0;
	char buf[BUFSZ], qbuf[QBUFSZ];
	const char *why;
	int i, n, cost, val, isrole, idx;
	char accel = 'a';

	for (;;) {
	    win = create_nhwindow(NHW_MENU);
	    start_menu(win);
	    Sprintf(buf, "You have %ld Renown to spend.", legacy_renown());
	    add_text(win, buf);
	    if (lprof.unlock_all)
		add_text(win,
		     "Every role and race is open (see Settings to change it).");
	    add_text(win, "Locked ones open through a feat, or for Renown.");
	    add_text(win, "");
	    accel = 'a';
	    for (isrole = 1; isrole >= 0; isrole--) {
		add_text(win, isrole ? "Roles:" : "Races:");
		for (i = 0; isrole ? roles[i].name.m != 0
				   : races[i].noun != 0; i++) {
		    const char *name = isrole ? roles[i].name.m
					      : races[i].noun;

		    why = legacy_unlock_text((boolean) isrole, i);
		    if (!why) {
			Sprintf(buf, "  %s: open", name);
			add_text(win, buf);
			continue;
		    }
		    cost = legacy_unlock_cost((boolean) isrole, i);
		    Sprintf(buf, "%s: %s, or %d Renown", name, why, cost);
		    val = (isrole ? 1 : 100) + i;
		    if (legacy_renown() >= cost)
			add_choice(win, val, accel++, buf);
		    else {
			Sprintf(qbuf, "  %s", buf);
			add_text(win, qbuf);
		    }
		}
	    }
	    end_menu(win, "Roles and races");
	    n = select_menu(win, PICK_ONE, &pick);
	    destroy_nhwindow(win);
	    if (n <= 0)
		return;
	    val = pick[0].item.a_int;
	    free((genericptr_t) pick), pick = 0;
	    isrole = (val < 100);
	    idx = isrole ? val - 1 : val - 100;
	    Sprintf(qbuf, "Spend %d Renown to open %s?",
		    legacy_unlock_cost((boolean) isrole, idx),
		    isrole ? roles[idx].name.m : races[idx].noun);
	    if (yn(qbuf) == 'y' && legacy_buy_unlock((boolean) isrole, idx))
		pline("%s is open.", isrole ? roles[idx].name.m
					    : upstart(strcpy(buf,
							 races[idx].noun)));
	}
}

STATIC_OVL void
show_feats()
{
	static const char *const groups[] = {
	    "Descent", "Branches", "Skill", "Knowledge", "Ancestry"
	};
	winid win;
	char buf[BUFSZ];
	int g, f;

	win = create_nhwindow(NHW_TEXT);
	Sprintf(buf, "Feats: %d of %d done.  Each pays its Renown once.",
		legacy_feats_done(), NUM_FEATS);
	putstr(win, 0, buf);
	for (g = 0; g < SIZE(groups); g++) {
	    putstr(win, 0, "");
	    putstr(win, ATR_BOLD, groups[g]);
	    for (f = 0; f < NUM_FEATS; f++) {
		if (legacy_feat_group(f) != g)
		    continue;
		Sprintf(buf, "[%c] %s (%d)", lprof.feat_done[f] ? 'x' : ' ',
			legacy_feat_name(f), legacy_feat_renown(f));
		if (legacy_feat_unlocks(f))
		    Sprintf(eos(buf), ": opens %s", legacy_feat_unlocks(f));
		putstr(win, 0, buf);
	    }
	}
	if (lprof.counter[LC_SPELLS] < 100 || lprof.counter[LC_UNDEAD] < 25) {
	    putstr(win, 0, "");
	    Sprintf(buf, "Spells cast: %ld.  Undead destroyed: %ld.",
		    lprof.counter[LC_SPELLS], lprof.counter[LC_UNDEAD]);
	    putstr(win, 0, buf);
	}
	display_nhwindow(win, TRUE);
	destroy_nhwindow(win);
}

STATIC_OVL void
show_ancestor(a)
struct legacy_ancestor *a;
{
	winid win;
	char buf[BUFSZ];

	win = create_nhwindow(NHW_TEXT);
	Sprintf(buf, "%s the %s, generation %d", a->name, a->role, a->gen);
	putstr(win, ATR_BOLD, buf);
	Sprintf(buf, "A %s %s %s %s, played in %s mode.", a->align, a->gender,
		a->race, a->role, a->mode == LMODE_LEGACY ? "Legacy"
							    : "Classic");
	putstr(win, 0, buf);
	Sprintf(buf, "Ended: %s.", a->death);
	putstr(win, 0, buf);
	Sprintf(buf, "Deepest level %d, experience level %d, %ld turns.",
		a->deepest, a->xl, a->turns);
	putstr(win, 0, buf);
	Sprintf(buf, "Help: %s.", a->help);
	putstr(win, 0, buf);
	Sprintf(buf, "Renown earned: %ld.", a->renown);
	putstr(win, 0, buf);
	if (a->when) {
	    Sprintf(buf, "Date: %04ld-%02ld-%02ld.", a->when / 10000,
		    (a->when / 100) % 100, a->when % 100);
	    putstr(win, 0, buf);
	}
	if (*a->epitaph) {
	    putstr(win, 0, "");
	    Sprintf(buf, "\"%s\"", a->epitaph);
	    putstr(win, 0, buf);
	}
	display_nhwindow(win, TRUE);
	destroy_nhwindow(win);
}

STATIC_OVL void
show_ancestors()
{
	struct legacy_ancestor *a;
	winid win;
	menu_item *pick = 0;
	char buf[BUFSZ];
	int i, n;

	if (!lprof.ancestors) {
	    pline("No ancestors yet.  Every game you finish adds one.");
	    return;
	}
	for (;;) {
	    win = create_nhwindow(NHW_MENU);
	    start_menu(win);
	    for (i = 0, a = lprof.ancestors; a && i < 100; a = a->next, i++) {
		Sprintf(buf, "%d. %s the %s, level %d: %s", a->gen, a->name,
			a->role, a->deepest, a->death);
		add_choice(win, i + 1, 0, buf);
	    }
	    end_menu(win, "Ancestors, newest first");
	    n = select_menu(win, PICK_ONE, &pick);
	    destroy_nhwindow(win);
	    if (n <= 0)
		return;
	    i = pick[0].item.a_int - 1;
	    free((genericptr_t) pick), pick = 0;
	    for (a = lprof.ancestors; a && i > 0; a = a->next)
		i--;
	    if (a)
		show_ancestor(a);
	}
}

STATIC_OVL void
hall_settings()
{
	winid win;
	menu_item *pick = 0;
	char buf[BUFSZ];
	int n, i;
	boolean any;

	for (;;) {
	    win = create_nhwindow(NHW_MENU);
	    start_menu(win);
	    Sprintf(buf, "[%c] Open every role and race in Legacy",
		    lprof.unlock_all ? 'x' : ' ');
	    add_choice(win, 1, 'a', buf);
	    Sprintf(buf, "[%c] Skip the Hall: start at character selection",
		    lprof.skip_hall ? 'x' : ' ');
	    add_choice(win, 2, 'b', buf);
	    if (lprof.skip_hall)
		add_text(win,
		   "    (the end-of-game menu still leads back to the Hall)");
	    add_text(win, "");
	    add_text(win, "Insight you have earned:");
	    for (i = 0, any = FALSE; i < NUM_INSIGHT; i++)
		if (insight_earned(i)) {
		    Sprintf(buf, "[%c] %s", lprof.ins_off[i] ? ' ' : 'x',
			    insight_name(i));
		    add_choice(win, 10 + i, (char) ('c' + i), buf);
		    any = TRUE;
		}
	    if (!any)
		add_text(win, "    (none yet; see the Codex)");
	    end_menu(win, "Settings");
	    n = select_menu(win, PICK_ONE, &pick);
	    destroy_nhwindow(win);
	    if (n <= 0)
		return;
	    if (pick[0].item.a_int == 1)
		lprof.unlock_all = !lprof.unlock_all;
	    else if (pick[0].item.a_int == 2)
		lprof.skip_hall = !lprof.skip_hall;
	    else if (pick[0].item.a_int >= 10)
		lprof.ins_off[pick[0].item.a_int - 10] =
		    !lprof.ins_off[pick[0].item.a_int - 10];
	    free((genericptr_t) pick), pick = 0;
	    lprof.dirty = TRUE;
	    legacy_write(FALSE);
	}
}

/* ------------------------------------------------------------------ */
/* restarting after a game */

#define RS_NONE		0	/* no marker: show the Hall */
#define RS_AGAIN	1	/* play again; flags.initrole etc. are set */
#define RS_PREPARE	2	/* a new character in the same mode */
#define RS_HALL		3

STATIC_OVL void
write_restart(what)
const char *what;
{
	FILE *fp = fopen(fqname(LEGACY_RESTART, SCOREPREFIX, 0), "w");

	if (fp) {
	    fprintf(fp, "%s\n", what);
	    (void) fclose(fp);
	}
}

/* read and remove the restart marker; 'mode' gets the game mode */
STATIC_OVL int
read_restart(mode)
int *mode;
{
	FILE *fp;
	char line[BUFSZ], what[BUFSZ];
	int m = LMODE_LEGACY, h[4], n, result = RS_NONE;
	const char *fq = fqname(LEGACY_RESTART, SCOREPREFIX, 0);

	if (!(fp = fopen(fq, "r")))
	    return RS_NONE;
	if (fgets(line, sizeof line, fp)) {
	    n = sscanf(line, "%s %d %d %d %d %d", what, &m, &h[0], &h[1],
		       &h[2], &h[3]);
	    if (!strcmp(what, "again") && n == 6) {
		lrun.mode = m;
		legacy_selecting = TRUE;
		if (ok_role(h[0], h[1], h[2], h[3])
			&& ok_race(h[0], h[1], h[2], h[3])
			&& ok_gend(h[0], h[1], h[2], h[3])
			&& ok_align(h[0], h[1], h[2], h[3])) {
		    set_hero(h);
		    result = RS_AGAIN;
		} else
		    result = RS_PREPARE;
	    } else if (!strcmp(what, "prepare") && n >= 2)
		result = RS_PREPARE;
	    else if (!strcmp(what, "hall"))
		result = RS_HALL;
	}
	(void) fclose(fp);
	(void) unlink(fq);
	*mode = m ? LMODE_LEGACY : LMODE_CLASSIC;
	return result;
}

/* ------------------------------------------------------------------ */
/* the Hall */

STATIC_OVL void
money_line(buf, label, val)
char *buf;
const char *label;
long val;
{
	Sprintf(buf, "  %-46s %5ld", label, val);
}

/*
 * Called at startup when there is no saved game.  Shows the Hall (or
 * follows the restart marker) and returns with the next game's mode
 * and character chosen.
 */
void
legacy_startup()
{
	winid win;
	menu_item *pick = 0;
	char buf[BUFSZ], goals[3][BUFSZ];
	int n, i, choice, mode = LMODE_LEGACY, restart;

	legacy_init();
	opt_role = flags.initrole;
	opt_race = flags.initrace;
	opt_gend = flags.initgend;
	opt_align = flags.initalign;
	hall_active = TRUE;

	restart = read_restart(&mode);
	if (restart == RS_AGAIN) {
	    legacy_setup_run();	/* keeps lrun.mode */
	    hall_active = FALSE;
	    return;
	}
	if (restart == RS_PREPARE || (restart == RS_NONE && lprof.skip_hall)) {
	    if (restart == RS_NONE)
		mode = lprof.last_mode;
	    if (mode == LMODE_LEGACY) {
		if (prepare_legacy(TRUE)) {
		    hall_active = FALSE;
		    return;
		}
	    } else {
		lrun.mode = LMODE_CLASSIC;
		legacy_selecting = FALSE;
		if (choose_own()) {
		    legacy_setup_run();
		    hall_active = FALSE;
		    return;
		}
	    }
	}

	for (;;) {
	    win = create_nhwindow(NHW_MENU);
	    start_menu(win);
	    Sprintf(buf, "Renown: %ld to spend (%ld earned in all)",
		    legacy_renown(), lprof.renown_earned);
	    add_text(win, buf);
	    Sprintf(buf, "Ancestors: %d.  Feats: %d of %d.",
		    legacy_ancestor_count(), legacy_feats_done(), NUM_FEATS);
	    add_text(win, buf);
	    n = legacy_next_goals(goals, 3);
	    if (n) {
		add_text(win, "Next goals:");
		for (i = 0; i < n; i++) {
		    Sprintf(buf, "  %s", goals[i]);
		    add_text(win, buf);
		}
	    }
	    add_text(win, "");
	    add_choice(win, 1, 'a',
		       "Play Legacy: a hard start that eases as your line grows");
	    add_choice(win, 2, 'b', "Play Classic: the original game, no help");
	    add_choice(win, 3, 'c', "Lineage: spend Renown on upgrades");
	    add_choice(win, 10, 'd', "Blessings: grow the pool Favor draws from");
	    add_choice(win, 4, 'e', "Roles and races");
	    add_choice(win, 11, 'f', "Role mastery and kits");
	    add_choice(win, 5, 'g', "Feats");
	    add_choice(win, 6, 'h', "Ancestors");
	    add_choice(win, 12, 'i', "The heirloom vault");
	    add_choice(win, 9, 'j', "The Codex and Insight");
	    add_choice(win, 7, 'k', "Settings");
	    add_choice(win, 8, 'q', "Quit");
	    Sprintf(buf, "The Hall of %s's line", plname);
	    end_menu(win, buf);
	    n = select_menu(win, PICK_ONE, &pick);
	    destroy_nhwindow(win);
	    choice = (n > 0) ? pick[0].item.a_int : 8;
	    if (pick)
		free((genericptr_t) pick), pick = 0;

	    switch (choice) {
	    case 1:
		if (prepare_legacy(FALSE)) {
		    hall_active = FALSE;
		    return;
		}
		break;
	    case 2:
		lrun.mode = LMODE_CLASSIC;
		legacy_selecting = FALSE;
		if (choose_own()) {
		    legacy_setup_run();
		    hall_active = FALSE;
		    return;
		}
		break;
	    case 3:
		lineage_shop();
		break;
	    case 4:
		unlock_shop();
		break;
	    case 5:
		show_feats();
		break;
	    case 6:
		show_ancestors();
		break;
	    case 7:
		hall_settings();
		break;
	    case 9:
		codex_menu(TRUE);
		break;
	    case 10:
		fate_bless_shop();
		break;
	    case 11:
		fate_mastery_menu();
		break;
	    case 12:
		fate_vault_menu();
		break;
	    default:
		hall_active = FALSE;
		legacy_write(FALSE);
		clearlocks();
		exit_nhwindows((char *) 0);
		terminate(EXIT_SUCCESS);
	    }
	}
}

/* ------------------------------------------------------------------ */
/* the recap at the end of a game */

/* what this game added to the Codex and Insight */
STATIC_OVL void
recap_knowledge(win)
winid win;
{
	static const char *const vol[] = { "monsters", "object types",
					   "places" };
	char buf[BUFSZ], prog[BUFSZ];
	long now = insight_mask();
	int i;
	boolean first = TRUE, moved;

	Strcpy(buf, "Codex: ");
	for (i = 0; i < 3; i++)
	    if (lrun.cx_new[i]) {
		Sprintf(eos(buf), "%s%d new %s", first ? "" : ", ",
			lrun.cx_new[i], vol[i]);
		first = FALSE;
	    }
	if (lres.lesson == 1) {
	    Sprintf(eos(buf), "%sa new Lesson", first ? "" : ", ");
	    first = FALSE;
	}
	if (first && !now)
	    return;
	putstr(win, 0, "");
	putstr(win, ATR_BOLD, "Knowledge");
	if (!first)
	    putstr(win, 0, buf);
	for (i = 0; i < NUM_INSIGHT; i++) {
	    if ((now & (1L << i)) && !(lrun.ins_had & (1L << i))) {
		Sprintf(buf, "Insight earned: %s", insight_name(i));
		putstr(win, 0, buf);
		continue;
	    }
	    if (now & (1L << i))
		continue;
	    /* not earned yet: did this game move it closer? */
	    switch (i) {
	    case INS_THREAT:
		moved = codex_total_kills() > lrun.kills_start;
		break;
	    case INS_PRICE:
		moved = codex_armory_count() > lrun.ids_start;
		break;
	    case INS_CORPSE:
		moved = lprof.icount[IC_CORPSES] > lrun.ic_start[IC_CORPSES];
		break;
	    case INS_PRAYER:
		moved = lprof.icount[IC_PRAYERS] > lrun.ic_start[IC_PRAYERS];
		break;
	    case INS_WAND:
		moved = lprof.icount[IC_WANDS] > lrun.ic_start[IC_WANDS];
		break;
	    case INS_ELBERETH:
		moved = lprof.icount[IC_ELBERETH] > lrun.ic_start[IC_ELBERETH];
		break;
	    default:
		moved = FALSE;
	    }
	    if (moved) {
		insight_progress(i, prog);
		Sprintf(buf, "Toward %s: %s", insight_name(i), prog);
		putstr(win, 0, buf);
	    }
	}
}

void
legacy_recap(how)
int how;
{
	winid win;
	menu_item *pick = 0;
	char buf[BUFSZ], line[BUFSZ], goals[3][BUFSZ];
	int n, i, f, choice;
	boolean anyfeat = FALSE;

	if (!lres.valid || how == PANICKED)
	    return;

	win = create_nhwindow(NHW_TEXT);
	Sprintf(buf, "%s the %s, generation %d (%s)", plname,
		(flags.female && urole.name.f) ? urole.name.f : urole.name.m,
		lrun.gen, lrun.mode == LMODE_LEGACY ? "Legacy" : "Classic");
	putstr(win, ATR_BOLD, buf);
	Sprintf(buf, "%s on turn %ld.", upstart(strcpy(line, lres.death)),
		moves);
	putstr(win, 0, buf);
	if (lrun.ward_used) {
	    Sprintf(buf, "The Ward saved you on turn %ld.", lrun.ward_turn);
	    putstr(win, 0, buf);
	}
	if (lres.lesson) {
	    char key[BUFSZ];

	    putstr(win, 0, lres.lesson == 1
			   ? "A new Lesson: this hasn't killed you before."
			   : "You have died this way before. The Lesson:");
	    legacy_lesson_key(key, lres.death);
	    legacy_put_wrapped(win, codex_lesson_tip(key));
	}

	putstr(win, 0, "");
	putstr(win, ATR_BOLD, "Renown from this game");
	if (lres.credited < lres.deepest)
	    Sprintf(line, "Depth: level %d (deepest %d, capped by XL)",
		    lres.credited, lres.deepest);
	else
	    Sprintf(line, "Depth: level %d", lres.credited);
	money_line(buf, line, lres.depth_pts);
	putstr(win, 0, buf);
	if (lres.landmark_pts) {
	    money_line(buf, "Landmarks", lres.landmark_pts);
	    putstr(win, 0, buf);
	    Sprintf(buf, "    (%s)", lres.landmarks);
	    putstr(win, 0, buf);
	}
	if (lres.soko_pts) {
	    Sprintf(line, "Sokoban levels finished: %d", lrun.soko_solved);
	    money_line(buf, line, lres.soko_pts);
	    putstr(win, 0, buf);
	}
	if (lres.lesson_pts) {
	    money_line(buf, "New Lesson", lres.lesson_pts);
	    putstr(win, 0, buf);
	}
	if (lres.mult != 100) {
	    *line = '\0';
	    if (lrun.unaided)
		Strcpy(line, "Unaided +25%");
	    if (lres.vow_pct)
		Sprintf(eos(line), "%svows kept +%d%%", *line ? ", " : "",
			lres.vow_pct);
	    Sprintf(buf, "  %s: everything above x%d.%02d", line,
		    lres.mult / 100, lres.mult % 100);
	    putstr(win, 0, buf);
	}
	if (discover)
	    putstr(win, 0, "  Explore mode earns no Renown.");
	else if (lres.floored)
	    putstr(win, 0, "  Every finished game pays at least 10.");
	money_line(buf, "This game", lres.total);
	putstr(win, 0, buf);
	for (f = 0; f < NUM_FEATS; f++)
	    if (lrun.feats[f / 32] & (1L << (f % 32))) {
		if (!anyfeat)
		    putstr(win, 0, "  Feats:");
		anyfeat = TRUE;
		Sprintf(line, "  %s", legacy_feat_name(f));
		money_line(buf, line, (long) legacy_feat_renown(f));
		putstr(win, 0, buf);
		if (legacy_feat_unlocks(f)) {
		    Sprintf(buf, "      opens %s", legacy_feat_unlocks(f));
		    putstr(win, 0, buf);
		}
	    }
	money_line(buf, "Total", lres.total + lrun.feat_renown);
	putstr(win, ATR_BOLD, buf);
	recap_knowledge(win);
	putstr(win, 0, "");
	Sprintf(buf, "You have %ld Renown to spend (%ld earned in all).",
		legacy_renown(), lprof.renown_earned);
	putstr(win, 0, buf);
	n = legacy_next_goals(goals, 3);
	if (n) {
	    putstr(win, 0, "");
	    putstr(win, ATR_BOLD, "Next goals");
	    for (i = 0; i < n; i++) {
		Sprintf(buf, "  %s", goals[i]);
		putstr(win, 0, buf);
	    }
	}
	display_nhwindow(win, TRUE);
	destroy_nhwindow(win);

	for (;;) {
	    win = create_nhwindow(NHW_MENU);
	    start_menu(win);
	    add_choice(win, 1, 'a', "Play again with the same kind of character");
	    add_choice(win, 2, 'b', "New character");
	    add_choice(win, 3, 'c', "Go to the Hall");
	    add_choice(win, 4, 'd', "Write an epitaph");
	    add_choice(win, 5, 'q', "Quit");
	    end_menu(win, "What next?");
	    n = select_menu(win, PICK_ONE, &pick);
	    destroy_nhwindow(win);
	    choice = (n > 0) ? pick[0].item.a_int : 5;
	    if (pick)
		free((genericptr_t) pick), pick = 0;
	    if (choice == 4) {
		getlin("Epitaph for this ancestor:", buf);
		if (*buf && *buf != '\033')
		    legacy_set_epitaph(buf);
		continue;
	    }
	    break;
	}
	switch (choice) {
	case 1:
	    Sprintf(buf, "again %d %d %d %d %d", lrun.mode, flags.initrole,
		    flags.initrace, flags.female ? 1 : 0, flags.initalign);
	    write_restart(buf);
	    break;
	case 2:
	    Sprintf(buf, "prepare %d", lrun.mode);
	    write_restart(buf);
	    break;
	case 3:
	    write_restart("hall");
	    break;
	default:
	    return;
	}
	/* going straight on: skip the high score list */
	program_state.stopprint++;
}

/*hall.c*/
