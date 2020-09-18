/*
  Copyright 2017 Northern.tech AS

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
  USA.
*/

#ifndef HEADER_ENV_MENDER_H
#define HEADER_ENV_MENDER_H

/* Avoid Mender specific code if we are just probing for configuration. */
#ifdef MENDER_AUTO_PROBING
#define MENDER_ENV_SETTINGS
#else

#ifdef MENDER_NO_DEFAULT_ALTBOOTCMD
#define MENDER_DEFAULT_ALTBOOTCMD
#else
#define MENDER_DEFAULT_ALTBOOTCMD                                              \
	"altbootcmd=run mender_altbootcmd; "                                   \
	"run bootcmd\0"
#endif

#define MENDER_DEFAULT_BOOTCMD                                                 \
	"run mender_setup; "                                                   \
	"setenv bootargs root=${mender_kernel_root} ${bootargs}; "             \
	"load ${mender_uboot_root} ${kernel_addr_r} ${mender_kernel_name}; "   \
	"${mender_boot_kernel_type} ${kernel_addr_r} - ${fdt_addr}; "          \
	"run mender_try_to_recover\0"

#define MENDER_ENV_SETTINGS                                                    \
	MENDER_DEFAULT_ALTBOOTCMD                                              \
	"bootlimit=1\0"                                                     \
	"bootcount=0\0"                                                     \
	"upgrade_available=0\0"                                             \
	"mender_boot_part=" __stringify(MENDER_KERNEL_PART_A_NUMBER) "\0"   \
	"mender_rootfs_part=" __stringify(MENDER_ROOTFS_PART_A_NUMBER) "\0" \
	"mender_uboot_boot=" MENDER_UBOOT_STORAGE_INTERFACE " " __stringify(MENDER_UBOOT_STORAGE_DEVICE) ":" __stringify(MENDER_BOOT_PART_NUMBER) "\0" \
	"mender_boot_kernel_type=" MENDER_BOOT_KERNEL_TYPE "\0"             \
	"mender_kernel_name=" MENDER_KERNEL_NAME "\0"                       \
	"mender_setup="                                                     \
	"setenv mender_kernel_root " MENDER_STORAGE_DEVICE_BASE "${mender_rootfs_part}; "	\
	"setenv mender_uboot_root " MENDER_UBOOT_STORAGE_INTERFACE " " __stringify(MENDER_UBOOT_STORAGE_DEVICE) ":${mender_boot_part}; " \
	"setenv expand_bootargs \"setenv bootargs \\\\\"${bootargs}\\\\\"\"; "              \
	"run expand_bootargs; "                                             \
	"setenv expand_bootargs\0"                                          \
	"mender_altbootcmd="                                                \
	"if test ${mender_boot_part} = " __stringify(MENDER_KERNEL_PART_A_NUMBER) "; "	\
	"then "										\
	"setenv mender_boot_part " __stringify(MENDER_KERNEL_PART_B_NUMBER) "; "	\
	"setenv mender_rootfs_part " __stringify(MENDER_ROOTFS_PART_B_NUMBER) "; "	\
	"else "										\
	"setenv mender_boot_part " __stringify(MENDER_KERNEL_PART_A_NUMBER) "; "	\
	"setenv mender_rootfs_part " __stringify(MENDER_ROOTFS_PART_A_NUMBER) "; "	\
	"fi; "					\
	"setenv upgrade_available 0; "		\
	"saveenv; "				\
	"run mender_setup\0"			\
						\
	"mender_try_to_recover="		\
	"if test ${upgrade_available} = 1; "	\
	"then reset; "				\
	"fi\0"

#endif /* !MENDER_AUTO_PROBING */

#endif /* HEADER_ENV_MENDER_H */
