/* SPDX-License-Identifier: GPL-3.0-only */
/* The engine: 8 voices for four sounds (one per track) in the audio ISR, as the originals' eight voice cards. The main
 * loop sends note events through a ring and writes the live patches and the controllers; the ISR owns the voices, the
 * held-note lists, the arpeggiators and the sustain. */
#ifndef RING_PUBLISH                            /* (one core: the ISR runs whole between two main-loop instructions) */
#define RING_PUBLISH() __asm__ volatile("" ::: "memory")
#endif
#ifndef RING_TAKE
#define RING_TAKE() __asm__ volatile("" ::: "memory")
#endif
#define NV 8u
#define NPART 4u                               /* the four tracks' sounds */
#define NCARD 8u                                /* the originals' voice cards: the tables below, the LEDs */
#define NHELD 16u
#define ENV_ONE (1 << 24)
#define LEVEL_FLOOR 2048                        /* -78 dB: a release below this frees the voice */

enum { E_IDLE, E_ATT, E_DEC, E_SUS, E_REL };
typedef struct { uint32_t stage; int32_t level; } env_t;   /* level Q24 */

typedef struct {
    osc_t o1, o2;
    uint32_t noise;
    env_t fe, ae;
    svf_state_t f1;
    svf_coef_t c1;
    lad_state_t lad;                            /* the 24 dB ladder */
    lad_coef_t lc;
    uint32_t inc1, inc2, pw1, pw2;              /* this block's oscillator settings */
    int32_t amp0, amp1;                         /* Q15 at the block's start and end */
    int32_t pitch16, target16;                  /* Q16 semitones: the glide */
    int32_t det8;                               /* unison / dual offset, Q8 */
    int32_t gl, gr;                             /* pan gains Q12 */
    int32_t drift;                              /* the vintage random walk */
    uint32_t lfo_blk;                           /* blocks since the note on (LFO delay / fade) */
    int32_t lfo_fade;                           /* Q15 */
    uint32_t seq;                               /* allocation order */
    uint8_t note, vel, gate;                    /* gate: the key (or the arp) holds it */
    uint8_t part;                               /* the track whose sound it is, 0..3 */
    uint8_t grp;                                /* a card of its sound's MONO / UNISON group */
    uint8_t mch;                                /* (RAIN) MPE: the member channel + 1 its note came on, 0 none */
    int32_t mbend, mpress, mslide;              /* (RAIN) MPE: its bend (Q4 of -8192..8191), pressure (Q8 0..127),
                                                 * slide (Q8 -64..63): followed through a one-pole, ~3 ms */
    int32_t l1, l2, ln, xq, pan0, rnd;          /* (RAIN 9) this block's levels and X-MOD (the sound's + the matrix's),
                                                 * the pan it started with, its random value (Q15, one per note) */
    int32_t dsd[3], ksd[3];                     /* (RAIN 12) the matrix on its sends into DELAY, DELAY 2, REVERB: the change
                                                 * of the track's send (-127..127), as gains this block (Q15, 0: none) */
    uint8_t kp;                                 /* (RAIN 28) a drum kit's piece + 1 (its own sound, kpt[]), 0: the track's */
} voice_t;

/* four sounds, one per track (A's also sets the effects). The main loop writes, the ISR reads. */
static patch_t live_p[NPART];
#define live (live_p[0])                        /* track A's sound: the effects' settings are its */
/* the mixer (main loop writes): per track a level 0..127, a pan -64..64, a send into the effects 0..127; muted: bit t */
static volatile uint8_t ctl_mix_lvl[NPART] = {100, 100, 100, 100};
static volatile uint8_t ctl_mix_cho[NPART] = {127, 127, 127, 127}, ctl_mix_dly[NPART] = {127, 127, 127, 127}, ctl_mix_dly2[NPART], ctl_mix_rev[NPART] = {127, 127, 127, 127};   /* (VIBE) the three sends */
static volatile int8_t ctl_mix_pan[NPART];
static volatile uint8_t ctl_mix_drv[NPART];   /* (VIBE) the 4th send: DRIVE */
static volatile uint32_t ctl_mix_off;           /* bit t: track t silent (muted, or another one soloed) */
/* the next pattern's mixer, drive and master, prepared by the main loop with the pattern (pat_q), taken by the ISR on
 * the downbeat it swaps the pattern in: the mix changes with the first note, not a frame later */
typedef struct { uint8_t lvl[NPART], cho[NPART], dly[NPART], rev[NPART], drv[NPART]; int8_t pan[NPART]; uint32_t off;
                 uint8_t dv_type, dv_amt, dv_tone, dv_mix, m_vol, m_dj, m_drive, m_comp; } mxq_t;
static mxq_t mx_q;
static volatile uint32_t ctl_mx_swap;
static voice_t vc[NV];
static uint32_t vseq, rr;                       /* allocation counter, round-robin pointer */

/* main loop -> ISR: note events */
typedef struct { uint8_t type, note, vel, part, ch; } nev_t;   /* type 1 on, 0 off; part: the track's sound; ch: (RAIN) an
                                                 * MPE member channel + 1, 0 none */
#define NQ 64u
static nev_t nq[NQ];
static volatile uint32_t nq_w, nq_r;
static void note_event_p(uint32_t part, uint32_t on, uint32_t note, uint32_t vel)
{
    if (nq_w - nq_r >= NQ) return;              /* a flood: drop (the ISR drains 64 per block) */
    nq[nq_w % NQ].part = (uint8_t)part;
    nq[nq_w % NQ].type = (uint8_t)on;
    nq[nq_w % NQ].note = (uint8_t)note;
    nq[nq_w % NQ].vel = (uint8_t)vel;
    nq[nq_w % NQ].ch = 0;
    RING_PUBLISH();
    nq_w++;
}
static void note_event(uint32_t on, uint32_t note, uint32_t vel) { note_event_p(0, on, note, vel); }
/* (RAIN) MPE: a note on member channel ch (0-based, 1..15) of the MPE sound (track A): its voice follows that channel's
 * own bend, pressure and slide */
static volatile int16_t mpe_bend[16];           /* -8192..8191, per channel (main loop writes, ISR reads) */
static volatile uint8_t mpe_press[16];          /* channel pressure 0..127 */
static volatile int8_t mpe_slide[16];           /* CC 74 - 64 */
static volatile uint32_t ctl_mpe_range;         /* the members' bend range in semitones, 0 = MPE off */
static volatile uint32_t ctl_mpe_pdst, ctl_mpe_pamt, ctl_mpe_sdst, ctl_mpe_samt;   /* pressure / slide: MD_* and 0..127 */
enum { MD_OFF, MD_AMP, MD_CUT, MD_VIB, MD_PWM, NMD };
/* (RAIN 9) the modulation matrix, as the Oberheim Matrix-6/12: per track MM_N slots, each a source to a destination by an
 * amount -100..100 %. Worked per voice per control block (16 samples); a track with no slot in use costs nothing */
#define MM_N 8u
enum { MXS_OFF, MXS_LFO, MXS_FENV, MXS_AENV, MXS_VEL, MXS_KEY, MXS_PRESS, MXS_SLIDE, MXS_WHEEL, MXS_RAND, NMXS };
enum { MXD_OFF, MXD_PITCH, MXD_O1, MXD_O2, MXD_PW, MXD_CUT, MXD_RESO, MXD_AMP, MXD_PAN, MXD_O1LVL, MXD_O2LVL, MXD_NOISE,
       MXD_XMOD, MXD_SDLY, MXD_SDLY2, MXD_SREV, NMXD };   /* (RAIN 12: + the sends, per voice) */
typedef struct { uint8_t src, dst; int8_t amt; } mm_slot_t;
static mm_slot_t ctl_mm[NPART][MM_N];           /* (main loop writes, ISR reads) */
static volatile uint8_t ctl_mm_used[NPART];     /* a slot of the track is in use */
static volatile uint8_t ctl_mm_feeds;           /* (RAIN 12) bit 0: a slot sends into DELAY, 1: DELAY 2, 2: REVERB */
static volatile uint8_t ctl_at[NPART];          /* channel pressure (aftertouch) per track, for MPE's PRESS without MPE */
static uint32_t ev_ch;                          /* (ISR) the event being handled: its MPE channel + 1, else 0 */
static void note_event_ch(uint32_t part, uint32_t on, uint32_t note, uint32_t vel, uint32_t ch)
{
    if (nq_w - nq_r >= NQ) return;
    nq[nq_w % NQ].part = (uint8_t)part;
    nq[nq_w % NQ].type = (uint8_t)on;
    nq[nq_w % NQ].note = (uint8_t)note;
    nq[nq_w % NQ].vel = (uint8_t)vel;
    nq[nq_w % NQ].ch = (uint8_t)(ch + 1u);
    RING_PUBLISH();
    nq_w++;
}

