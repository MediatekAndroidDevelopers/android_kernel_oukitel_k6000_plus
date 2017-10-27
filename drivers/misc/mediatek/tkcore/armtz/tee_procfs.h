#ifndef __TEE_PROCFS_H__
#define __TEE_PROCFS_H__

#define TEE_LOG_IRQ	280

#define TEE_LOG_CTL_BUF_SIZE	256

#define TEE_LOG_SIGNAL_THRESHOLD_SIZE 1024

#define TEE_CRASH_MAGIC_NO	0xdeadbeef


struct tee;

void tee_create_proc_entry(struct tee *tee);
void tee_delete_proc_entry(struct tee *tee);

int __init tee_init_procfs(void);
void __exit tee_exit_procfs(void);

#endif
