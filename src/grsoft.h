/*
 *
 * FACILITY:
 *
 *	VAX C Run-Time System
 *
 * ABSTRACT:
 *
 *	This module contains some definitions for GR/GR3
 *
 * AUTHOR:
 *
 *	Josef Heinen
 *
 * VERSION:
 *
 *	V1.0
 *
 */


/* Entry point definitions */

#if !defined (cray) && !defined (_WIN32)
#if defined (VMS) || ((defined (hpux) || defined (aix)) && !defined(NAGware))

#define GR00DG	gr00dg
#define GR90DG	gr90dg
#define GRARRW	grarrw
#define GRAXLIN	graxlin
#define GRAXLOG	graxlog
#define GRAXS	graxs
#define GRAXSL	graxsl
#define GRBLD	grbld
#define GRCHN	grchn
#define GRCHNC	grchnc
#define GRCHRC	grchrc
#define GRCLP	grclp
#define GRCRCL	grcrcl
#define GRDCUR	grdcur
#define GRDEL	grdel
#define GRDN	grdn
#define GRDRAX	grdrax
#define GRDRDM	grdrdm
#define GRDRDU	grdrdu
#define GRDRHS	grdrhs
#define GRDRKU	grdrku
#define GRDRLG	grdrlg
#define GRDRNE	grdrne
#define GRDRW	grdrw
#define GRDRWS	grdrws
#define GRDSH	grdsh
#define GREND	grend
#define GRENDE	grende
#define GRFILL	grfill
#define GRFONT	grfont
#define GRFRBN	grfrbn
#define GRGFLD	grgfld
#define GRJMP	grjmp
#define GRJMPS	grjmps
#define GRHHFL	grhhfl
#define GRHHNL	grhhnl
#define GRHPAN	grhpan
#define GRLGND	grlgnd
#define GRLN	grln
#define GRLNCN	grlncn
#define GRMRKS	grmrks
#define GRMSKN	grmskn
#define GRMSKF	grmskf
#define GRNWPN	grnwpn
#define GRNXTF	grnxtf
#define GRPREC	grprec
#define GRPTS	grpts
#define GRSCAX	grscax
#define GRSCDL	grscdl
#define GRSCLC	grsclc
#define GRSCLP	grsclp
#define GRSCLV	grsclv
#define GRSHD	grshd
#define GRSHOW	grshow
#define GRSPHR	grsphr
#define GRSPTS	grspts
#define GRSTRT	grstrt
#define GRTXT	grtxt
#define GRTXTC	grtxtc
#define GRVAR	grvar
#define GRVFLD	grvfld
#define GRWIN	grwin
#define GSTXAL	gstxal
#define KURVEF	kurvef
#define PSKURF	pskurf
#define PSKURL	pskurl
#define SKURF	skurf
#define SKURL	skurl

#else

#define GR00DG	gr00dg_
#define GR90DG	gr90dg_
#define GRARRW	grarrw_
#define GRAXLIN	graxlin_
#define GRAXLOG	graxlog_
#define GRAXS	graxs_
#define GRAXSL	graxsl_
#define GRBLD	grbld_
#define GRCHN	grchn_
#define GRCHNC	grchnc_
#define GRCHRC	grchrc_
#define GRCLP	grclp_
#define GRCRCL	grcrcl_
#define GRDCUR	grdcur_
#define GRDEL	grdel_
#define GRDN	grdn_
#define GRDRAX	grdrax_
#define GRDRDM	grdrdm_
#define GRDRDU	grdrdu_
#define GRDRHS	grdrhs_
#define GRDRKU	grdrku_
#define GRDRLG	grdrlg_
#define GRDRNE	grdrne_
#define GRDRW	grdrw_
#define GRDRWS	grdrws_
#define GRDSH	grdsh_
#define GREND	grend_
#define GRENDE	grende_
#define GRFILL	grfill_
#define GRFONT	grfont_
#define GRFRBN	grfrbn_
#define GRGFLD	grgfld_
#define GRJMP	grjmp_
#define GRJMPS	grjmps_
#define GRHHFL	grhhfl_
#define GRHHNL	grhhnl_
#define GRHPAN	grhpan_
#define GRLGND	grlgnd_
#define GRLN	grln_
#define GRLNCN	grlncn_
#define GRMRKS	grmrks_
#define GRMSKN	grmskn_
#define GRMSKF	grmskf_
#define GRNWPN	grnwpn_
#define GRNXTF	grnxtf_
#define GRPREC	grprec_
#define GRPTS	grpts_
#define GRSCAX	grscax_
#define GRSCDL	grscdl_
#define GRSCLC	grsclc_
#define GRSCLP	grsclp_
#define GRSCLV	grsclv_
#define GRSHD	grshd_
#define GRSHOW	grshow_
#define GRSPHR	grsphr_
#define GRSPTS	grspts_
#define GRSTRT	grstrt_
#define GRTXT	grtxt_
#define GRTXTC	grtxtc_
#define GRVAR	grvar_
#define GRVFLD	grvfld_
#define GRWIN	grwin_
#define GSTXAL	gstxal_
#define KURVEF	kurvef_
#define PSKURF	pskurf_
#define PSKURL	pskurl_
#define SKURF	skurf_
#define SKURL	skurl_

#endif
#endif /* cray, _WIN32 */