/* the controllers (main loop writes) */
static volatile int32_t ctl_bend;               /* -8192..8191 */
static volatile uint32_t ctl_sustain, ctl_hold, ctl_cc7 = 127;   /* ctl_hold: bit p = sound p's HOLD */
#if BOARD_SMK37
static volatile int32_t ctl_vib;                /* the SMK-37's mod wheel 0..127: a vibrato from the LFO, up to +-1/2 semitone */
#endif
static volatile int32_t ctl_tune8;              /* global tune, Q8 semitones */
static volatile uint32_t ctl_tempo = 120, ctl_ext_quarter;   /* MIDI clock: frames per quarter, 0 = none */
static volatile uint32_t ctl_ext_lock, ctl_ext_beats;   /* the transport came from the clock's START (its phase is locked
                                                 * on), and the master's quarters counted since (24 clocks each) */
static volatile uint32_t ctl_arp_restart, ctl_panic;

/* ISR -> main loop */
static volatile int32_t ui_vlevel[NV];          /* each voice's amp envelope, Q15 */
static volatile uint8_t ui_vnote[NV];
static volatile uint32_t ui_render_us, ui_arp_step;
#define SCOPE_N 240u
static int16_t scope[SCOPE_N];
static volatile uint32_t scope_state, scope_stride = 4;   /* 0 idle, 1 armed, 2 running, 3 done */

/* ---- the held notes (ISR): in order of pressing, physically down or held by the pedal / latch */
static struct { uint8_t note, vel, phys; } held[NPART][NHELD];   /* per sound: its keys */
static uint32_t nheld[NPART];
static uint32_t rnd_state = 0x9E3779B9u;

/* ---- the four sounds as the ISR sees them: the patch, its LFO (global per sound, as the originals'), the numbers
 * derived from the patch once per block */
typedef struct {
    const patch_t *p;
    uint32_t lfo_ph, lfo_rnd_a, lfo_rnd_b;
    int32_t lfo_val;                            /* this block, Q15 */
    int32_t last_pitch16;                       /* where a glide starts from */
    int32_t fenv_amt, keytrk, velflt, lfo_pd, lfo_fd, lfo_pwd, lfo_ad, wheel_q, bend8, env_o2, det_fine8;
    int32_t lvl1, lvl2, lvln, drive_q12, reso_k, lad_k, vint, sus_f, sus_a, gain_q12, morph_q;
    int32_t cut_sm, reso_sm;                    /* CUTOFF and RESO slewed (Q8), ~25 ms: a CC sweep in semitone steps glides */
    uint32_t pw1_base, pw2_base, w1, w2, ftype, sync_on, xmod_q, glide_k;
} part_t;
static part_t pt[NPART] = {{&live_p[0], 0, 0, 0, 0, 60 << 16}, {&live_p[1], 0, 0, 0, 0, 60 << 16},
                            {&live_p[2], 0, 0, 0, 0, 60 << 16}, {&live_p[3], 0, 0, 0, 0, 60 << 16}};
/* (RAIN 28/30) the drum kit's five places: a program each (the main loop loads them: kit_sync), in voices whose kp
 * is set. Their numbers are worked out only in a block where one of them sounds */
static patch_t kit_p[NKT], kit_base[NKT];      /* (kit_base: the programs as saved; kit_p: + the kit track's knobs) */
static volatile uint8_t kit_busy;
static volatile uint8_t ctl_kit[NPART][12];     /* (RAIN 31) each track's kit keys as played: 0 silent, 1..NKP a synthesized
                                                 * drum, NKP + 1 the key's program (kit_base[NKP + pitch class]) */               /* the main loop is copying kit_base: this block keeps kit_p as it was */
static uint8_t kit_trk;                         /* the track whose kit played last: its knobs move the drums */
static part_t kpt[NKT];
static uint8_t start_kp;                        /* voice_start: the piece the next voice plays (+ 1), 0 the track's sound */
static const part_t *vpart(const voice_t *v) { return v->kp ? &kpt[(v->kp - 1u) % NKT] : &pt[v->part]; }

static void held_remove(uint32_t p, uint32_t note)
{
    uint32_t i, j;
    for (i = j = 0; i < nheld[p]; i++)
        if (held[p][i].note != note) held[p][j++] = held[p][i];
    nheld[p] = j;
}

/* ---- vintage: each voice card has its own small offsets, scaled by the knob */
static const int16_t VINT_TUNE8[NCARD] = {-20, 12, 7, -15, 18, -9, 3, -17};     /* +-8 cents */
static const int16_t VINT_CUT8[NCARD] = {-300, 250, 120, -380, 60, 330, -180, 220};   /* +-1.5 semitones */
static const int8_t VINT_ENV[NCARD] = {10, -6, 14, -12, 3, -16, 8, -4};         /* /128: +-12 % on the times */
static const int8_t PAN_CARD[NCARD] = {-40, 40, -24, 24, -56, 56, -10, 10};     /* the originals' pan pots */

static void voice_pan(voice_t *v, int32_t pan)   /* -64..64 */
{
    uint32_t pos = (uint32_t)(clampi(pan, -64, 64) + 64);
    v->gl = PAN_Q12[128u - pos];
    v->gr = PAN_Q12[pos];
}

static void voice_start(voice_t *v, uint32_t part, uint32_t note, uint32_t vel, int32_t det8, int32_t pan)
{
    const part_t *g0 = start_kp ? &kpt[(start_kp - 1u) % NKT] : &pt[part];
    const uint8_t *pv = g0->p->v;
    v->kp = start_kp;
    v->part = (uint8_t)part;
    v->note = (uint8_t)note;
    v->vel = (uint8_t)vel;
    v->gate = 1;
    v->seq = ++vseq;
    v->det8 = det8;
    v->target16 = (int32_t)note << 16;
    v->pitch16 = pv[P_GLIDE] ? g0->last_pitch16 : v->target16;
    v->fe.stage = E_ATT;
    v->ae.stage = E_ATT;
    v->lfo_blk = 0;
    v->lfo_fade = pv[P_LFO_DELAY] ? 0 : 32767;
    v->mch = (uint8_t)ev_ch;                    /* (RAIN) MPE: from its channel's values, no sweep from 0 */
    if (ev_ch) {
        v->mbend = mpe_bend[(ev_ch - 1u) & 15u] * 16;
        v->mpress = mpe_press[(ev_ch - 1u) & 15u] * 256;
        v->mslide = mpe_slide[(ev_ch - 1u) & 15u] * 256;
    }
    v->pan0 = pan;
    v->rnd = (int32_t)(xorshift(&v->noise) >> 16) - 32768;   /* (RAIN 9) RAND: one value per note */
    voice_pan(v, pan);
}

static int part_grouped(uint32_t part)
{
    uint32_t m = pt[part].p->v[P_MODE];
    return m == M_MONO || m == M_UNISON;
}

/* the eight cards are shared by the four sounds. POLY / DUAL: round robin over the cards; steal the oldest released,
 * then the oldest. A card that sounds in another sound's MONO / UNISON group is kept (a held bass or lead is never
 * cut by a chord); only if nothing else is left */
static uint32_t voice_alloc(uint32_t part)
{
    uint32_t i, v, pick = 0xFFFFFFFFu, best = 0xFFFFFFFFu;
#define KEPT(v) (vc[v].grp && vc[v].gate && vc[v].part != part)
    for (i = 0; i < NV; i++) {
        v = (rr + i) % NV;
        if (vc[v].ae.stage == E_IDLE && !KEPT(v)) { rr = v + 1u; return v; }
    }
    for (v = 0; v < NV; v++)
        if (!KEPT(v) && !vc[v].gate && vc[v].seq < best) { best = vc[v].seq; pick = v; }
    if (best == 0xFFFFFFFFu)
        for (v = 0; v < NV; v++)
            if (!KEPT(v) && vc[v].seq < best) { best = vc[v].seq; pick = v; }
    if (pick == 0xFFFFFFFFu)                    /* (every card held by another group: the oldest anyway) */
        for (v = 0; v < NV; v++)
            if (vc[v].seq < best) { best = vc[v].seq; pick = v; }
#undef KEPT
    rr = pick + 1u;
    return pick;
}

/* a MONO / UNISON group: n cards for one note. Its own cards first, then idle ones, then released, then the oldest
 * POLY ones; another group's sounding cards last. Returns them in g[] (its order: the detune / pan spread) */
static void group_pick(uint32_t part, uint32_t n, uint8_t *g)
{
    uint32_t i, v, taken = 0;
    for (i = 0; i < n; i++) {
        uint32_t best = 0xFFFFFFFFu, pick = 0, score, bs = 0xFFFFFFFFu;
        for (v = 0; v < NV; v++) {
            if (taken >> v & 1u) continue;
            if (vc[v].part == part && vc[v].grp) score = 0;
            else if (vc[v].ae.stage == E_IDLE && !vc[v].gate) score = 1;
            else if (!vc[v].gate) score = 2;
            else if (!vc[v].grp) score = 3;
            else score = 4;
            if (score < bs || (score == bs && vc[v].seq < best)) { bs = score; best = vc[v].seq; pick = v; }
        }
        taken |= 1u << pick;
        g[i] = (uint8_t)pick;
    }
}

