/*	SCCS Id: @(#)legacy.h	0.0.7E7F3	2026/10/13	*/
/* Slash'EM Legacy: the profile that carries over from game to game. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef LEGACY_H
#define LEGACY_H

/* files in the game's directory */
#define LEGACY_PROFILE	"legacy.dat"	/* the profile */
#define LEGACY_BACKUP	"legacy.bak"	/* the profile before its last write */
#define LEGACY_TMP	"legacy.tmp"	/* a profile being written */
#define LEGACY_RESTART	"legacy.restart" /* what to do when the app restarts */
#define LEGACY_VERSION	1

/* how a game is played */
#define LMODE_CLASSIC	0	/* the original game; nothing carries in */
#define LMODE_LEGACY	1	/* Lineage, locked roles and the Ward */

/* Lineage tracks, bought with Renown.  Only the first LIN_AVAILABLE are
   in the game so far; the others arrive with later parts of Legacy. */
#define LIN_VIGOR	0
#define LIN_PROVISIONS	1
#define LIN_REMEDIES	2
#define LIN_ESCAPE	3
#define LIN_PATRON	4
#define LIN_LORE	5
#define LIN_COMPANION	6
#define LIN_ARMS	7
#define LIN_WARD	8
#define LIN_FAVOR	9
#define LIN_FORTUNE	10
#define LIN_INHERITANCE	11
#define NUM_LINEAGE	12
#define LIN_AVAILABLE	9
#define LIN_MAXRANK	3

/* landmarks: each pays Renown once per game */
#define LM_MINES	0
#define LM_ORACLE	1
#define LM_MINETOWN	2
#define LM_SOKOBAN	3	/* finishing Sokoban */
#define LM_MINESEND	4
#define LM_BIGROOM	5
#define LM_QPORTAL	6
#define LM_MEDUSA	7	/* killing Medusa */
#define LM_CASTLE	8
#define LM_VALLEY	9
#define LM_ASCENDED	10
#define NUM_LANDMARKS	11

/* feats: one-time achievements, counted across all games */
#define F_DL3		0
#define F_MINES		1
#define F_MINETOWN	2
#define F_ORACLE	3
#define F_SOKO1		4
#define F_SOKOBAN	5
#define F_LUCKSTONE	6
#define F_DL10		7
#define F_BIGROOM	8
#define F_QPORTAL	9
#define F_MEDUSA	10
#define F_CASTLE	11
#define F_WISHWAND	12
#define F_GEHENNOM	13
#define F_VLAD		14
#define F_QUEST		15
#define F_AMULET	16
#define F_ASCEND	17
#define F_GRUND		18
#define F_WYRM		19
#define F_SAM		20
#define F_TOMB		21
#define F_SPIDERS	22
#define F_SUNLESS	23
#define F_MOLOCH	24
#define F_LUDIOS	25
#define F_FRANKEN	26
#define F_DIG		27
#define F_THROWN20	28
#define F_XL8		29
#define F_PETDL8	30
#define F_WIZXL10	31
#define F_SPELLS100	32
#define F_UNDEAD25	33
#define F_ELFDL10	34
#define F_POLY5		35
#define F_LYCAN		36
#define F_VAMPIRE	37
#define F_PROTECTION	38
#define F_GIFT		39
#define F_CROWNED	40
#define F_LESSONS10	41
#define F_GHOST		42
#define F_ANCESTORS25	43
#define NUM_FEATS	44

/* counters kept across games */
#define LC_SPELLS	0	/* spells cast */
#define LC_UNDEAD	1	/* undead destroyed */
#define NUM_LCOUNTERS	2

/* one finished game, as the Ancestors list shows it */
struct legacy_ancestor {
	struct legacy_ancestor *next;	/* the next older one */
	int gen;		/* generation: the profile's game number */
	int mode;		/* LMODE_xxx */
	int deepest;		/* deepest level reached */
	int xl;			/* experience level at the end */
	long turns;
	long renown;		/* Renown the game paid, feats included */
	boolean ward_fired;
	int dnum, dlevel;	/* where it ended, to recognise its bones */
	long when;		/* yyyymmdd */
	char name[PL_NSIZ];
	char role[20], race[20], gender[10], align[10];
	char death[BUFSZ];	/* "killed by a soldier ant" */
	char help[BUFSZ];	/* the help it had */
	char epitaph[BUFSZ];
};

/* the profile, kept in legacy.dat */
struct legacy_profile {
	long renown_earned;	/* all Renown ever earned */
	long renown_spent;
	xchar lineage[NUM_LINEAGE];	/* ranks owned */
	boolean lin_off[NUM_LINEAGE];	/* switched off for the next game */
	long feat_done[NUM_FEATS];	/* when earned (yyyymmdd), or 0 */
	long counter[NUM_LCOUNTERS];
	unsigned long roles_bought, races_bought;	/* by index, with Renown */
	boolean unlock_all;	/* every role and race open in Legacy */
	boolean skip_hall;	/* start at character selection */
	int last_mode;		/* LMODE_xxx of the last game */
	int generation;		/* games started */
	char **lessons;		/* causes of death met so far */
	int nlessons, maxlessons;
	struct legacy_ancestor *ancestors;	/* newest first */
	boolean dirty;		/* changed since it was written */
};

