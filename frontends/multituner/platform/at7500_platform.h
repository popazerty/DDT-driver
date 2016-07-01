/*
 * @brief at7500_platform.c
 *
 * @author konfetti
 *
 * 	Copyright (C) 2011 duckbox
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _at7500_123
#define _at7500_123

#include <linux/version.h>
#include <linux/dvb/version.h>
#include <linux/dvb/frontend.h>

#include "avl2108_platform.h"
#include "avl2108_reg.h"
#include "avl2108.h"

#include "socket.h"
#include "tuner.h"
#include "lnb.h"

struct avl_private_data_s avl_tuner_priv =
{
	.ref_freq         = 1,
	.demod_freq       = 11200, /* fixme: the next three could be determined by the pll config!!! */
	.fec_freq         = 16800,
	.mpeg_freq        = 22400,
	.i2c_speed_khz    = TUNER_I2C_CLK,
	.agc_polarization = AGC_POL_INVERT,
	.mpeg_mode        = MPEG_FORMAT_TS_PAR,
	.mpeg_serial      = MPEG_MODE_PARALLEL,
	.mpeg_clk_mode    = MPEG_CLK_MODE_RISING,
	.max_lpf          = 0,
	.pll_config       = 5,
	.usedTuner        = cTUNER_INT_STV6306,
	.usedLNB          = cLNB_PIO,
	.lpf              = 193,
	.lock_mode        = LOCK_MODE_FIXED,
	.iq_swap          = CI_FLAG_IQ_NO_SWAPPED,
	.auto_iq_swap     = CI_FLAG_IQ_AUTO_BIT_AUTO,
	.agc_ref          = 0x30,
	.mpeg_data_clk    = -1,
};

struct platform_frontend_config_s avl2108_frontend =
{
	.name               = "avl2108",

	.demod_i2c          = 0x0C,
	.tuner_i2c          = 0xC0,
	.private            = &avl_tuner_priv,
};

struct tunersocket_s atevio7500_socket =
{
	.numSockets = 2,
	.socketList = (struct socket_s[])
	{
		[0] =
		{
			.name               = "socket-1",

			.tuner_enable       = {3, 3, 1},
			.lnb                = {2, 6, 0, 2, 5, 1},
			.i2c_bus            = 0,
		},
		[1] =
		{
			.name               = "socket-2",

			.tuner_enable       = {3, 2, 1},
			.lnb                = {4, 4, 0, 4, 3, 1},
			.i2c_bus            = 1,
		},
	},
};

struct platform_device avl2108_frontend_device =
{
	.name    = "avl2108",
	.id      = -1,
	.dev     = {
		.platform_data = &avl2108_frontend,
	},
	.num_resources        = 0,
	.resource             = NULL,
};

struct platform_device atevio7500_socket_device =
{
	.name    = "socket",
	.id      = -1,
	.dev     = {
		.platform_data = &atevio7500_socket,
	},
	.num_resources        = 0,
	.resource             = NULL,
};

struct platform_device *platform[] __initdata =
{
	&avl2108_frontend_device,
	&atevio7500_socket_device,
};

#endif