static uint32_t uni_count(uint32_t part)
{
    uint32_t m = pt[part].p->v[P_MODE];
    return m == M_MONO ? 1u : m == M_UNISON ? (uint32_t)clampi(pt[part].p->v[P_UNI_N], 1, (int32_t)NV) : 0u;
}
static uint8_t grp_v[NPART][NV];                /* each sound's group: its cards, in spread order */
static uint32_t grp_n[NPART];

static void voice_trigger(uint32_t part, uint32_t note, uint32_t vel)
{
    const uint8_t *pv = pt[part].p->v;
    uint32_t mode = pv[P_MODE], n, i, legato = 0;
    int32_t det = (pv[P_UNI_DET] * pv[P_UNI_DET]) >> 6, spread = pv[P_SPREAD];
    if (mode == M_KIT || mode == M_SKIT) {      /* (RAIN 28/31) the key's drum, a one-shot: a synthesized one at its own
                                                 * pitch, or the key's program at the key's note (main loop: ctl_kit) */
        uint32_t pc = note % 12u, e = ctl_kit[part % NPART][pc], k, at, v;
        if (!e) return;                         /* (a key set to OFF) */
        if (e <= NKP) { k = e - 1u; at = KIT_NOTE[k]; }
        else { k = NKP + pc; at = note; }
        if (pc == 6u || pc == 8u)               /* a closed hat (F#, G#) cuts the open one (A#) */
            for (v = 0; v < NV; v++)
                if (vc[v].kp && vc[v].part == part && vc[v].note % 12u == 10u) vc[v].gate = 0;
        kpt[k].lfo_ph = 0;                      /* (the clap's flutter starts with the hit) */
        kit_trk = (uint8_t)part;
        v = voice_alloc(part);
        start_kp = (uint8_t)(k + 1u);
        voice_start(&vc[v], part, at, vel, 0, 0);
        start_kp = 0;
        vc[v].note = (uint8_t)note;             /* (the key: its light, the choke) */
        vc[v].grp = 0;
        return;
    }
    if (mode == M_POLY) {
        uint32_t v = voice_alloc(part);
        voice_start(&vc[v], part, note, vel, 0, (PAN_CARD[v % NCARD] * spread) >> 7);
        vc[v].grp = 0;
    } else if (mode == M_DUAL) {
        uint32_t a = voice_alloc(part), b;
        voice_start(&vc[a], part, note, vel, -det / 2, (-48 * spread) >> 7);
        vc[a].grp = 0;
        b = voice_alloc(part);
        voice_start(&vc[b], part, note, vel, det / 2, (48 * spread) >> 7);
        vc[b].grp = 0;
    } else {
        n = uni_count(part);
        if (grp_n[part] == n) {                 /* a key still down in the group (all its cards still its own): legato */
            uint32_t own = 1;
            for (i = 0; i < n; i++) {
                const voice_t *w = &vc[grp_v[part][i]];
                if (w->part != part || !w->grp) own = 0;
                else if (w->gate) legato = 1;
            }
            legato &= own;
        }
        if (!legato) {
            for (i = 0; i < grp_n[part]; i++)   /* (the cards it had and gives up fall silent) */
                if (vc[grp_v[part][i]].part == part && vc[grp_v[part][i]].grp) vc[grp_v[part][i]].gate = 0;
            group_pick(part, n, grp_v[part]);
            grp_n[part] = n;
        }
        for (i = 0; i < n; i++) {
            voice_t *v = &vc[grp_v[part][i]];
            int32_t d = n > 1 ? (det * (int32_t)(2u * i) / (int32_t)(n - 1u)) - det : 0;
            int32_t pan = n > 1 ? ((int32_t)(128u * i) / (int32_t)(n - 1u) - 64) * spread >> 7 : 0;
            if (legato) {                       /* a key still down: slide, no retrigger */
                v->note = (uint8_t)note;
                v->target16 = (int32_t)note << 16;
                if (!pv[P_GLIDE]) v->pitch16 = v->target16;
                v->det8 = d;
                v->gate = 1;
                voice_pan(v, pan);
            } else
                voice_start(v, part, note, vel, d, pan);
            v->grp = 1;
        }
    }
    pt[part].last_pitch16 = (int32_t)note << 16;
    if (pv[P_LFO_PHASE] == LP_SYNC && !legato) {
        for (i = 0; i < NV; i++)
            if (vc[i].gate && vc[i].part == part && vc[i].note != note) break;
        if (i == NV) pt[part].lfo_ph = 0;       /* the first key of a chord restarts the LFO */
    }
}

/* the key (or the arp, the sequencer) let go of a note. In MONO / UNISON a key still held takes over (legato) */
static void voice_release(uint32_t part, uint32_t note, uint32_t from_arp)
{
    const uint8_t *pv = pt[part].p->v;
    uint32_t v, mode = pv[P_MODE];
    if (mode >= M_KIT) return;                  /* (RAIN 28) a drum sounds out: the key's release does not cut it */
    if ((mode == M_MONO || mode == M_UNISON) && !from_arp && nheld[part] && pv[P_ARP_MODE] == A_OFF) {
        uint32_t n = grp_n[part], i;
        voice_t *v0 = &vc[grp_v[part][0]];
        if (n && v0->gate && v0->part == part && v0->grp && v0->note == note) {
            for (i = 0; i < n; i++) {
                voice_t *w = &vc[grp_v[part][i]];
                if (w->part != part || !w->grp) continue;   /* (a card another sound took meanwhile) */
                w->note = held[part][nheld[part] - 1u].note;
                w->target16 = (int32_t)w->note << 16;
                if (!pv[P_GLIDE]) w->pitch16 = w->target16;
            }
            pt[part].last_pitch16 = v0->target16;
        }
        return;
    }
    for (v = 0; v < NV; v++)
        if (vc[v].gate && vc[v].note == note && vc[v].part == part && (!ev_ch || vc[v].mch == ev_ch)) vc[v].gate = 0;
}

static void voices_release_all(void)
{
    uint32_t v;
    for (v = 0; v < NV; v++) vc[v].gate = 0;
}
static void voices_release_part(uint32_t part)
{
    uint32_t v;
    for (v = 0; v < NV; v++)
        if (vc[v].part == part) vc[v].gate = 0;
}

static int seq_phase(uint32_t *ph);             /* the sequencer (its file): running, and where in the beat (frames) */
static int seq_starting(void);                  /* the transport starts (or starts over) in this block */
static void seq_tick(void);                     /* the step sequencer (obv_seq.h): per block, after the arp */

/* the click (main: ctl_click on / off) and a one-bar count-in before a recording (main: ctl_count) */
static volatile uint32_t ctl_click, ctl_count, ui_seq_count;
static int32_t click_amp;
static uint32_t click_ph, click_inc;
static void click_start(uint32_t accent)        /* beat 1 higher and louder */
{
    click_amp = accent ? 9000 : 6000;
    click_ph = 0;
    click_inc = accent ? 233739600u : 155826400u;   /* 2400 / 1600 Hz in 2^32 / 44100 steps */
}

/* ---- the arpeggiators (per block, 0.36 ms steps): one per sound, each on its own keys and its own ARP settings */
typedef struct { uint32_t cnt, period, gate_at, pos, dir, sounding, note, was_on; } arp_t;
static arp_t arp[NPART];

/* ISR -> main loop: the notes the arps play (REC records them, not the keys that feed them), each with the key's
 * velocity and where its track was when it played (step * 256 + how far into it): REC puts it exactly there, however
 * late the main loop takes it */
#define AQ 64u
typedef struct { uint8_t on, note, part, vel; uint32_t q8; } arpev_t;
static arpev_t arp_q[AQ];
static volatile uint32_t arp_qw, arp_qr;
static uint32_t seq_q8_now(uint32_t t);         /* (obv_seq.h) where track t is in this block, 0xFFFFFFFF: not running */
static void arp_post(uint32_t p, uint32_t on, uint32_t note, uint32_t vel)
{
    arpev_t *e;
    if (arp_qw - arp_qr >= AQ) return;
    e = &arp_q[arp_qw % AQ];
    e->on = (uint8_t)on; e->note = (uint8_t)note; e->part = (uint8_t)p; e->vel = (uint8_t)vel;
    e->q8 = seq_q8_now(p);
    RING_PUBLISH();
    arp_qw++;
}

static volatile uint32_t eng_t;                 /* frames the engine has made (the MIDI clock is measured in them) */

/* a quarter in frames. The internal tempo rounded to 48 frames (~1 ms): the beat (counted in blocks of CTL) and every
 * step length (q / 1, 2, 3, 4, 6, 8, 12) come out whole, so the arp, the tracks and the bar stay locked forever (else
 * at 121 BPM the tracks slid off the beat). The tempo is off by at most 0.1 % for it. */