/* the current game's Legacy state; saved with the game */
struct legacy_run {
	int mode;		/* LMODE_xxx */
	int gen;		/* generation of this character */
	xchar lineage[NUM_LINEAGE];	/* ranks in use this game */
	boolean unaided;	/* owned Lineage all switched off: +25% */
	boolean ward_used;
	long ward_turn;
	long landmarks;		/* LM_xxx bits reached */
	int soko_solved;	/* Sokoban levels finished */
	int thrown_kills;
	int polys;		/* times polymorphed */
	boolean lycan_caught;
	unsigned first_pet;	/* m_id of the starting pet */
	long feats[2];		/* feats earned in this game (bits) */
	long feat_renown;	/* Renown those feats paid */
	int ghost_ledger;	/* level where an ancestor's ghost waits, or 0 */
	char ghost_name[PL_NSIZ];
};

/* what the last game paid, for the recap */
struct legacy_result {
	boolean valid;
	int deepest;		/* deepest level reached */
	int credited;		/* the part of it that pays */
	int cap;		/* 3 x experience level + 3 */
	long depth_pts, landmark_pts, soko_pts, lesson_pts;
	long subtotal;		/* before the multiplier */
	int mult;		/* multiplier, in percent */
	long total;		/* what the game paid, feats aside */
	boolean floored;	/* raised to the minimum */
	int lesson;		/* 0: none, 1: new, 2: met before */
	char landmarks[BUFSZ];
	char death[BUFSZ];
};

extern struct legacy_profile lprof;
extern struct legacy_run lrun;
extern struct legacy_result lres;
extern boolean legacy_selecting;	/* role selection honours locks */
extern boolean legacy_throwing;		/* a thrown or fired missile is hitting */

#define E extern

/* legacy.c */
E void NDECL(legacy_init);
E void FDECL(legacy_write, (BOOLEAN_P));
E boolean NDECL(legacy_mode);
E boolean FDECL(legacy_role_locked, (int));
E boolean FDECL(legacy_race_locked, (int));
E const char *FDECL(legacy_unlock_text, (BOOLEAN_P, int));
E int FDECL(legacy_unlock_cost, (BOOLEAN_P, int));
E long NDECL(legacy_renown);
E int FDECL(legacy_lineage_cost, (int, int));
E const char *FDECL(legacy_lineage_name, (int));
E const char *FDECL(legacy_lineage_desc, (int, int));
E const char *FDECL(legacy_lineage_blocked, (int));
E boolean NDECL(legacy_unaided_possible);
E void NDECL(legacy_setup_run);
E void FDECL(legacy_newgame, (struct monst *));
E void NDECL(legacy_welcome);
E void FDECL(legacy_help_text, (char *, BOOLEAN_P));
E const char *FDECL(legacy_feat_name, (int));
E int FDECL(legacy_feat_renown, (int));
E const char *FDECL(legacy_feat_unlocks, (int));
E int FDECL(legacy_feat_group, (int));
E void FDECL(legacy_feat, (int));
E void FDECL(legacy_count, (int));
E void FDECL(legacy_arrived, (BOOLEAN_P));
E void FDECL(legacy_mon_died, (struct monst *, BOOLEAN_P));
E void FDECL(legacy_soko_solved, (int));
E void FDECL(legacy_got_object, (struct obj *));
E void NDECL(legacy_polymorphed);
E void FDECL(legacy_xlevel, (int));
E void FDECL(legacy_bones_loaded, (const char *));
E void NDECL(legacy_turn);
E boolean FDECL(legacy_ward, (int));
E void FDECL(legacy_ward_move, (int));
E void FDECL(legacy_game_over, (int, const char *));
E void FDECL(legacy_hero_desc, (char *, int, int, int, int));
E int NDECL(legacy_feats_done);
E int NDECL(legacy_ancestor_count);
E int FDECL(legacy_next_goals, (char [][BUFSZ], int));
E boolean FDECL(legacy_buy_lineage, (int));
E boolean FDECL(legacy_buy_unlock, (BOOLEAN_P, int));
E void FDECL(legacy_set_epitaph, (const char *));
E void FDECL(legacy_save_run_fd, (int));
E void FDECL(legacy_restore_run_fd, (int));

/* u_init.c */
E void NDECL(lineage_inv);

/* hall.c */
E void NDECL(legacy_startup);
E boolean NDECL(legacy_hall_active);
E void FDECL(legacy_recap, (int));

#undef E

#endif /* LEGACY_H */
