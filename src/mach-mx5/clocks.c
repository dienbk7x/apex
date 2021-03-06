/** @file clocks.c

   written by Marc Singer
   9 Aug 2011

   Copyright (C) 2011 Marc Singer

   -----------
   DESCRIPTION
   -----------

*/

#include <config.h>
#include <apex.h>
#include <service.h>
#include <arch-arm.h>
#include <command.h>
#include "clocks.h"
#include "hardware.h"

#define CLKIN   (24*1000*1000)

uint32_t decode_pll (uint32_t frequency, uint32_t mfn, uint32_t mfd, uint32_t op)
{
  uint32_t pd   = (op & 0xf) + 1;
  uint32_t mfi  = (op >> 4) & 0xf;
  mfi      = (mfi >= 5) ? mfi : 5;
  mfd     += 1;

  return ((4*(frequency/1000)*(mfi*mfd + mfn))/(mfd*pd))*1000;
}

uint32_t decode_pllx (int index)
{
  return decode_pll (CLKIN, DPLLx_DP_MFN(index), DPLLx_DP_MFD(index),
                     DPLLx_DP_OP(index));
}

uint32_t decode_pll1 (void)
{
  return decode_pll (CLKIN, DPLLx_DP_MFN(1), DPLLx_DP_MFD(1), DPLLx_DP_OP(1));
}

uint32_t decode_pll2 (void)
{
  return decode_pll (CLKIN, DPLLx_DP_MFN(2), DPLLx_DP_MFD(2), DPLLx_DP_OP(2));
}

uint32_t decode_pll3 (void)
{
  return decode_pll (CLKIN, DPLLx_DP_MFN(3), DPLLx_DP_MFD(3), DPLLx_DP_OP(3));
}

uint32_t periph_clk (void)
{
  if ((CCM_CBCDR & (1<<25)) == 0)
    return decode_pll2 ();

  switch ((CCM_CBCMR >> 12) & 3) {
  case 0:
    return decode_pll1 ();
  case 1:
    return decode_pll3 ();
  case 2:                       /* lp_apm_clk */
  default:
    return 0;
  }
}

uint32_t ahb_clk (void)
{
  int ahb_podf = ((CCM_CBCDR >> 10) & 0x7) + 1;
  return periph_clk ()/ahb_podf;
}

uint32_t ipg_clk (void)
{
  int ipg_podf = ((CCM_CBCDR >>  8) & 0x3) + 1;
  return ahb_clk ()/ipg_podf;
}

uint32_t ipg_per_clk (void)
{
  if (CCM_CBCMR & 1)
    return ipg_clk ();
  else {
    int pred1 = ((CCM_CBCDR >> 6) & 3) + 1;
    int pred2 = ((CCM_CBCDR >> 3) & 7) + 1;
    int podf  = ((CCM_CBCDR >> 0) & 3) + 1;

    return periph_clk ()/(pred1 * pred2 * podf);
  }
}

uint32_t esdhcx_clk (int index)
{
  switch (index) {
  default:
  case 0:
    return 0;
  case 1:
    return decode_pllx (((CCM_CSCMR1 >> 20) & 3) + 1);
  case 2:
    return decode_pllx (((CCM_CSCMR1 >> 16) & 3) + 1);
  case 3:
    return esdhcx_clk  (((CCM_CSCMR1 >> 19) & 1) + 1);
  case 4:
    return esdhcx_clk  (((CCM_CSCMR1 >> 18) & 1) + 1);
  }
}

uint32_t cspi_clk (void)
{
  int pred = ((CCM_CSCDR2>>25) &    7) + 1;
  int podf = ((CCM_CSCDR2>>19) & 0x3f) + 1;
  return decode_pllx (((CCM_CSCMR1 >> 4) & 3) + 1)/(pred*podf);
}

uint32_t uart_clk (void)
{
  uint32_t freq = 0;

  switch ((CCM_CSCMR1 >> 24) & 3) {
  case 0:                       /* pll1 */
  case 1:                       /* pll2 */
  case 2:                       /* pll3 */
    freq = decode_pllx (((CCM_CSCMR1 >> 24) & 3) + 1);
  case 3:                       /* lp_apm */
    break;
  }

  return freq / ( (((CCM_CSCDR1 >> 3) & 7) + 1)
                 *(((CCM_CSCDR1 >> 0) & 7) + 1));
}

static const char* clock_speed (uint32_t frequency)
{
  static char sz[32];
  uint32_t fraction = 0;
  const char* units = "Hz";

  if (frequency > 1000*1000) {
    units      = "MHz";
    fraction   = (frequency/1000)%1000;
    frequency /= 1000*1000;
  }
  else if (frequency > 1000) {
    units      = "KHz";
    fraction   = frequency%1000;
    frequency /= 1000;
  }

  snprintf (sz, sizeof (sz), "%d", frequency);
  if (fraction) {
    if (fraction%100 == 0)
      snprintf (sz + strlen (sz), sizeof (sz) - strlen (sz), ".%01d",
                fraction/100);
    else if (fraction%10 == 0)
      snprintf (sz + strlen (sz), sizeof (sz) - strlen (sz), ".%02d",
                fraction/10);
    else
      snprintf (sz + strlen (sz), sizeof (sz) - strlen (sz), ".%03d",
                fraction);
  }
  if (units)
    snprintf (sz + strlen (sz), sizeof (sz) - strlen (sz), "%s", units);

  return sz;
}

static void clocks_report (void)
{
  printf ("  clocks: clkin %s", clock_speed (CLKIN));
  printf ("  pll1 %s", clock_speed (decode_pll1 ()));
  printf ("  pll2 %s", clock_speed (decode_pll2 ()));
  printf ("  pll3 %s\n", clock_speed (decode_pll3 ()));
  printf ("          esdhc1 %s", clock_speed (esdhcx_clk (1)));
  printf ("  esdhc2 %s", clock_speed (esdhcx_clk (2)));
  printf ("  esdhc3 %s", clock_speed (esdhcx_clk (3)));
  printf ("  esdhc4 %s\n", clock_speed (esdhcx_clk (4)));
  printf ("          cspi %s", clock_speed (cspi_clk ()));
  printf ("  ahb %s", clock_speed (ahb_clk ()));
  printf ("  ipg %s", clock_speed (ipg_clk ()));
  printf ("  ipg_per %s\n", clock_speed (ipg_per_clk ()));
  printf ("          uart %s\n", clock_speed (uart_clk ()));
}

static __service_7 struct service_d clock_service = {
  .name        = "clocks-imx51",
  .description = "Freescale iMX51x clock service",
  .report      = clocks_report,
};