static uint32_t tempo_quarter(void)             /* the tempo's quarter: the MIDI clock's, else ours */
{
    if (ctl_ext_quarter) return ctl_ext_quarter;
    return (60u * FS / (uint32_t)clampi((int32_t)ctl_tempo, 40, 240) + 24u) / 48u * 48u;
}
/* the transport's: one quarter latched on each beat (obv_seq.h, with the MIDI clock's phase correction in it), so a
 * tempo that changes inside a beat cannot put the steps already counted off it; 0 = stopped */
static uint32_t seq_q;
static uint32_t quarter_len(void) { return seq_q ? seq_q : tempo_quarter(); }

static void arp_stop_note(uint32_t p)
{
    arp_t *a = &arp[p];
    if (a->sounding) { voice_release(p, a->note, 1); a->sounding = 0; arp_post(p, 0, a->note, 0); }
}

static void arp_step(uint32_t p)
{
    const uint8_t *pv = pt[p].p->v;
    arp_t *a = &arp[p];
    uint8_t sorted[NHELD];
    uint32_t i, j, n = nheld[p], oct = clampi(pv[P_ARP_OCT], 1, 4), len, k, mode = pv[P_ARP_MODE], idx;
    for (i = 0; i < n; i++) sorted[i] = held[p][i].note;
    if (mode != A_PLAY)
        for (i = 1; i < n; i++) {               /* insertion sort: 16 at most */
            uint8_t t = sorted[i];
            for (j = i; j && sorted[j - 1u] > t; j--) sorted[j] = sorted[j - 1u];
            sorted[j] = t;
        }
    len = n * oct;
    if (mode == A_RND)
        k = xorshift(&rnd_state) % len;
    else if (mode == A_UPDN && len > 2u) {
        if (a->pos >= 2u * len - 2u) a->pos = 0;
        k = a->pos < len ? a->pos : 2u * len - 2u - a->pos;
        a->pos++;
    } else {
        if (a->pos >= len) a->pos = 0;
        k = mode == A_DOWN ? len - 1u - a->pos : a->pos;
        a->pos++;
    }
    idx = k % n;
    a->note = (uint32_t)clampi((int32_t)sorted[idx] + 12 * (int32_t)(k / n), 0, 127);
    for (i = 0; i < n; i++)                     /* the velocity of that key */
        if (held[p][i].note == sorted[idx]) break;
    voice_trigger(p, a->note, i < n ? held[p][i].vel : 100u);
    arp_post(p, 1, a->note, i < n ? held[p][i].vel : 100u);
    a->sounding = 1;
    ui_arp_step++;
}

static void arp_tick(uint32_t p)
{
    const uint8_t *pv = pt[p].p->v;
    arp_t *a = &arp[p];
    uint32_t mode = pv[P_ARP_MODE], div = DIV_PER_QUARTER[pv[P_ARP_DIV] % NDIV], q;
    if (mode == A_OFF) {
        if (a->was_on) { arp_stop_note(p); a->was_on = 0; }
        return;
    }
    if (!a->was_on) { voices_release_part(p); a->was_on = 1; a->pos = 0; a->cnt = 0xFFFFFFFFu; }
    if (!nheld[p]) { arp_stop_note(p); a->pos = 0; a->cnt = 0xFFFFFFFFu; return; }
    q = quarter_len();
    a->period = q / div;
    if (a->period < CTL) a->period = CTL;
    a->gate_at = (a->period * (pv[P_ARP_GATE] + 1u)) >> 7;
    {                                           /* the transport running: on its grid (the first key: at once if just
                                                 * after a line, else on the next one), as a DAW's arp */
        uint32_t ph;
        if (seq_starting()) {                   /* the transport starts in this block: the downbeat */
            if (a->sounding && a->cnt != 0xFFFFFFFFu && a->cnt < a->period / 4u) { a->cnt = 0; return; }   /* (its note
                                                 * just came, e.g. the one that started REC: it is the downbeat's) */
            a->cnt = 0; arp_stop_note(p); arp_step(p); return;
        }
        if (seq_phase(&ph)) {
            if (ph / a->period >= div) return;  /* (the last sliver of a beat: its line is the next beat's start) */
            ph %= a->period;
            if (ph < CTL || (a->cnt == 0xFFFFFFFFu && ph < a->period / 4u)) { a->cnt = ph; arp_stop_note(p); arp_step(p); return; }
            if (a->cnt == 0xFFFFFFFFu) return;  /* (waiting for the line) */
            a->cnt = ph;
            if (a->sounding && a->cnt >= a->gate_at && pv[P_ARP_GATE] < 127u) arp_stop_note(p);
            return;
        }
    }
    if (a->cnt == 0xFFFFFFFFu || a->cnt + CTL >= a->period) {
        a->cnt = a->cnt == 0xFFFFFFFFu ? 0 : a->cnt + CTL - a->period;
        arp_stop_note(p);
        arp_step(p);
    } else {
        a->cnt += CTL;
        if (a->sounding && a->cnt >= a->gate_at && pv[P_ARP_GATE] < 127u) arp_stop_note(p);
    }
}

/* ---- note events (ISR), per sound: its held keys, its arp, its HOLD; the pedal holds them all */
static uint32_t sus_was;                        /* bit p: sound p was held */
static inline uint32_t part_sus(uint32_t p) { return ctl_sustain || (ctl_hold >> p & 1u); }
static void ev_note_on(uint32_t p, uint32_t note, uint32_t vel)
{
    uint32_t i, arp_on = pt[p].p->v[P_ARP_MODE] != A_OFF, sus = part_sus(p);
    if (arp_on && sus) {                        /* latch: a new chord after all keys were up starts over */
        for (i = 0; i < nheld[p]; i++)
            if (held[p][i].phys) break;
        if (i == nheld[p]) { nheld[p] = 0; arp[p].pos = 0; arp[p].cnt = 0xFFFFFFFFu; }
    }
    held_remove(p, note);
    if (nheld[p] == NHELD) { for (i = 1; i < NHELD; i++) held[p][i - 1u] = held[p][i]; nheld[p]--; }
    held[p][nheld[p]].note = (uint8_t)note;
    held[p][nheld[p]].vel = (uint8_t)vel;
    held[p][nheld[p]].phys = 1;
    nheld[p]++;
    if (arp_on) {
        if (nheld[p] == 1u) { arp[p].pos = 0; arp[p].cnt = 0xFFFFFFFFu; }   /* the first key: a step right away */
        return;
    }
    voice_trigger(p, note, vel);
}

static void ev_note_off(uint32_t p, uint32_t note)
{
    uint32_t i;
    for (i = 0; i < nheld[p]; i++)
        if (held[p][i].note == note) held[p][i].phys = 0;
    if (part_sus(p)) return;                    /* the pedal / its HOLD keeps it */
    held_remove(p, note);
    if (pt[p].p->v[P_ARP_MODE] != A_OFF) return;
    voice_release(p, note, 0);
}

static void ev_sustain_up(uint32_t p)           /* the pedal / its HOLD let go: what no key holds is released */
{
    uint32_t i, j;
    {
        uint8_t gone[NHELD], ng = 0;
        for (i = j = 0; i < nheld[p]; i++)
            if (held[p][i].phys) held[p][j++] = held[p][i];
            else gone[ng++] = held[p][i].note;
        nheld[p] = j;
        if (pt[p].p->v[P_ARP_MODE] != A_OFF) return;
        for (i = 0; i < ng; i++) voice_release(p, gone[i], 0);
    }
}

static void parts_off(uint32_t p)               /* a panic: its keys, its arp, its notes */
{
    nheld[p] = 0;
    arp_stop_note(p);
    arp[p].was_on = 0;
    voices_release_part(p);
}

static void engine_events(void)
{
    uint32_t sus = 0, p;
    for (p = 0; p < NPART; p++) sus |= part_sus(p) << p;
    if (ctl_panic) { ctl_panic = 0; for (p = 0; p < NPART; p++) parts_off(p); voices_release_all(); }
    for (p = 0; p < NPART; p++)                 /* each sound on its own: A's HOLD off never touches B */
        if ((sus_was & ~sus) >> p & 1u) ev_sustain_up(p);
    sus_was = sus;
    while (nq_r != nq_w) {
        nev_t e;
        RING_TAKE();
        e = nq[nq_r % NQ];
        p = e.part % NPART;
        nq_r++;
        ev_ch = e.ch;                           /* (RAIN) MPE: the voice it starts or ends is this channel's */
        if (e.type) ev_note_on(p, e.note, e.vel);
        else ev_note_off(p, e.note);
        ev_ch = 0;
    }
}

