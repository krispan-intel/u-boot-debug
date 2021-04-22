// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2019 Intel Corporation
 */

#include <common.h>
#include <dm.h>
#include <image-svn.h>
#include <tee.h>

#define TA_SVN_UUID		      \
	{ 0xb3e0c1a2, 0x8209, 0x4826, \
	  { 0xb2, 0x22, 0xfa, 0x42, 0x3f, 0x2d, 0x08, 0x57 } }

/* The function IDs implemented in this TA */
#define TA_SVN_CMD_GET                  0
#define TA_SVN_CMD_SET                  1
#define TA_SVN_CMD_LOCK_UNTIL_REBOOT    2

static int open_session(struct udevice *dev, u32 *session)
{
	struct tee_open_session_arg arg;
	const struct tee_optee_ta_uuid uuid = TA_SVN_UUID;
	int rc;

	memset(&arg, 0, sizeof(arg));
	tee_optee_ta_uuid_to_octets(arg.uuid, &uuid);
	rc = tee_open_session(dev, &arg, 0, NULL);
	if (rc) {
		return rc;
	}
	if (arg.ret) {
		return -EIO;
	}
	*session = arg.session;

	return 0;
}

static int match(struct tee_version_data *vers, const void *data)
{
	return vers->gen_caps & TEE_GEN_CAP_GP;
}

static int ta_svn_get(struct udevice *dev, u32 session, u64 *svn)
{
	struct tee_param param = { .attr = TEE_PARAM_ATTR_TYPE_VALUE_OUTPUT };
	struct tee_invoke_arg arg;

	debug("%s()\n", __func__);
	memset(&arg, 0, sizeof(arg));
	arg.session = session;
	arg.func = TA_SVN_CMD_GET;

	if (tee_invoke_func(dev, &arg, 1, &param) || arg.ret) {
		return -EIO;
	}
	debug("TA result: %lld\n", param.u.value.a);
	*svn = ((u64)param.u.value.a << 32) | (u32)param.u.value.b;

	return 0;
}

static int ta_svn_set(struct udevice *dev, u32 session, u64 svn)
{
	struct tee_param param = { .attr = TEE_PARAM_ATTR_TYPE_VALUE_INPUT,
				   .u.value = { .a = (u32)(svn >> 32),
						.b = (u32)(svn) } };
	struct tee_invoke_arg arg;

	debug("%s()\n", __func__);
	memset(&arg, 0, sizeof(arg));
	arg.session = session;
	arg.func = TA_SVN_CMD_SET;

	if (tee_invoke_func(dev, &arg, 1, &param)) {
		return -EIO;
	}
	if (arg.ret) {
		debug("SVN TA ret: %d\n", arg.ret);
		return -EPERM;
	}

	return 0;
}

static int ta_svn_lock(struct udevice *dev, u32 session)
{
	struct tee_invoke_arg arg;

	debug("%s()\n", __func__);
	memset(&arg, 0, sizeof(arg));
	arg.session = session;
	arg.func = TA_SVN_CMD_LOCK_UNTIL_REBOOT;

	if (tee_invoke_func(dev, &arg, 0, NULL) || arg.ret) {
		return -EIO;
	}

	return 0;
}

int board_svn_get(u64 *svn)
{
	struct udevice *dev;
	u32 session = 0;
	int rc;

	debug("%s()\n", __func__);

	dev = tee_find_device(NULL, match, NULL, NULL);
	if (!dev) {
		printf("%s: Cannot find TEE device\n", __func__);
		return -ENODEV;
	}

	rc = open_session(dev, &session);
	if (rc) {
		printf("%s: Failed to open session\n", __func__);
		return rc;
	}

	rc = ta_svn_get(dev, session, svn);
	if (rc) {
		printf("%s: Failed to invoke function\n", __func__);
		/* Continue to close session. */
	}

	if (tee_close_session(dev, session)) {
		printf("%s: Failed to close session\n", __func__);
		/* Continue since this is not a critical error. */
	}
	return rc;
}

int board_svn_set(u64 svn)
{
	struct udevice *dev;
	u32 session = 0;
	int rc;

	debug("%s()\n", __func__);

	dev = tee_find_device(NULL, match, NULL, NULL);
	if (!dev) {
		printf("%s: Cannot find TEE device\n", __func__);
		return -ENODEV;
	}

	rc = open_session(dev, &session);
	if (rc) {
		printf("%s: Failed to open session\n", __func__);
		return rc;
	}

	rc = ta_svn_set(dev, session, svn);
	if (rc) {
		printf("%s: Failed to invoke function\n", __func__);
		/* Continue to close session. */
	}

	if (tee_close_session(dev, session)) {
		printf("%s: Failed to close session\n", __func__);
		/* Continue since this is not a critical error. */
	}

	return rc;
}

int board_svn_lock_until_reboot(void)
{
	struct udevice *dev;
	u32 session = 0;
	int rc;

	debug("%s()\n", __func__);

	dev = tee_find_device(NULL, match, NULL, NULL);
	if (!dev) {
		printf("%s: Cannot find TEE device\n", __func__);
		return -ENODEV;
	}

	rc = open_session(dev, &session);
	if (rc) {
		printf("%s: Failed to open session\n", __func__);
		return rc;
	}

	rc = ta_svn_lock(dev, session);
	if (rc) {
		printf("%s: Failed to invoke function\n", __func__);
		/* Continue to close session. */
	}

	if (tee_close_session(dev, session)) {
		printf("%s: Failed to close session\n", __func__);
		/* Continue since this is not a critical error. */
	}

	return rc;
}

