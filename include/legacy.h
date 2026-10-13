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
#define LEGACY_VERSION	2

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
#define F_ID50		44
#define F_BESTIARY100	45
#define F_INSIGHT	46
#define NUM_FEATS	47

/* counters kept across games */
#define LC_SPELLS	0	/* spells cast */
#define LC_UNDEAD	1	/* undead destroyed */
#define NUM_LCOUNTERS	2

/* Insight: help earned by doing things, counted across all games */
#define INS_FACTS	0	/* monster fact sheets */
#define INS_THREAT	1	/* a threat word when looking at a monster */
#define INS_WARN	2	/* learned warnings */
#define INS_CORPSE	3	/* corpse sense */
#define INS_PRAYER	4	/* prayer sense */
#define INS_PRICE	5	/* price sense */
#define INS_WAND	6	/* wand sense */
#define INS_ELBERETH	7	/* quick Elbereth */
#define INS_SOKOBAN	8	/* Sokoban memory */
#define NUM_INSIGHT	9

/* Insight counters kept in the profile */
#define IC_CORPSES	0	/* corpses eaten */
#define IC_PRAYERS	1	/* successful prayers */
#define IC_WANDS	2	/* wands engrave-tested */
#define IC_ELBERETH	3	/* Elbereths engraved */
#define NUM_ICOUNTERS	4

/* learned warnings: each switches on when its cause has killed you */
#define LW_FLOATEYE	0	/* hitting a floating eye that can see */
#define LW_TOUCH	1	/* hitting a cockatrice bare-handed */
#define LW_WATER	2	/* stepping into water */
#define LW_LAVA		3	/* stepping into lava */
#define LW_FOUNTAIN	4	/* fountains in front of the Minetown watch */
#define NUM_LWARN	5

/* Codex volumes */
#define CX_BESTIARY	0
#define CX_ARMORY	1
#define CX_ATLAS	2
#define CX_LESSONS	3
#define NUM_CODEX	4

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
	long icount[NUM_ICOUNTERS];	/* Insight counters */
	boolean ins_off[NUM_INSIGHT];	/* Insight switched off in Settings */
	boolean dirty;		/* changed since it was written */
};

/* Sokoban memory's recording of each level */
#define SOKO_LEVS	6	/* indexed by dunlev */
#define SOKO_ROWS	21
#define SOKO_COLS	32
#define SOKO_MAXB	26
#define SOKO_LOG	600
#define SOKO_UNSEEN	0
#define SOKO_RECORDING	1
#define SOKO_LOST	2	/* a boulder it didn't know about was pushed */
#define SOKO_DONE	3
/* how good a remembered Sokoban record is, best first */
#define SQ_SOLVED	0	/* every hole filled */
#define SQ_PARTIAL	1	/* got to the level above */
#define SQ_LOST		2	/* some pushes weren't recorded */

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
	/* the Codex and Insight (Phase 2) */
	int cx_new[NUM_CODEX];	/* Codex entries this game added */
	long ins_had;		/* Insight items earned before this game */
	long ins_told;		/* Insight items announced so far */
	long ic_start[NUM_ICOUNTERS];	/* counters when the game began */
	long kills_start, ids_start;	/* likewise */
	xchar soko_layout[6];	/* Sokoban layout used on each level */
	boolean pray_safe;	/* prayer sense showed "safe" last turn */
	/* Sokoban memory: each level as first seen, and the pushes made */
	char soko_map[SOKO_LEVS][SOKO_ROWS][SOKO_COLS];
	xchar soko_bx[SOKO_LEVS][SOKO_MAXB];	/* where each boulder is now, */
	xchar soko_by[SOKO_LEVS][SOKO_MAXB];	/* bx -1 once it fills a hole */
	uchar soko_nb[SOKO_LEVS];
	uchar soko_log[SOKO_LEVS][SOKO_LOG];	/* boulder<<3 | dir<<1 | filled */
	short soko_nlog[SOKO_LEVS];
	uchar soko_state[SOKO_LEVS];	/* SOKO_xxx */
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
extern const char *legacy_frozen_by;	/* why the hero is helpless, or 0 */

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
E void FDECL(legacy_clean_field, (char *, const char *, int));
E boolean FDECL(legacy_know_lesson_like, (const char *));
E void FDECL(legacy_lesson_key, (char *, const char *));
E void FDECL(legacy_soko_layout, (const char *));
E void NDECL(legacy_soko_arrived);
E void FDECL(legacy_soko_push, (int, int, int, int));

/* codex.c */
E void NDECL(codex_init);
E boolean FDECL(codex_read, (const char *, char *));
E void FDECL(codex_write, (FILE *));
E void FDECL(codex_saw, (struct permonst *));
E void FDECL(codex_killed, (struct permonst *));
E void FDECL(codex_identified, (int));
E void FDECL(codex_place, (int));
E void NDECL(codex_arrived);
E void NDECL(codex_entered_shop);
E void NDECL(codex_entered_temple);
E void FDECL(codex_soko_remember, (const char *, char **, int, char **, int, int));
E long NDECL(codex_total_kills);
E char *FDECL(legacy_type_name, (int));
E int NDECL(codex_armory_count);
E int FDECL(codex_count, (int));
E long FDECL(codex_kills, (struct permonst *));
E const char *FDECL(codex_lesson_tip, (const char *));
E void FDECL(codex_menu, (BOOLEAN_P));
E void FDECL(legacy_put_wrapped, (winid, const char *));
E void FDECL(codex_look, (struct permonst *, struct monst *));
E int NDECL(docodex);

/* pray.c */
E boolean NDECL(prayer_is_safe);

/* insight.c */
E boolean FDECL(insight_earned, (int));
E boolean FDECL(insight_on, (int));
E void FDECL(insight_progress, (int, char *));
E const char *FDECL(insight_name, (int));
E const char *FDECL(insight_desc, (int));
E void FDECL(insight_count, (int));
E void NDECL(insight_check);
E long NDECL(insight_mask);
E boolean FDECL(insight_warning, (int));
E boolean FDECL(insight_hit_check, (struct monst *, BOOLEAN_P));
E boolean NDECL(insight_fountain_check);
E boolean FDECL(insight_corpse_check, (struct obj *));
E void NDECL(insight_turn);
E boolean NDECL(insight_pray_marker);
E void FDECL(insight_wand_sense, (int, BOOLEAN_P));
E int NDECL(doelbereth);

/* u_init.c */
E void NDECL(lineage_inv);

/* hall.c */
E void NDECL(legacy_startup);
E boolean NDECL(legacy_hall_active);
E void FDECL(legacy_recap, (int));

#undef E

#endif /* LEGACY_H */