/* ---- envelopes: analog RC curves per block (CTL frames), level Q24 */
static inline void env_tick(env_t *e, uint32_t a, uint32_t d, int32_t sus, uint32_t r, int32_t gate, int32_t vint)
{
    int32_t k, target;
    if (gate && e->stage >= E_REL) e->stage = E_ATT;
    if (!gate && e->stage && e->stage < E_REL) e->stage = E_REL;
    switch (e->stage) {
    case E_ATT:
        k = (int32_t)ENV_K_ATT[a];
        k += (int32_t)(((int64_t)k * vint) >> 7);
        target = ENV_ONE + (ENV_ONE * 3 >> 3);  /* aims at 1.3, stops at 1.0 */
        e->level += (int32_t)(((int64_t)(target - e->level) * k) >> 24);
        if (e->level >= ENV_ONE) { e->level = ENV_ONE; e->stage = E_DEC; }
        break;
    case E_DEC:
        k = (int32_t)ENV_K_DEC[d];
        k += (int32_t)(((int64_t)k * vint) >> 7);
        e->level += (int32_t)(((int64_t)(sus - e->level) * k) >> 24) - 16;
        if (e->level <= sus + 64) { e->level = sus; e->stage = E_SUS; }
        break;
    case E_SUS:
        e->level = sus;                         /* the knob moves it */
        break;
    case E_REL:
        k = (int32_t)ENV_K_DEC[r];
        k += (int32_t)(((int64_t)k * vint) >> 7);
        e->level += (int32_t)(((int64_t)(0 - e->level) * k) >> 24) - 32;
        if (e->level < LEVEL_FLOOR) { e->level = 0; e->stage = E_IDLE; }
        break;
    default:
        e->level = 0;
    }
}

/* ---- the LFO (global; per voice a phase offset in SPREAD and a delay / fade) */
static int32_t lfo_shape(const part_t *g, uint32_t ph, uint32_t shape)
{
    int32_t p;
    switch (shape) {
    case L_TRI:
        p = (int32_t)((ph + 0x40000000u) >> 15);
        return p < 65536 ? p - 32768 : 98303 - p;
    case L_SAW: return 32767 - (int32_t)(ph >> 16);
    case L_RAMP: return (int32_t)(ph >> 16) - 32768;
    case L_SQR: return ph < 0x80000000u ? 32767 : -32767;
    case L_SH: return (int32_t)(g->lfo_rnd_b >> 16) - 32768;
    default: {                                  /* smooth random: a line from one random value to the next */
        int32_t a = (int32_t)(g->lfo_rnd_a >> 16) - 32768, b = (int32_t)(g->lfo_rnd_b >> 16) - 32768;
        return a + (int32_t)(((int64_t)(b - a) * (ph >> 8)) >> 24);
    }
    }
}

static void lfo_tick(part_t *g)
{
    uint32_t inc = LFO_INC[g->p->v[P_LFO_RATE]], old = g->lfo_ph;
    g->lfo_ph += inc;
    if (g->lfo_ph < old) { g->lfo_rnd_a = g->lfo_rnd_b; g->lfo_rnd_b = xorshift(&rnd_state); }
    g->lfo_val = lfo_shape(g, g->lfo_ph, g->p->v[P_LFO_SHAPE]);
}

/* ---- one control block for a voice: pitch, cutoff, envelopes, amp */
static int32_t vol_q12;                         /* the output stage: CC 7 (each sound's VOLUME is in its voices) */

static void block_globals(part_t *g)            /* a sound's derived numbers, once per block */
{
    const uint8_t *pv = g->p->v;
    int32_t d;
    g->fenv_amt = (int32_t)pv[P_FENV_AMT] - 64;
    g->keytrk = pv[P_KEYTRK];
    g->velflt = pv[P_VEL_FLT];
    d = pv[P_LFO_PITCH]; g->lfo_pd = (d * d) >> 7;
    d = pv[P_LFO_FLT]; g->lfo_fd = (d * d) >> 7;
    d = pv[P_LFO_PWM]; g->lfo_pwd = (d * d) >> 7;
    g->lfo_ad = pv[P_LFO_AMP];
    g->wheel_q = pv[P_MOD_DEPTH];               /* the knob and the mod wheel (CC 1) are the same value */
    g->bend8 = (ctl_bend * pv[P_BEND] * 256) >> 13;
    g->env_o2 = (int32_t)pv[P_ENV_O2] - 64;
    g->det_fine8 = ((int32_t)pv[P_O2_FINE] - 64) * 2;
    d = pv[P_O1_LVL]; g->lvl1 = (d * d) >> 7;
    d = pv[P_O2_LVL]; g->lvl2 = (d * d) >> 7;
    d = pv[P_NOISE]; g->lvln = (d * d) >> 7;
    d = pv[P_DRIVE]; g->drive_q12 = 4096 + (d * d * 15 * 4096 / 16129);   /* 1 .. 16 into the filter's input stage */
    g->cut_sm += ((pv[P_CUTOFF] << 8) - g->cut_sm) >> 6;   /* tau = 64 blocks = 23 ms */
    g->reso_sm += ((pv[P_RESO] << 8) - g->reso_sm) >> 6;
    g->reso_k = 8192 - g->reso_sm * 8400 / (127 * 256);
    g->lad_k = (8192 - g->reso_k) >> 1;         /* the ladder's feedback, Q10: 0 .. 4.1 */
    g->vint = pv[P_VINTAGE];
    g->sus_f = pv[P_FS] * (ENV_ONE / 127);
    g->sus_a = pv[P_AS] * (ENV_ONE / 127);
    d = pv[P_VOLUME];
    g->gain_q12 = (d * d) >> 2;                 /* (x CC 7 at the output: as VOLUME x CC 7 squared was) */
    g->pw1_base = ((uint32_t)pv[P_O1_PW] << 25) + (1u << 24);
    g->pw2_base = ((uint32_t)pv[P_O2_PW] << 25) + (1u << 24);
    g->w1 = pv[P_O1_WAVE];
    g->w2 = pv[P_O2_WAVE];
    g->ftype = pv[P_FTYPE];
    g->morph_q = pv[P_FMORPH];                  /* SEM: 0 LP, 64 notch, 127 HP */
    g->sync_on = pv[P_SYNC];
    d = pv[P_XMOD]; g->xmod_q = (uint32_t)((d * d) >> 7);
    g->glide_k = GLIDE_K[pv[P_GLIDE]];
}

static inline uint32_t pw_mod(uint32_t base, int32_t mod)
{
    int64_t p = (int64_t)base + ((int64_t)mod << 8);
    if (p < (1 << 26)) p = 1 << 26;
    if (p > (int64_t)0xFFFFFFFFu - (1 << 26)) p = (int64_t)0xFFFFFFFFu - (1 << 26);
    return (uint32_t)p;
}

