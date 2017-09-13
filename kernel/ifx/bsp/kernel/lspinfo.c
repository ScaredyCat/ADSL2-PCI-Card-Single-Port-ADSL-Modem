//607101:leejack 2006/07/10 Integrate build environment

/*
 * <include/linux/lspinfo.h>
 *
 * Linux support package info to be exported in to proc.
 *
 * Author: MontaVista Software, Inc. <source@mvista.com>
 *
 * 2003 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
//607101:leejack
//#include <linux/lspinfo.h>
#include "./lspinfo.h"

static struct proc_dir_entry *lspinfo_dir, *lsp_revision_file,
    *lsp_board_name_file, *lsp_build_id_file, *lsp_name_file,
    *lsp_mvl_arch_file, *lsp_summary_file;

static int
read_lsp_board_name(char *page, char **start,
		    off_t off, int count, int *eof, void *data)
{

	int len;

	MOD_INC_USE_COUNT;

	len = sprintf(page, "%s\n", LSP_BOARD_NAME);

	MOD_DEC_USE_COUNT;

	return len;

}

static int
read_lsp_build_id(char *page, char **start,
		  off_t off, int count, int *eof, void *data)
{

	int len;

	MOD_INC_USE_COUNT;

	len = sprintf(page, "%s\n", LSP_BUILD_ID);

	MOD_DEC_USE_COUNT;

	return len;

}

static int
read_lsp_name(char *page, char **start,
	      off_t off, int count, int *eof, void *data)
{

	int len;

	MOD_INC_USE_COUNT;

	len = sprintf(page, "%s\n", LSP_NAME);

	MOD_DEC_USE_COUNT;

	return len;

}

static int
read_lsp_mvl_arch(char *page, char **start,
		  off_t off, int count, int *eof, void *data)
{

	int len;

	MOD_INC_USE_COUNT;

	len = sprintf(page, "%s\n", LSP_MVL_ARCH);

	MOD_DEC_USE_COUNT;

	return len;

}

static int
read_lsp_revision(char *page, char **start,
		  off_t off, int count, int *eof, void *data)
{

	int len;

	MOD_INC_USE_COUNT;

	len = sprintf(page, "%s\n", LSP_REVISION);

	MOD_DEC_USE_COUNT;

	return len;

}

static int
read_lsp_summary(char *page, char **start,
		 off_t off, int count, int *eof, void *data)
{

	int len;

	MOD_INC_USE_COUNT;

	len = sprintf(page, "Board Name		: %s\n"
		      "Lsp Name		: %s\n"
		      "LSP Revision		: %s.%s\n"
		      "MVL Architecture	: %s\n",
		      LSP_BOARD_NAME, LSP_NAME,
		      LSP_BUILD_ID, LSP_REVISION, LSP_MVL_ARCH);

	MOD_DEC_USE_COUNT;

	return len;

}

int __init
init_mvlversion(void)
{

	int ret = 0;

	lspinfo_dir = proc_mkdir("lspinfo", NULL);

	if (lspinfo_dir == NULL) {
		return -ENOMEM;
	}

	lspinfo_dir->owner = THIS_MODULE;

	lsp_revision_file =
	    create_proc_read_entry("revision", 0444, lspinfo_dir,
				   read_lsp_revision, NULL);

	if (lsp_revision_file == NULL) {
		ret = -ENOMEM;
		goto no_revision;
	}

	lsp_revision_file->owner = THIS_MODULE;

	lsp_mvl_arch_file =
	    create_proc_read_entry("mvl_arch", 0444, lspinfo_dir,
				   read_lsp_mvl_arch, NULL);

	if (lsp_mvl_arch_file == NULL) {
		ret = -ENOMEM;
		goto no_arch;
	}

	lsp_mvl_arch_file->owner = THIS_MODULE;

	lsp_board_name_file =
	    create_proc_read_entry("board_name", 0444, lspinfo_dir,
				   read_lsp_board_name, NULL);

	if (lsp_board_name_file == NULL) {
		ret = -ENOMEM;
		goto no_board;
	}

	lsp_board_name_file->owner = THIS_MODULE;

	lsp_name_file =
	    create_proc_read_entry("lsp_name", 0444, lspinfo_dir, read_lsp_name,
				   NULL);

	if (lsp_name_file == NULL) {
		ret = -ENOMEM;
		goto no_name;
	}

	lsp_name_file->owner = THIS_MODULE;

	lsp_build_id_file =
	    create_proc_read_entry("build_id", 0444, lspinfo_dir,
				   read_lsp_build_id, NULL);

	if (lsp_build_id_file == NULL) {
		ret = -ENOMEM;
		goto no_build_id;
	}

	lsp_build_id_file->owner = THIS_MODULE;

	lsp_name_file->owner = THIS_MODULE;

	lsp_summary_file =
	    create_proc_read_entry("summary", 0444, lspinfo_dir,
				   read_lsp_summary, NULL);

	if (lsp_summary_file == NULL) {
		ret = -ENOMEM;
		goto no_summary;
	}

	printk(KERN_INFO "LSP Revision %s\n", LSP_REVISION);
	goto out;

      no_summary:
	remove_proc_entry("build_id", lspinfo_dir);
      no_build_id:
	remove_proc_entry("lsp_name", lspinfo_dir);
      no_name:
	remove_proc_entry("board_name", lspinfo_dir);
      no_board:
	remove_proc_entry("mvl_arch", lspinfo_dir);
      no_arch:
	remove_proc_entry("revision", lspinfo_dir);
      no_revision:
	remove_proc_entry("lspinfo", NULL);
	printk(KERN_INFO "LSP Revision information unavalible\n");
      out:
	return ret;

}

void __exit
cleanup_mvlversion(void)
{

	remove_proc_entry("build_id", lspinfo_dir);
	remove_proc_entry("summary", lspinfo_dir);
	remove_proc_entry("lsp_name", lspinfo_dir);
	remove_proc_entry("board_name", lspinfo_dir);
	remove_proc_entry("mvl_arch", lspinfo_dir);
	remove_proc_entry("revision", lspinfo_dir);
	remove_proc_entry("lspinfo", NULL);

}

module_init(init_mvlversion);
module_exit(cleanup_mvlversion);

MODULE_AUTHOR("source@mvista.com");
MODULE_DESCRIPTION("LSP Proc Infomation");
MODULE_LICENSE("GPL");