static void voice_block(voice_t *v, uint32_t card)
{
    const part_t *g = vpart(v);
    const uint8_t *pv = g->p->v;
    int32_t lfo, p8, p1, p2, cut, fenv, tmp, velg, trem, vint = g->vint, vt = (vint * VINT_ENV[card % NCARD]) >> 7, lfo_to, a;
    int32_t mq_p = 0, mq_s = 0, mpw = 0;      /* (RAIN) MPE: pressure, slide (Q15), their pulse width offset */
    int32_t mx[NMXD];                           /* (RAIN 9) the matrix per destination, Q15 */
    uint32_t ph, glide_k = g->glide_k;
    /* glide */
    if (v->pitch16 != v->target16) {
        if (!glide_k) v->pitch16 = v->target16;
        else {
            int32_t diff = v->target16 - v->pitch16;
            v->pitch16 += (int32_t)(((int64_t)diff * glide_k) >> 16);
            if (diff > -64 && diff < 64) v->pitch16 = v->target16;
        }
    }
    /* envelopes */
    env_tick(&v->fe, pv[P_FA], pv[P_FD], g->sus_f, pv[P_FR], v->gate, vt);
    env_tick(&v->ae, pv[P_AA], pv[P_AD], g->sus_a, pv[P_AR], v->gate, vt);
    fenv = v->fe.level >> 9;                    /* Q15 */
    /* the LFO as this voice sees it */
    ph = g->lfo_ph;
    if (pv[P_LFO_PHASE] == LP_SPREAD) ph += card << 29;
    lfo = pv[P_LFO_PHASE] == LP_SPREAD ? lfo_shape(g, ph, pv[P_LFO_SHAPE]) : g->lfo_val;
    if (v->lfo_fade < 32767) {
        uint32_t dl = LFO_DELAY_BLK[pv[P_LFO_DELAY]];
        if (v->lfo_blk >= dl) { v->lfo_fade += 32767 / (int32_t)(dl / 2u + 1u) + 1; if (v->lfo_fade > 32767) v->lfo_fade = 32767; }
        v->lfo_blk++;
    }
    lfo = (lfo * v->lfo_fade) >> 15;
    /* vintage: a slow random walk on the pitch */
    v->drift += (int32_t)(xorshift(&v->noise) & 31u) - 16;
    v->drift -= v->drift >> 11;
    /* pitch, Q8 semitones */
    p8 = (v->pitch16 >> 8) + g->bend8 + ctl_tune8 + v->det8 + ((VINT_TUNE8[card % NCARD] * vint) >> 7) + ((v->drift * vint) >> 10);
    p8 += (lfo * g->wheel_q * 4) >> 15;         /* the wheel's vibrato */
    if (v->mch) {                               /* (RAIN) MPE: this note's own bend, pressure and slide */
        uint32_t c = (v->mch - 1u) & 15u;
        v->mbend += (mpe_bend[c] * 16 - v->mbend) >> 2;
        v->mpress += (mpe_press[c] * 256 - v->mpress) >> 2;
        v->mslide += (mpe_slide[c] * 256 - v->mslide) >> 2;
        p8 += (int32_t)(((int64_t)v->mbend * (int32_t)ctl_mpe_range * 256) >> 17);
        mq_p = (v->mpress * 258) >> 8;          /* Q15 0..32766 */
        mq_s = v->mslide * 2;                   /* Q15 -32768..32766 */
        if (ctl_mpe_pdst == MD_VIB) p8 += (((lfo * mq_p) >> 15) * (int32_t)ctl_mpe_pamt) >> 13;
        if (ctl_mpe_sdst == MD_VIB) p8 += (((lfo * (mq_s < 0 ? -mq_s : mq_s)) >> 15) * (int32_t)ctl_mpe_samt) >> 13;
    }
    {                                           /* (RAIN 9) the matrix: the sources, summed per destination (Q15 each) */
        uint32_t s, pa = v->part % NPART;
        int32_t i;
        for (i = 0; i < NMXD; i++) mx[i] = 0;
        if (ctl_mm_used[pa]) {
            if (!v->mch) mq_p = (int32_t)ctl_at[pa] * 258;   /* (PRESS without MPE: the track's aftertouch) */
            for (s = 0; s < MM_N; s++) {
                const mm_slot_t *m = &ctl_mm[pa][s];
                int32_t val;
                if (!m->amt || !m->src || !m->dst || m->src >= NMXS || m->dst >= NMXD) continue;
                switch (m->src) {
                case MXS_LFO: val = lfo; break;
                case MXS_FENV: val = fenv; break;
                case MXS_AENV: val = v->ae.level >> 9; break;
                case MXS_VEL: val = (int32_t)v->vel * 258; break;
                case MXS_KEY: val = clampi(((int32_t)v->note - 60) * 546, -32767, 32767); break;   /* +-5 octaves around C4 */
                case MXS_PRESS: val = mq_p; break;
                case MXS_SLIDE: val = mq_s; break;
                case MXS_WHEEL: val = (int32_t)pv[P_MOD_DEPTH] * 258; break;
                default: val = v->rnd; break;
                }
                mx[m->dst] += (val * ((int32_t)m->amt * 328)) >> 15;   /* (100 % = x 1.0) */
            }
            p8 += (mx[MXD_PITCH] * 3) >> 5;     /* 100 %: an octave */
        }
    }
#if BOARD_SMK37
    p8 += (lfo * ctl_vib) >> 15;                /* the SMK-37's mod wheel: pitch only, gentle */
#endif
    lfo_to = (lfo * g->lfo_pd * 24) >> 15;
    p1 = p8 + (pv[P_LFO_PITCH_TO] != TO_O2 ? lfo_to : 0);
    p2 = p8 + (pv[P_LFO_PITCH_TO] != TO_O1 ? lfo_to : 0) + ((int32_t)pv[P_O2_SEMI] - 24) * 256 + g->det_fine8
       + ((fenv * g->env_o2 * 97) >> 15);
    p1 += (mx[MXD_O1] * 3) >> 5;               /* (RAIN 9) */
    p2 += (mx[MXD_O2] * 3) >> 5;
    v->inc1 = pitch_inc(p1);
    v->inc2 = pitch_inc(p2);
    tmp = (lfo * g->lfo_pwd);
    if (v->mch) {                               /* (RAIN) MPE to the pulse width: both oscillators */
        if (ctl_mpe_pdst == MD_PWM) mpw += mq_p * (int32_t)ctl_mpe_pamt;   /* (as the LFO's PWM at full depth) */
        if (ctl_mpe_sdst == MD_PWM) mpw += mq_s * (int32_t)ctl_mpe_samt;
    }
    mpw += mx[MXD_PW] * 127;                    /* (RAIN 9: 100 % = the LFO's PWM at full depth) */
    v->pw1 = pw_mod(g->pw1_base, (pv[P_LFO_PWM_TO] != TO_O2 ? tmp : 0) + mpw);
    v->pw2 = pw_mod(g->pw2_base, (pv[P_LFO_PWM_TO] != TO_O1 ? tmp : 0) + mpw);
    /* cutoff, Q8 semitones */
    cut = g->cut_sm + 8 * 256 + ((fenv * g->fenv_amt * 390) >> 15)
        + ((int32_t)v->note - 60) * g->keytrk * 2 + (((int32_t)v->vel - 64) * g->velflt >> 1)
        + ((lfo * g->lfo_fd * 97) >> 15) + ((lfo * g->wheel_q * 48) >> 15) + ((VINT_CUT8[card % NCARD] * vint) >> 7)
        + ((v->drift * vint) >> 7);             /* the vintage walk on the cutoff too: ~1/4 semitone at the default */
    if (v->mch) {                               /* (RAIN) MPE to the cutoff: up to 4 octaves, pressure up, slide both ways */
        if (ctl_mpe_pdst == MD_CUT) cut += (mq_p * (int32_t)ctl_mpe_pamt * 96) >> 15;
        if (ctl_mpe_sdst == MD_CUT) cut += (mq_s * (int32_t)ctl_mpe_samt * 96) >> 15;
    }
    cut += (mx[MXD_CUT] * 3) >> 3;              /* (RAIN 9) 100 %: 4 octaves */
    if (mx[MXD_RESO]) {                         /* (RAIN 9) the resonance per voice: 100 % = its whole range */
        int32_t rs = clampi(g->reso_sm + mx[MXD_RESO], 0, 127 * 256), rk = 8192 - ((rs * 8467) >> 15);
        if (g->ftype == F_LP24) lad_coef(&v->lc, cut, (8192 - rk) >> 1);
        else svf_coef(&v->c1, cut, rk);
    }
    else if (g->ftype == F_LP24) lad_coef(&v->lc, cut, g->lad_k);
    else svf_coef(&v->c1, cut, g->reso_k);
    v->l1 = clampi(g->lvl1 + ((mx[MXD_O1LVL] * 127) >> 15), 0, 127);   /* (RAIN 9) the mix per voice */
    v->l2 = clampi(g->lvl2 + ((mx[MXD_O2LVL] * 127) >> 15), 0, 127);
    v->ln = clampi(g->lvln + ((mx[MXD_NOISE] * 127) >> 15), 0, 127);
    v->xq = clampi((int32_t)g->xmod_q + ((mx[MXD_XMOD] * 127) >> 15), 0, 127);
    v->dsd[0] = clampi((mx[MXD_SDLY] * 127) >> 15, -127, 127);   /* (RAIN 12) +100 %: the whole send on top */
    v->dsd[1] = clampi((mx[MXD_SDLY2] * 127) >> 15, -127, 127);
    v->dsd[2] = clampi((mx[MXD_SREV] * 127) >> 15, -127, 127);
    if (mx[MXD_PAN]) voice_pan(v, v->pan0 + ((mx[MXD_PAN] * 64) >> 15));
    else if (ctl_mm_used[v->part % NPART]) voice_pan(v, v->pan0);
    /* amp */
    velg = 32767 - ((((127 - (int32_t)v->vel) * (int32_t)pv[P_VEL_AMP]) >> 7) * 258);
    trem = 32767 - (((32767 - lfo) * g->lfo_ad) >> 8);
    v->amp0 = v->amp1;
    a = (int32_t)(((int64_t)(v->ae.level >> 9) * velg >> 15) * trem >> 15);
    if (v->mch && ctl_mpe_pdst == MD_AMP)       /* (RAIN) MPE: the pressure plays the level (at full AMOUNT: none, no sound) */
        a = (int32_t)(((int64_t)a * (32767 - (((32767 - mq_p) * (int32_t)ctl_mpe_pamt) >> 7))) >> 15);
    if (v->mch && ctl_mpe_sdst == MD_AMP) {     /* (RAIN) the slide: louder up, softer down */
        int32_t gs = 32767 + ((mq_s * (int32_t)ctl_mpe_samt) >> 8);
        a = (int32_t)(((int64_t)a * (gs < 0 ? 0 : gs > 49151 ? 49151 : gs)) >> 15);
    }
    if (mx[MXD_AMP]) a = (int32_t)(((int64_t)a * clampi(32767 + mx[MXD_AMP], 0, 65534)) >> 15);   /* (RAIN 9) up to x 2 */
    v->amp1 = (a * g->gain_q12) >> 12;          /* the sound's VOLUME */
    ui_vlevel[card] = a;
    ui_vnote[card] = v->note;
}

/* ---- the samples of one voice: oscillators -> mix -> input stage (drive) -> filter -> amp -> pan */
static void voice_render(voice_t *v, int32_t *out, int32_t *sd, int32_t *sd2, int32_t *sr)
{
    int32_t k0 = v->ksd[0], k1 = v->ksd[1], k2 = v->ksd[2];   /* (RAIN 12) this voice's own sends (0: none) */
    uint32_t i, inc1 = v->inc1, inc2 = v->inc2, pw1 = v->pw1, pw2 = v->pw2, d1 = inc1 >> 15, d2 = inc2 >> 15;
    int32_t amp0 = v->amp0, da = v->amp1 - v->amp0, gl = v->gl, gr = v->gr, o2n = v->o2.prev;
    const part_t *g = vpart(v);
    int32_t l1 = v->l1, l2 = v->l2, ln = v->ln, drv = g->drive_q12, morph_q = g->morph_q;   /* (RAIN 9: the voice's) */
    uint32_t ft = g->ftype, xq = (uint32_t)v->xq, sy = g->sync_on, ww1 = g->w1, ww2 = g->w2;
    int32_t hw1 = (ww1 & OSC_SAW ? -65536 : 0) + (ww1 & OSC_PLS ? -65536 : 0);
    int32_t hw2 = (ww2 & OSC_SAW ? -65536 : 0) + (ww2 & OSC_PLS ? -65536 : 0);
    for (i = 0; i < CTL; i++) {
        int32_t s1, s2, mix, lp, bp, hp, y, amp, rt = -1;
        uint32_t ie = inc1;
        if (xq) {                               /* OSC2 -> OSC1 linear FM, the step never below inc/16 */
            int32_t m = (int32_t)(inc1 >> 8 > 0x200000u ? 0x200000u : inc1 >> 8) * (((o2n >> 8) * (int32_t)xq) >> 6);
            m += (int32_t)inc1;
            ie = m < (int32_t)(inc1 >> 4) ? inc1 >> 4 : (uint32_t)m;
            d1 = ie >> 15;
        }
        s1 = osc_step(&v->o1, ie, d1, pw1, ww1, hw1, -1);
        if (sy && v->o1.ph < ie && d1)
            rt = (int32_t)(v->o1.ph / d1);
        s2 = osc_step(&v->o2, inc2, d2, pw2, ww2, hw2, rt);
        o2n = v->o2.prev;
        mix = s1 * l1 + s2 * l2;
        if (ln) mix += ((int32_t)(xorshift(&v->noise) >> 16) - 32768) * ln;
        mix >>= 7;
        mix = sat_in(((mix >> 3) * drv) >> 9);  /* the input stage: DRIVE's gain, then a tanh (never a hard clip) */
        if (ft == F_LP24)
            y = lad_step(&v->lc, &v->lad, mix);
        else {
            svf_step(&v->c1, &v->f1, mix, &lp, &bp, &hp);
            switch (ft) {
            case F_HP12: y = hp; break;
            case F_BP12: y = bp; break;
            case F_NOTCH: y = hp + lp; break;
            case F_SEM: y = lp + (((hp - lp) * morph_q) >> 7); break;   /* the SEM's LP .. notch .. HP pot */
            default: y = lp; break;
            }
        }
        y = sat_knee(y);
        amp = amp0 + ((da * (int32_t)i) >> 4);
        y = (y * amp) >> 16;
        out[2u * i] += (y * gl) >> 12;
        out[2u * i + 1u] += (y * gr) >> 12;
        if (k0) sd[i] += (y * k0) >> 15;
        if (k1) sd2[i] += (y * k1) >> 15;
        if (k2) sr[i] += (y * k2) >> 15;
    }
    (void)o2n;
}

/* the mixer: each sound into its own bus, then by its level and pan to the output, and by its three sends into the
 * effects (VIBE: chorus, delay, reverb side by side, as return tracks; their MIX = the return's level) */
static int32_t bus[NPART][2u * HALF_FRAMES], snd_v[HALF_FRAMES];
/* (RAIN 17) the sends into chorus, delay, delay 2, reverb in two sets: core 0 fills one while core 1 works the effects
 * on the other (the block before), into fx_wet; core 0 adds fx_wet when its own block is done. One core: set 0 only */
static int32_t snd_buf[2][4][HALF_FRAMES] __attribute__((section(".pool")));
static int32_t fx_wet[2u * HALF_FRAMES] __attribute__((section(".pool")));
static int32_t *snd_c = snd_buf[0][0], *snd_d = snd_buf[0][1], *snd_d2 = snd_buf[0][2], *snd_r = snd_buf[0][3];
static uint32_t snd_cur, fxq_ready, fxq_n, fxq_quarter;
static void fx_job(uint32_t set)                /* (core 1, or core 0 when core 1 gave up) the effects of a set of sends */
{
    uint32_t i;
    int32_t (*q)[HALF_FRAMES] = snd_buf[set & 1u];
    fx_block(&live, fxq_quarter);
    for (i = 0; i < 2u * fxq_n; i++) fx_wet[i] = 0;
    fx_process(fx_wet, q[0], q[1], q[2], q[3], fxq_n, 0);
}
static volatile uint32_t ui_trk_peak[NPART];   /* (VIBE) each track's peak after its fader, for the mixer's meters (the UI takes it) */
static int32_t mix_g[NPART];                    /* the level as it moves (Q12, slewed per block: no zipper) */

static void engine_audio(int32_t *out, uint32_t n)
{
    uint32_t b, v, i, p, used = 0, pipe, kicked = 0;
    static int32_t sc_prev;
    static uint32_t sc_i, sc_sub;
    {                                           /* (RAIN 5) a delay without a send rests (its tail rings out first) */
        uint32_t t, s1 = 0, s2 = 0;
        for (t = 0; t < NPART; t++) { s1 |= ctl_mix_dly[t]; s2 |= ctl_mix_dly2[t]; }
        fx_has_d1 = s1 != 0 || (ctl_mm_feeds & 1u); fx_has_d2 = s2 != 0 || (ctl_mm_feeds & 2u);   /* (RAIN 12: + the matrix) */
    }
    pipe = (uint32_t)cpu1_on();                 /* (RAIN 17) two cores: the effects of the block before, beside this one */
    fx_lat = pipe ? HALF_FRAMES : 0u;
    if (pipe && fxq_ready) {
        fxq_quarter = tempo_quarter();
        kicked = (uint32_t)cpu1_run(fx_job, snd_cur ^ 1u);
    }
    if (!pipe) fx_block(&live, tempo_quarter());   /* (the delay: not each beat's phase correction, its tape slew would wobble) */
    vol_q12 = (int32_t)((ctl_cc7 * ctl_cc7) >> 2);
    for (p = 0; p < NPART; p++)
        for (i = 0; i < 2u * n; i++) bus[p][i] = 0;
    for (i = 0; i < n; i++) snd_c[i] = snd_d[i] = snd_d2[i] = snd_r[i] = snd_v[i] = 0;
    for (b = 0; b < n; b += CTL) {
        engine_events();
        eng_t += CTL;
        if (ctl_arp_restart) { ctl_arp_restart = 0; for (p = 0; p < NPART; p++) { arp[p].pos = 0; arp[p].cnt = 0xFFFFFFFFu; } }
        for (p = 0; p < NPART; p++) arp_tick(p);
        seq_tick();                             /* (may swap the pattern in) */
        for (p = 0; p < NPART; p++) { lfo_tick(&pt[p]); block_globals(&pt[p]); }
        {   /* (RAIN 28) the drum pieces that sound; a one-shot lets go of its key once its decay has run out */
            uint32_t kl = 0;
            for (v = 0; v < NV; v++)
                if (vc[v].kp && (vc[v].gate || vc[v].ae.stage != E_IDLE)) {
                    if (vc[v].gate && vc[v].ae.stage >= E_SUS) vc[v].gate = 0;   /* (whatever its program's sustain) */
                    kl |= 1u << ((vc[v].kp - 1u) % NKT);
                }
            if (kl) {   /* the kit track's CUTOFF, RESO, ENV AMT, RELEASE (= the decay too: one-shots), DRIVE move every drum
                         * by how far they are from their defaults: each keeps its own character */
                static const uint8_t KN[6] = {P_CUTOFF, P_RESO, P_FENV_AMT, P_AR, P_AD, P_DRIVE};
                const uint8_t *tv = pt[kit_trk % NPART].p->v;
                int32_t dl[6];
                uint32_t j;
                for (j = 0; j < 6u; j++) {
                    uint32_t q = j == 4u ? P_AR : KN[j];   /* (RELEASE moves the decay as well) */
                    dl[j] = (int32_t)tv[q] - (int32_t)PDEF[q].def;
                }
                for (p = 0; p < NKT; p++) {
                    if (!(kl >> p & 1u)) continue;
                    if (!kit_busy) {
                        kit_p[p] = kit_base[p];
                        for (j = 0; j < 6u; j++)
                            kit_p[p].v[KN[j]] = (uint8_t)clampi((int32_t)kit_base[p].v[KN[j]] + dl[j], 0, PDEF[KN[j]].max);
                    }
                    lfo_tick(&kpt[p]);
                    block_globals(&kpt[p]);
                }
            }
        }
        for (v = 0; v < NV; v++) {
            if (vc[v].ae.stage == E_IDLE && !vc[v].gate) {
                if (ui_vlevel[v]) { ui_vlevel[v] = 0; vc[v].amp0 = vc[v].amp1 = 0; }
                continue;
            }
            p = vc[v].part % NPART;
            used |= 1u << p;
            voice_block(&vc[v], v);
            {                                   /* (RAIN 12) the matrix's sends: the track's send + the change, after its
                                                 * fader (as the mixer's: the bus's mid is 0.707 of the voice) */
                voice_t *vv = &vc[v];
                static const volatile uint8_t *const SND[3] = {ctl_mix_dly, ctl_mix_dly2, ctl_mix_rev};
                uint32_t j;
                for (j = 0; j < 3u; j++) {
                    int32_t dd = vv->dsd[j], s0 = SND[j][p];
                    if (!dd || (ctl_mix_off >> p & 1u)) { vv->ksd[j] = 0; continue; }
                    if (dd < -s0) dd = -s0;
                    if (dd > 127 - s0) dd = 127 - s0;
                    vv->ksd[j] = (int32_t)((((int64_t)dd * 23170 >> 7) * mix_g[p]) >> 12);
                }
            }
            voice_render(&vc[v], bus[p] + 2u * b, snd_d + b, snd_d2 + b, snd_r + b);
        }
    }
    for (p = 0; p < NPART; p++) {               /* the mixer */
        int32_t lvl = ctl_mix_off >> p & 1u ? 0 : (int32_t)ctl_mix_lvl[p], target = lvl * lvl * 4096 / 10000, g0 = mix_g[p], gl, gr, sc, sd, sd2, sr, sv;   /* 100 = unity */
        uint32_t pos = (uint32_t)(clampi(ctl_mix_pan[p], -64, 64) + 64);
        if (!(used >> p & 1u)) { mix_g[p] = target; continue; }
        mix_g[p] += (target - g0) / 2;
        gl = PAN_Q12[128u - pos] * 4096 / 2896; /* (centre = unity: the pan law's -3 dB taken back) */
        gr = PAN_Q12[pos] * 4096 / 2896;
        gl = gl > 4096 ? 4096 : gl; gr = gr > 4096 ? 4096 : gr;
        sc = ctl_mix_cho[p]; sd = ctl_mix_dly[p]; sd2 = ctl_mix_dly2[p]; sr = ctl_mix_rev[p]; sv = ctl_mix_drv[p];
        {
        int32_t g = g0 << 8, dg = ((mix_g[p] - g0) << 8) / (int32_t)n;   /* (the level glides over the half, Q8) */
        uint32_t pk = 0;
        int32_t any_send = sc | sd | sd2 | sr | sv;
        for (i = 0; i < n; i++, g += dg) {     /* (RAIN 16) level x pan as one Q12 gain, 32-bit (was two 64-bit products); the
                                                 * bus / 4 first (it can reach ~524k: no overflow), as the output stage does */
            int32_t gq = g >> 8, Gl = (gq * gl) >> 12, Gr = (gq * gr) >> 12;
            int32_t l = ((bus[p][2u * i] >> 2) * Gl) >> 10, r = ((bus[p][2u * i + 1u] >> 2) * Gr) >> 10;
            uint32_t al = (uint32_t)(l < 0 ? -l : l), ar = (uint32_t)(r < 0 ? -r : r);
            if (al > pk) pk = al;
            if (ar > pk) pk = ar;
            out[2u * i] += l; out[2u * i + 1u] += r;
            if (any_send) {                     /* (RAIN 16: a track that sends nowhere skips the five sums) */
                int32_t m = (l + r) >> 1;
                snd_c[i] += (m * sc) >> 7; snd_d[i] += (m * sd) >> 7; snd_d2[i] += (m * sd2) >> 7; snd_r[i] += (m * sr) >> 7; snd_v[i] += (m * sv) >> 7;
            }
        }
        if (pk > ui_trk_peak[p]) ui_trk_peak[p] = pk;
        }
    }
    for (i = 0; i < n; i++) {                   /* the output stage: a gentle tanh, never a hard clip */
        int32_t l = softclip((clampi(out[2u * i], -262144, 262143) >> 2) * vol_q12 >> 10);
        int32_t r = softclip((clampi(out[2u * i + 1u], -262144, 262143) >> 2) * vol_q12 >> 10);
        int32_t m = (l + r) >> 1;
        out[2u * i] = l;
        out[2u * i + 1u] = r;
        if (scope_state == 1u) {                /* armed: wait for a rising zero crossing */
            if (sc_prev < 0 && m >= 0) { scope_state = 2; sc_i = 0; sc_sub = 0; }
        } else if (scope_state == 2u) {
            if (!sc_sub++) scope[sc_i++] = (int16_t)m;
            if (sc_sub >= scope_stride) sc_sub = 0;
            if (sc_i >= SCOPE_N) scope_state = 3;
        }
        sc_prev = m;
    }
    for (i = 0; i < n; i++) {                   /* the sends at the output's scale (the effects work after the output stage) */
        snd_c[i] = sat16((clampi(snd_c[i], -262144, 262143) >> 2) * vol_q12 >> 10);
        snd_d[i] = sat16((clampi(snd_d[i], -262144, 262143) >> 2) * vol_q12 >> 10);
        snd_d2[i] = sat16((clampi(snd_d2[i], -262144, 262143) >> 2) * vol_q12 >> 10);
        snd_r[i] = sat16((clampi(snd_r[i], -262144, 262143) >> 2) * vol_q12 >> 10);
        snd_v[i] = sat16((clampi(snd_v[i], -262144, 262143) >> 2) * vol_q12 >> 10);
    }
    if (pipe) {                                 /* (RAIN 17) the effects' returns of the block before */
        if (fxq_ready) {
            if (!kicked || cpu1_wait() != 0) fx_job(snd_cur ^ 1u);   /* (core 1 refused or gave up: here, this once) */
#ifndef OBV_HOST
            else { c1_diag.stage = 5; c1_diag.jobs++; }   /* (RAIN 25) the diagnosis: one more block from core 1 */
#endif
            for (i = 0; i < 2u * n && i < 2u * fxq_n; i++) out[i] = sat16(soft16(out[i] + fx_wet[i]));
        }
        fxq_ready = 1; fxq_n = n;
        snd_cur ^= 1u;
        snd_c = snd_buf[snd_cur][0]; snd_d = snd_buf[snd_cur][1]; snd_d2 = snd_buf[snd_cur][2]; snd_r = snd_buf[snd_cur][3];
    } else {
        fx_process(out, snd_c, snd_d, snd_d2, snd_r, n, 1);
        fxq_ready = 0;
    }
    drive_process(out, snd_v, n);
    master_process(out, n);
    for (i = 0; i < n && click_amp; i++) {     /* the click: after the effects, never recorded */
        int32_t c;
        click_ph += click_inc;
        c = click_ph & 0x80000000u ? click_amp : -click_amp;
        out[2u * i] += c;
        out[2u * i + 1u] += c;
        click_amp -= (click_amp >> 8) + 1;      /* ~6 ms */
        if (click_amp < 64) click_amp = 0;
    }
}

static int engine_quiet(void)                   /* nothing sounds: a flash erase is safe */
{
    uint32_t v;
    for (v = 0; v < NV; v++)
        if (vc[v].ae.stage != E_IDLE || vc[v].gate) return 0;
    return nq_r == nq_w;
}

static void engine_init(void)
{
    uint32_t v;
    for (v = 0; v < NV; v++) {
        vc[v].noise = 0x12345678u * (v + 1u) + 0x9E3779B9u;
        vc[v].o1.ph = xorshift(&vc[v].noise);  /* free-running oscillators start anywhere */
        vc[v].o2.ph = xorshift(&vc[v].noise);
        vc[v].gl = vc[v].gr = 2896;
    }
    for (v = 0; v < NPART; v++) {
        pt[v].lfo_rnd_a = xorshift(&rnd_state);
        pt[v].lfo_rnd_b = xorshift(&rnd_state);
    }
    for (v = 0; v < NKT; v++) {                 /* (RAIN 28/30) the drums, ready (their CUTOFF / RESO not gliding in): the
                                                 * DRUM KIT's made here, the SOUND KIT's loaded by the main loop (kit_sync) */
        if (v < NKP) patch_from_factory(&kit_base[v], &KIT_PC[v]);
        else patch_defaults(&kit_base[v]);
        kit_p[v] = kit_base[v];
        kpt[v].p = &kit_p[v];
        kpt[v].last_pitch16 = 60 << 16;
        kpt[v].lfo_rnd_a = xorshift(&rnd_state);
        kpt[v].lfo_rnd_b = xorshift(&rnd_state);
        kpt[v].cut_sm = kit_p[v].v[P_CUTOFF] << 8;
        kpt[v].reso_sm = kit_p[v].v[P_RESO] << 8;
    }
    fx_init();
}
